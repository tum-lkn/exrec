#!/bin/bash

if [ "$EUID" -ne 0 ]; then
  echo "This script must run as root"
  exit 1
fi

if [ $# -eq 0 ]; then
  echo "Usage: $0 port_pci_address"
  exit 1
fi

PORT=$1

# Checking docker is installed
docker --version
if [ $? != 0 ]; then
  echo "Docker does not seem to be installed!"
  exit 1
fi

# Kill DPDK
pkill -9 main

# Wait for DPDK to be fully out
while ps aux | grep app/build/main | grep -v grep; do
  :
done

# Ensure container is stopped
docker stop dpdk

# Check the port exists
lspci -D | grep $PORT
if [ $? != 0 ]; then
  echo "The specified port does not exist!"
  cd - || exit 1
  exit 1
fi

IFC_NAME=$(ls -l /sys/class/net/ | grep $PORT | rev | cut -d "/" -f 1 | rev)
ip link set dev $IFC_NAME up
