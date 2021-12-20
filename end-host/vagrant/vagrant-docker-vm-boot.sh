#!/bin/bash

apt-get install iperf

# Up the main interface
ip link set dev eth1 up
ip link set eth1 mtu 8000

# Allocate huge pages
mkdir -p /mnt/huge
echo 150 > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages
mount -t hugetlbfs nodev /mnt/huge

# Sleep to make sure interface is correctly up
sleep 3

mkdir /mnt/nas-exrec
# TODO Mount NAS folder for data
mount -t nfs <path/to/nas> /mnt/nas-exrec

cp -r /home/vagrant/moongen-scripts /root/moongen-scripts

# Set realtime settings
echo 0 > /proc/sys/kernel/watchdog
echo 0 > /proc/sys/kernel/nmi_watchdog
echo -1 > /proc/sys/kernel/sched_rt_period_us
echo -1 > /proc/sys/kernel/sched_rt_runtime_us

