#!/bin/bash

if [ $# -ne 4 ]; then
	echo "Usage: lua_script packet_size n_flows"
	exit 1
fi

docker rm moongen

docker run -d --privileged \
	-v /sys/bus/pci/drivers:/sys/bus/pci/drivers \
	-v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages \
	-v /sys/devices/system/node:/sys/devices/system/node \
	-v /mnt/huge:/mnt/huge \
	-v /lib/modules:/lib/modules \
	-v /dev:/dev \
	-v /root/moongen-scripts:/root/moongen-scripts \
	--net="host" \
	--name moongen \
	-ti docker_moongen \
	$1 $2 $3 $4
