local mg      = require "moongen"
local memory  = require "memory"
local device  = require "device"
local stats   = require "stats"
local log     = require "log"
local limiter = require "software-ratecontrol"
local pcap   = require "pcap"
local pf     = require "pf"

local ETH_DST	= "11:12:13:14:15:16"

function configure(parser)
	parser:argument("txPort", "Device to use."):args(1):convert(tonumber)
	parser:option("-r --rate", "Rate in Pkts/us."):convert(tonumber):default(0)
	parser:option("-c --rc", "Rate control method hw|sw|moongen.")
	parser:option("-t --threads", "Number of threads."):convert(tonumber):default(1)
	parser:option("-p --pattern", "Pattern cbr|poisson|custom"):default("cbr")
	parser:option("-l --pktsize", "Packet size in byte"):convert(tonumber):default(1440)
	parser:option("-u --dport", "Destination UDP port"):convert(tonumber):default(5001)
	parser:option("-d --dstip", "Destination IP address"):default("10.0.0.1")
	parser:option("-s --srcip", "Source IP address"):default("10.0.0.2")
	parser:option("-f --file", "Write result to a pcap file.")
	parser:option("-s --snap-len", "Truncate packets to this size."):convert(tonumber):target("snapLen")
	parser:option("-o --output", "File to output statistics to")
	parser:argument("filter", "A BPF filter expression."):args("*"):combine()
	local args = parser:parse()
	if args.filter then
		local ok, err = pcall(pf.compile_filter, args.filter)
		if not ok then
			parser:error(err)
		end
	end
	return args
end


function master(args)
	args.rate = args.rate
	args.threads = args.threads or 1
	args.pattern = args.pattern or "cbr"
	if args.pattern == "cbr" and args.threads ~= 1 then
		return log:error("cbr only supports one thread")
	end
	local txDev = device.config{port = args.txPort, txQueues = args.threads, rxQueues = args.threads, rssQueues = args.threads, disableOffloads = args.rc ~= "moongen"}
	device.waitForLinks()
	stats.startStatsTask{txDevices = {txDev}, rxDevices = {txDev}, file = args.output}
	for i = 1, args.threads do
		if args.rate > 0 then
			local rateLimiter
			if args.rc == "sw" then
				rateLimiter = limiter:new(txDev:getTxQueue(i - 1), args.pattern, 1 / args.rate * 1000)
			end
			mg.startTask(
					"loadSlave",
					txDev:getTxQueue(i - 1),
					txDev,
					args.rate,
					args.rc,
					args.pattern,
					rateLimiter,
					args.pktsize,
					args.srcip,
					args.dstip,
					args.dport,
					i,
					args.threads
			)
		end
		mg.startTask("dumper", txDev:getRxQueue(i - 1), args, i)
	end
	mg.waitForTasks()
end

function loadSlave(queue, txDev, rate, rc, pattern, rateLimiter, pktsize, src_ip, dst_ip, dst_port, threadId, numThreads)
	local mem = memory.createMemPool(4096, function(buf)
		buf:getUdpPacket():fill{
			ethSrc = txDev,
			ethDst = ETH_DST,
			ip4Src = src_ip,
			ip4Dst = dst_ip,
			udpDst = dst_port,
			pktLength = pktsize
		}
	end)
	if rc == "hw" then
		local bufs = mem:bufArray()
		if pattern ~= "cbr" then
			return log:error("HW only supports CBR")
		end
		queue:setRate(rate * (pktsize + 4) * 8)
		mg.sleepMillis(100) -- for good measure
		while mg.running() do
			bufs:alloc(pktsize)
			queue:send(bufs)
		end
	elseif rc == "sw" then
		-- larger batch size is useful when sending it through a rate limiter
		local bufs = mem:bufArray(128)
		local linkSpeed = txDev:getLinkStatus().speed
		while mg.running() do
			bufs:alloc(pktsize)
			if pattern == "custom" then
				for _, buf in ipairs(bufs) do
					buf:setDelay(rate * linkSpeed / 8)
				end
			end
			rateLimiter:send(bufs)
		end
	elseif rc == "moongen" then
		-- larger batch size is useful when sending it through a rate limiter
		local bufs = mem:bufArray(128)
		local dist = pattern == "poisson" and poissonDelay or function(x) return x end
		while mg.running() do
			bufs:alloc(pktsize)
			for _, buf in ipairs(bufs) do
				buf:setDelay(dist(10^10 / numThreads / 8 / (rate * 10^6) - pktsize - 24))
			end
			queue:sendWithDelay(bufs, rate * numThreads)
		end
	else
		log:error("Unknown rate control method")
	end
end

function dumper(queue, args, threadId)
	local handleArp = args.arp
	-- default: show everything
	local filter = args.filter and pf.compile_filter(args.filter) or function() return true end
	local snapLen = args.snapLen
	local writer
	local captureCtr, filterCtr
	if args.file then
		if args.threads > 1 then
			if args.file:match("%.pcap$") then
				args.file = args.file:gsub("%.pcap$", "")
			end
			args.file = args.file .. "-thread-" .. threadId .. ".pcap"
		else
			if not args.file:match("%.pcap$") then
				args.file = args.file .. ".pcap"
			end
		end
		writer = pcap:newWriter(args.file)
		captureCtr = stats:newPktRxCounter("Capture, thread #" .. threadId)
		filterCtr = stats:newPktRxCounter("Filter reject, thread #" .. threadId)
	end
	local bufs = memory.bufArray()
	while mg.running() do
		local rx = queue:tryRecv(bufs, 100)
		local batchTime = mg.getTime()
		for i = 1, rx do
			local buf = bufs[i]
			if filter(buf:getBytes(), buf:getSize()) then
				if writer then
					writer:writeBuf(batchTime, buf, snapLen)
					captureCtr:countPacket(buf)
				else
					buf:dump()
				end
			elseif filterCtr then
				filterCtr:countPacket(buf)
			end
			buf:free()
		end
		if writer then
			captureCtr:update()
			filterCtr:update()
		end
	end
	if writer then
		captureCtr:finalize()
		filterCtr:finalize()
		log:info("Flushing buffers, this can take a while...")
		writer:close()
	end
end