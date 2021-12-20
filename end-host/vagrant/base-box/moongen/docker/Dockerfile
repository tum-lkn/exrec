FROM ubuntu:18.04
MAINTAINER johannes.zerwas@tum.de

RUN apt-get update
RUN apt-get install -y build-essential cmake linux-headers-$(uname -r) pciutils libnuma-dev git python kmod iproute2 libtbb-dev
WORKDIR /root
RUN git clone https://github.com/emmericp/MoonGen.git
WORKDIR /root/MoonGen
RUN git checkout 834aed0d980e8388b979d28eccaa5e61bb7d6058
RUN chmod +x build.sh
# Add a sed command after pulling libmoon to comment out automatic interface binding
RUN sed -i "/cd libmoon*/a sed -i 's/^\\\.\\\/bind-interfaces\\\.sh/#.\\\/bind-interfaces.sh/' build.sh" build.sh
RUN ./build.sh
WORKDIR /root
COPY ./docker-scripts/start_app.sh /root/docker-scripts/start_app.sh
COPY ./docker-scripts/start_flowgen.sh /root/docker-scripts/start_flowgen.sh
COPY ./moongen-scripts/ /root/moongen-scripts/
RUN chmod +x /root/docker-scripts/start_app.sh
RUN chmod +x /root/docker-scripts/start_flowgen.sh
ENTRYPOINT ["/root/docker-scripts/start_app.sh"]
