#!/bin/bash

IP_ADDR=$1
IFACE=$2
TABLE=$3

ip a | grep $IFACE
if [ $? != 0 ]; then
  echo "virtual interface not created. is container already running?"
  exit 1
fi
sleep 5
ip addr add "$IP_ADDR/24" dev $IFACE
ip link set $IFACE up

ip route add 10.5.0.0/24 dev $IFACE src $IP_ADDR table $TABLE
ip rule add table $TABLE from $IP_ADDR 
