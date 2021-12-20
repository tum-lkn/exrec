#!/bin/bash

SCRIPT=$(readlink -f $0)
SCRIPTPATH=$(dirname $SCRIPT)

. $SCRIPTPATH/dpdk_profile.sh
URL=https://git.dpdk.org/dpdk/snapshot/dpdk-$DPDK_VERSION.tar.gz

# Download DPDK
cd $BASEDIR
wget $URL
tar xzvf dpdk-$DPDK_VERSION.tar.gz

# Copy config
cp $BASEDIR/dpdk-config/* $RTE_SDK/config/

# Build
cd $RTE_SDK
make install T=$RTE_TARGET
