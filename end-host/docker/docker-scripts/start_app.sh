#!/bin/bash

# Load variables
SCRIPT=$(readlink -f $0)
SCRIPTPATH=$(dirname $SCRIPT)
. $SCRIPTPATH/dpdk_profile.sh

# The port the app should use
# PATH_TO_CONFIG_JSON=$1
PORT_BASE_ADDR=$1
PORT_BASE_ADDR2=$2
SYNC_PORT=$3

# Activate driver
# modprobe uio_pci_generic
modprobe uio
insmod $RTE_SDK/$RTE_TARGET/kmod/igb_uio.ko
insmod $RTE_SDK/$RTE_TARGET/kmod/rte_kni.ko kthread_mode=multiple carrier=on

# Make sure the two ports are bound to the correct driver
$RTE_SDK/usertools/dpdk-devbind.py --bind=igb_uio $PORT_BASE_ADDR.0
$RTE_SDK/usertools/dpdk-devbind.py --bind=igb_uio $PORT_BASE_ADDR.1
$RTE_SDK/usertools/dpdk-devbind.py --bind=igb_uio $PORT_BASE_ADDR.2
$RTE_SDK/usertools/dpdk-devbind.py --bind=igb_uio $PORT_BASE_ADDR.3

if [ ! -z $SYNC_PORT ]; then
  # If the third param is provided, the second one is the second card and sync port becomes the third param
  # Sync port
  $RTE_SDK/usertools/dpdk-devbind.py --unbind $SYNC_PORT
  $RTE_SDK/usertools/dpdk-devbind.py --bind=igb_uio $SYNC_PORT

	$RTE_SDK/usertools/dpdk-devbind.py --bind=igb_uio $PORT_BASE_ADDR2.0
	$RTE_SDK/usertools/dpdk-devbind.py --bind=igb_uio $PORT_BASE_ADDR2.1
	$RTE_SDK/usertools/dpdk-devbind.py --bind=igb_uio $PORT_BASE_ADDR2.2
	$RTE_SDK/usertools/dpdk-devbind.py --bind=igb_uio $PORT_BASE_ADDR2.3
elif [ ! -z $PORT_BASE_ADDR2 ]; then
  # Sync port
  $RTE_SDK/usertools/dpdk-devbind.py --unbind $PORT_BASE_ADDR2
  $RTE_SDK/usertools/dpdk-devbind.py --bind=igb_uio $PORT_BASE_ADDR2
fi

if [ $? != 0 ]; then
	echo "Impossible to bind $PORT to DPDK driver!"
	exit 1
fi
 
# ~/dpdk-stable/usertools# ./cpu_layout.py
# ======================================================================
# Core and Socket Information (as reported by '/sys/devices/system/cpu')
# ======================================================================
#  
# cores =  [0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13]
# sockets =  [0, 1]
#
#         Socket 0    Socket 1   
#         --------    --------   
# Core 0  [0]         [1]        
# Core 1  [2]         [3]        
# Core 2  [4]         [5]        
# Core 3  [6]         [7]        
# Core 4  [8]         [9]        
# Core 5  [10]        [11]       
# Core 8  [12]        [13]       
# Core 9  [14]        [15]       
# Core 10 [16]        [17]       
# Core 11 [18]        [19]       
# Core 12 [20]        [21]       
# Core 13 [22]        [23]
#
#
# -l: Use ports 18, 20, 22 for DPDK. They are on the same socket as the NIC and
#     different cores.
# Note that we use the kernel parameter "isolcpus" to prevent the kernel from using
# lcores 18,20,22 to ensure our DPDK threads are not bothered.

./app/build/main --config="/root/config.json"

# Connect the interfaces back to the kernel
$RTE_SDK/usertools/dpdk-devbind.py --bind=i40e $PORT_BASE_ADDR.0
$RTE_SDK/usertools/dpdk-devbind.py --bind=i40e $PORT_BASE_ADDR.1
$RTE_SDK/usertools/dpdk-devbind.py --bind=i40e $PORT_BASE_ADDR.2
$RTE_SDK/usertools/dpdk-devbind.py --bind=i40e $PORT_BASE_ADDR.3

if [ ! -z $SYNC_PORT ]; then
  $RTE_SDK/usertools/dpdk-devbind.py --bind=i40e $PORT_BASE_ADDR2.0
  $RTE_SDK/usertools/dpdk-devbind.py --bind=i40e $PORT_BASE_ADDR2.1
  $RTE_SDK/usertools/dpdk-devbind.py --bind=i40e $PORT_BASE_ADDR2.2
  $RTE_SDK/usertools/dpdk-devbind.py --bind=i40e $PORT_BASE_ADDR2.3

  $RTE_SDK/usertools/dpdk-devbind.py --bind=ixgbe $SYNC_PORT
elif [ ! -z $PORT_BASE_ADDR2 ]; then
  $RTE_SDK/usertools/dpdk-devbind.py --bind=ixgbe $PORT_BASE_ADDR2
fi
