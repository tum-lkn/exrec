#!/bin/bash

if [ $# -ne 4 ]; then
	echo "Usage: $0 lua_script packet_size duration n_flows"
	exit 1
fi


# The port the app should use
PORT=0000:00:06.0

# Make sure the port is bound to the correct driver
modprobe uio_pci_generic
./MoonGen/libmoon/deps/dpdk/usertools/dpdk-devbind.py --unbind $PORT
./MoonGen/libmoon/deps/dpdk/usertools/dpdk-devbind.py --bind=uio_pci_generic $PORT
if [ $? != 0 ]; then
	echo "Impossible to bind $PORT to DPDK driver!"
	exit 1
fi

./MoonGen/build/MoonGen /root/moongen-scripts/$1.lua 0 --size $2 --duration $3 --flows $4

