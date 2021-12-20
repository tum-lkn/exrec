FROM python:3.8-slim-buster
MAINTAINER johannes.zerwas@tum.de

RUN apt-get update && apt-get install -y vim libncurses5-dev ncurses-base python3-pip
RUN pip3 install scapy ncclient netconf-console

COPY ./src /root/controller
