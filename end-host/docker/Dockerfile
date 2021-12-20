FROM ubuntu:18.04
MAINTAINER amaury.van-bemten@tum.de

# Dependencies (update in same command so that he reupdates if we need a new package)
RUN apt-get update -y && apt-get install -y pkg-config wget make coreutils gcc-multilib libnuma-dev linux-headers-$(uname -r) python iproute2 sudo kmod pciutils

# Do everything in root
WORKDIR /root

# Copy DPDK environments variables
COPY ./docker-scripts/dpdk_profile.sh /root/docker-scripts/dpdk_profile.sh

# Copy DPDK config
COPY ./dpdk-config/ /root/dpdk-config/

# Build DPDK
COPY ./docker-scripts/build_dpdk.sh /root/docker-scripts/build_dpdk.sh
RUN chmod +x /root/docker-scripts/build_dpdk.sh
RUN /root/docker-scripts/build_dpdk.sh

# Build application
COPY ./app /root/app
COPY ./docker-scripts/build_app.sh /root/docker-scripts/build_app.sh
RUN chmod +x /root/docker-scripts/build_app.sh
RUN /root/docker-scripts/build_app.sh

# Copy starting script
COPY ./docker-scripts/start_app.sh /root/docker-scripts/start_app.sh
RUN chmod +x /root/docker-scripts/start_app.sh

ENTRYPOINT ["/root/docker-scripts/start_app.sh"]
