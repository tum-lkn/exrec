#!/bin/bash

SCRIPT=$(readlink -f $0)
SCRIPTPATH=$(dirname $SCRIPT)

. $SCRIPTPATH/dpdk_profile.sh
cd $BASEDIR/app
make
