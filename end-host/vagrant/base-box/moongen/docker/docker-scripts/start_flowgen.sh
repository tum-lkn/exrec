#!/bin/bash

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

./MoonGen/build/MoonGen /root/moongen-scripts/single_flow.lua 0 $@
