#!/bin/bash

echo "nameserver 8.8.8.8" > /etc/resolv.conf

# Install scapy (through pip because python3-scapy is too old)
apt-get update
apt-get install -y nfs-common

# Add ssh key to root
cat /dev/zero | ssh-keygen -q -N ""
cat /home/vagrant/host_public_key.pub >> /home/vagrant/.ssh/authorized_keys
cat /home/vagrant/host_public_key.pub >> /root/.ssh/authorized_keys
# TODO(user) put ssh pubkeys of your controllers here
echo "ssh-rsa <<XXXX>> root@ocs-ctr" >> /root/.ssh/authorized_keys
cat /home/vagrant/vagrant-ml_id_rsa.key.pub >> /home/vagrant/.ssh/authorized_keys
cat /home/vagrant/vagrant-ml_id_rsa.key.pub >> /root/.ssh/authorized_keys
cp /home/vagrant/vagrant-ml_id_rsa.key /root/.ssh/id_rsa
cp /home/vagrant/vagrant-ml_id_rsa.key.pub /root/.ssh/id_rsa.pub

# Disable client side StrictHostKeyChecking
sed -i /StrictHostKeyChecking/s/ask/no/g /etc/ssh/ssh_config
sed -i /StrictHostKeyChecking/s/#//g /etc/ssh/ssh_config


cp /home/vagrant/install_horovod.sh /root/install_horovod.sh
chmod +x /root/install_horovod.sh

/root/install_horovod.sh