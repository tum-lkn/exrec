#!/bin/bash

# Hennes has only 8GB memory
echo 8 > /sys/devices/system/node/node0/hugepages/hugepages-1048576kB/nr_hugepages
mkdir -p /mnt/huge
umount /mnt/huge
mount -t hugetlbfs -o pagesize=1G nodev /mnt/huge
