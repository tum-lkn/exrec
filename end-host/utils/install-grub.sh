#!/bin/bash

cp grub /etc/default/grub
update-grub

cp blacklist-nouveau.conf /etc/modprobe.d/blacklist-nouveau.conf
cp modules /etc/initramfs-tools/modules

update-initramfs -u -k all
