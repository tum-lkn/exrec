#!/bin/bash

if [ "$EUID" -ne 0 ]; then
	echo "This script must run as root"
	exit 1
fi

if [ $# -eq 0 ]; then
	echo "Usage: $0 vm_id"
	exit 1
fi

if [[ "$1" =~ ^[0-9]+$ ]] && [ "$1" -ge 0 -a "$1" -le 64 ]; then
	VMDIR=/vagrant/$1

	if [ -d "$VMDIR" ]; then
		# Turning off the VM
		cd $VMDIR
		vagrant destroy -f
		cd -

		# Removing directory
		rm -rf $VMDIR
	else
		echo "Directory $VMDIR of the VM does not exist, doing nothing!"
		exit 0
	fi
else
	echo "Invalid VM id: must be integer in [0, 64]"
	exit 1
fi
