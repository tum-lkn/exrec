#!/bin/bash
hostname=$(hostname)

# These servers have 128GB RAM, hence 64GB per NUMA node, hence, we use 60 huge pages per NUMA node for VMs
n_huge_pages=60

echo $n_huge_pages > /sys/devices/system/node/node0/hugepages/hugepages-1048576kB/nr_hugepages
echo $n_huge_pages > /sys/devices/system/node/node1/hugepages/hugepages-1048576kB/nr_hugepages
mkdir -p /mnt/huge
umount /mnt/huge
mount -t hugetlbfs -o pagesize=1G nodev /mnt/huge

# Set up LLC
apt-get update -y && apt-get install intel-cmt-cat
modprobe msr
pqos -e "llc@0:0=0x0f0;llc@0:1=0x00f;llc@0:2=0x700"
pqos -a "llc:1=20;llc:2=22"

echo "Run docker: $1 $2 $3 $4"
PORT_BASE_NAME="${2//:/_}"
docker stop dpdk
docker rm dpdk
# -it removed as ssh session is not a TTY
docker run -d --privileged \
	-v /sys/bus/pci/drivers:/sys/bus/pci/drivers \
	-v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages \
	-v /sys/devices/system/node:/sys/devices/system/node \
	-v /mnt/huge:/mnt/huge \
	-v /lib/modules:/lib/modules \
	-v /dev:/dev \
	-v /tmp:/tmp \
	-v $1:/root/config.json \
	--net="host" \
	--name dpdk \
	 docker_dpdk \
	$2 \
	$3 \
	$4
