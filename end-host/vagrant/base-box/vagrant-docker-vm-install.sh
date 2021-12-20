#!/bin/bash

echo "nameserver 8.8.8.8" > /etc/resolv.conf

# Install scapy (through pip because python3-scapy is too old)
apt-get update
apt-get install -y python3-pip iperf3 iperf tcpdump nfs-common python3.7-minimal
pip3 install scapy

# Add ssh key to root
cat /dev/zero | ssh-keygen -q -N ""
cat /home/vagrant/host_public_key.pub >> /home/vagrant/.ssh/authorized_keys
cat /home/vagrant/host_public_key.pub >> /root/.ssh/authorized_keys
# TODO put ssh pubkeys of your controllers here
echo "ssh-rsa <<XXXX>> root@ocs-ctr" >> /root/.ssh/authorized_keys

# Move moongen code
cp -r /home/vagrant/moongen /root/moongen

# Install docker
chmod +x /root/moongen/install-docker.sh
/root/moongen/install-docker.sh

# Build MoonGen
cd /root/moongen/docker/
chmod +x build_docker.sh
./build_docker.sh
