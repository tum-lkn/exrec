#!/bin/bash

if [ "$EUID" -ne 0 ]; then
  echo "This script must run as root"
  exit 1
fi

if [ $# -eq 0 ]; then
  echo "Usage: $0 port_pci_address ip_address matching"
  exit 1
fi

PATH_TO_CONFIG_JSON=$1
PORT_BASE_ADDR=$2
PORT_BASE_ADDR2=$3
SYNC_PORT=$4
DOCKER_DIR=$PROJECT/end-host/docker

# Check if config exists
if [ ! -f "$PATH_TO_CONFIG_JSON" ]; then
  echo "Config file $PATH_TO_CONFIG_JSON does not exist!"
  exit 1
fi

# Security to allow people to connect to the socket
echo "security_driver = \"none\"" >> /etc/libvirt/qemu.conf
awk '!seen[$0]++' /etc/libvirt/qemu.conf > ok
mv ok /etc/libvirt/qemu.conf
service libvirtd restart
if [ $? != 0 ]; then
	echo "Impossible to set the QEMU config security driver"
	exit 1
fi
if [[ -f /tmp/sock0 ]]; then
	rm /tmp/sock0
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

# Checking the docker directory is there
if [ ! -d "$DOCKER_DIR" ]; then
  echo "Docker directory $DOCKER_DIR for DPDK does not exist!"
  exit 1
fi

# Checking the build and run files are there
BUILD_FILE=$DOCKER_DIR/build_docker.sh
RUN_FILE=$DOCKER_DIR/run_docker.sh
if [ ! -f "$BUILD_FILE" ]; then
  echo "Docker build file $BUILD_FILE for DPDK does not exist!"
  exit 1
fi

if [ ! -f "$RUN_FILE" ]; then
  echo "Docker run file $RUN_FILE for DPDK does not exist!"
  exit 1
fi

# Checking docker is installed
docker --version
if [ $? != 0 ]; then
  echo "Docker does not seem to be installed!"
  exit 1
fi

# Go to docker directory
cd $DOCKER_DIR || exit 1

# Build docker container
$BUILD_FILE
if [ $? != 0 ]; then
  echo "Failed to build the docker container!"
  cd - || exit 1
  exit 1
fi

# Check the port exists
lspci -D | grep $PORT_BASE_ADDR
if [ $? != 0 ]; then
  echo "The specified port does not exist!"
  cd - || exit 1
  exit 1
fi

echo "$RUN_FILE $PATH_TO_CONFIG_JSON $PORT_BASE_ADDR $PORT_BASE_ADDR2 $SYNC_PORT"
# Run the docker container
$RUN_FILE $PATH_TO_CONFIG_JSON $PORT_BASE_ADDR $PORT_BASE_ADDR2 $SYNC_PORT
if [ $? != 0 ]; then
  echo "Failed to start the docker container: is it already running?"
  cd - || exit 1
  exit 1
fi

cd - || exit 1
