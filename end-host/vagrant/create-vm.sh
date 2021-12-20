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
	# Make sure the huge pages are there
	if [ ! -d /mnt/huge ]; then
		echo "Huges pages seem not to be available!"
		exit 1
	fi

	# Make sure we can use the hugepages
	chmod -R 777 /mnt/huge
	if [ $? != 0 ]; then
		echo "Impossible to set permissions for the hugepages"
		exit 1
	fi

	# Make sure the virtIO socket is there
	if [ ! -S /tmp/sock0 ]; then
		echo "The vhost-net socket /tmp/sock0 does not exist!"
		exit 1
	fi

	# Make sure we can connect to the socket
	chmod ugo+rwx /tmp/sock0
	if [ $? != 0 ]; then
		echo "Impossible to set permissions for the socket file"
		exit 1
	fi

	# Creating directory for the VM
	VMDIR=/vagrant/$1
	if [ -d "$VMDIR" ]; then
		echo "A directory already exists for this VM ($VMDIR), probably it is already running!"
		exit 1
	fi
	mkdir $VMDIR

	# Checking that the vagrant template is there
	VAGRANT_TEMPLATE=$PROJECT/end-host/vagrant/Vagrantfile.template
	if [ ! -f "$VAGRANT_TEMPLATE" ]; then
		echo "The vagrant template file does not exist, cannot create VM!"
		exit 1
	fi

	# Checking that the boot script is there
	BOOT_SCRIPT=$PROJECT/end-host/vagrant/vagrant-docker-vm-boot.sh
	if [ ! -f "$BOOT_SCRIPT" ]; then
		echo "The boot script does not exist, cannot create VM!"
		exit 1
	fi

	# Checking perl is there
	perl --version
	if [ $? != 0 ]; then
		echo "Perl does not seem to be installed but is necessary for creating the VM Vagrantfile"
		exit 1
	fi

	export HOSTNAME=$(hostname)
	export VM_ID=$1
	export SSH_PORT=$((20000 + $VM_ID))
	# TODO(user) update if hostnames differ
	case $HOSTNAME in
		server1)
			export MAC=$(printf "02:da:e1:00:00:%02x" $VM_ID)
			;;
		server2)
			export MAC=$(printf "02:ba:57:1a:20:%02x" $VM_ID)
			;;
		server3)
			export MAC=$(printf "02:ca:a2:00:00:%02x" $VM_ID)
			;;
		server4)
			export MAC=$(printf "02:ba:11:ac:00:%02x" $VM_ID)
			;;
		*)
			export MAC=$(printf "02:00:00:00:00:%02x" $VM_ID)
			;;
	esac
	printf -v VM_ID "%02d" $VM_ID

	# Creating the Vagrantfile
	cat $VAGRANT_TEMPLATE | perl -p -e 's/\$HOSTNAME/$ENV{HOSTNAME}/eg' | perl -p -e 's/\$VM_ID/$ENV{VM_ID}/eg' | perl -p -e 's/\$SSH_PORT/$ENV{SSH_PORT}/eg' | perl -p -e 's/\$MAC/$ENV{MAC}/eg' > /vagrant/$1/Vagrantfile
	cp $BOOT_SCRIPT /vagrant/$1
	cp -r /home/lkn/rotor-emulation/end-host/vagrant/moongen-scripts /vagrant/$1/moongen-scripts
	cd /vagrant/$1
	vagrant up
	if [ $? != 0 ]; then
		echo "vagrant up failed!"
		cd -
		exit 1
	fi
	cd -
else
	echo "Invalid VM id: must be integer in [0, 64]"
	exit 1
fi
