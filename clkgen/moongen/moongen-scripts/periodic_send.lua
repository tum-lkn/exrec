local mg     = require "moongen"
local memory = require "memory"
local device = require "device"
local filter = require "filter"
local stats  = require "stats"
local log    = require "log"
local os     = require "os"


require "utils"

local DST_MAC="ff:ff:ff:ff:ff:ff"
local SRC_IP		= "10.0.0.1"
local DST_IP		= "10.1.0.1"
local SRC_PORT		= 5000
local DST_PORT		= 12480

local VLAN_MIN = 2

function configure(parser)
	parser:description("Generates UDP traffic and measure latencies. Edit the source to modify constants like IPs.")
	parser:argument("txDev", "Device to transmit from."):convert(tonumber)
	parser:option("-p --period", "Slot period [us]"):default(100):convert(tonumber)
	parser:option("-v --vmax", "Max. VLAN ID"):default(8):convert(tonumber)
	parser:option("-d --duty", "Duty cycle"):default(1):convert(tonumber)
end

function master(args)
	txDev = device.config{port = args.txDev, rxQueues = 1, txQueues = 1, dropEnable=false}
	device.waitForLinks()
	log:info("Making sure the device negotiated 1G or 10G")
	while true do
		txSpeed = txDev:getLinkStatus().speed
		if txSpeed == 1000 or txSpeed == 10000 then
			break
		end
	end
	mg.sleepMillis(1000)
	mg.startTask("loadSlave", txDev:getTxQueue(0), args.period, args.vmax, args.duty)
	mg.waitForTasks()
end

local function fillUdpPacket(buf, len)
	buf:getUdpPacket():fill{
		ethDst = DST_MAC,
		ip4Src = SRC_IP,
		ip4Dst = DST_IP,
		udpSrc = SRC_PORT,
		udpDst = DST_PORT,
		pktLength = len
	}
end

function loadSlave(queue, delay, vlan_max, duty_cycle)
	day = duty_cycle * delay
	night = (1.0 - duty_cycle) * delay

	local mempool = memory.createMemPool(function(buf)
		fillUdpPacket(buf, size)
	end)
	-- Hacky to sent only one packet
	local bufs = mempool:bufArray(1)
	local txCtr = stats:newDevTxCounter(queue, "csv")

	local current_vlan = VLAN_MIN
	while mg.running() do
		bufs:alloc(66)
		for i, buf in ipairs(bufs) do
			local pkt = buf:getUdpPacket()
			pkt.payload.uint32[0] = bit.rshift(hton(170), 16) + bit.rshift(hton(current_vlan), 8)
		end
		-- UDP checksums are optional, so using just IPv4 checksums would be sufficient here
		bufs:offloadUdpChecksums()
		queue:send(bufs)

		current_vlan = current_vlan + 1
		if current_vlan > vlan_max then
		    current_vlan = VLAN_MIN
		end

		txCtr:update()

		mg.sleepMicros(day)

		-- Reconfiguration delay
		for i, buf in ipairs(bufs) do
			local pkt = buf:getUdpPacket()
			-- Sent VLAN id 1 which is invalid and results in no sending
			pkt.payload.uint32[0] = bit.rshift(hton(170), 16) + bit.rshift(hton(1), 8)
		end
		-- UDP checksums are optional, so using just IPv4 checksums would be sufficient here
		bufs:offloadUdpChecksums()
		queue:send(bufs)
		mg.sleepMicros(night)
	end
	txCtr:finalize()
end
