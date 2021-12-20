#!/bin/bash
cd $(dirname $0) || exit 1

docker stop of_ctr
docker rm of_ctr

docker build -t rotor-controller:latest .

docker run -d --privileged \
  --net="host" \
  --name of_ctr \
  rotor-controller:latest \
  ryu-manager /root/controller/$1

cd - || exit
