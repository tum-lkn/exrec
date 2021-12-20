#!/bin/bash
cd "$(dirname $0)" || exit 1

docker stop da_ctr
docker rm da_ctr

docker build -t da-controller:latest .

docker run -d --privileged \
  --net="host" \
  --name da_ctr \
  -v $(dirname $0)/mappings:/root/mappings \
  da-controller:latest \
  python3.8 /root/controller/$@

cd - || exit
