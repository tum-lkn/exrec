#!/bin/bash
if [ $# -ne 5 ]; then
	echo "Usage: $0 lua_script port  period vlanmax duty"
	exit 1
fi


# The port the app should use
PORT=$2
echo $PORT
# Make sure the port is bound to the correct driver
modprobe uio_pci_generic
./MoonGen/libmoon/deps/dpdk/usertools/dpdk-devbind.py --unbind $PORT
./MoonGen/libmoon/deps/dpdk/usertools/dpdk-devbind.py --bind=uio_pci_generic $PORT
if [ $? != 0 ]; then
	echo "Impossible to bind $PORT to DPDK driver!"
	exit 1
fi

./MoonGen/build/MoonGen /root/moongen-scripts/$1.lua 0 -p $3 -v $4 -d $5

./MoonGen/libmoon/deps/dpdk/usertools/dpdk-devbind.py --unbind $PORT
# TODO fill in default driver
# ./MoonGen/libmoon/deps/dpdk/usertools/dpdk-devbind.py --bind= $PORT

