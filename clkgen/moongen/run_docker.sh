#!/bin/bash

if [ $# -ne 5 ]; then
	echo "Usage: lua_script port period vlanmax duty"
	exit 1
fi

# Disable turbo boost
apt-get install -y msr-tools
modprobe msr
if [[ -z $(which rdmsr) ]]; then
  echo "msr-tools is not installed"
  exit 1
fi


cores=$(cat /proc/cpuinfo | grep processor | awk '{print $3}')
for core in $cores; do
  wrmsr -p${core} 0x1a0 0x4000850089
  state=$(sudo rdmsr -p${core} 0x1a0 -f 38:38)
  if [[ $state -eq 1 ]]; then
    : # OK
  else
    echo "turbo boost on core ${core} still enabled!"
    exit 1
  fi
done

# Disable power saving
apt-get install -y cpufrequtils
echo "GOVERNOR=\"performance\"" >/etc/default/cpufrequtils
service cpufrequtils restart


# use only 8GB hugetables
echo 8 > /sys/devices/system/node/node0/hugepages/hugepages-1048576kB/nr_hugepages
mkdir -p /mnt/huge
umount /mnt/huge
mount -t hugetlbfs -o pagesize=1G nodev /mnt/huge

docker stop clkgen
docker rm clkgen
docker run -d --privileged \
	-v /sys/bus/pci/drivers:/sys/bus/pci/drivers \
	-v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages \
	-v /sys/devices/system/node:/sys/devices/system/node \
	-v /mnt/huge:/mnt/huge \
	-v /lib/modules:/lib/modules \
	-v /dev:/dev \
	--net="host" \
	--name clkgen \
	docker_moongen \
	$1 \
  $2 \
	$3 \
	$4 \
	$5
