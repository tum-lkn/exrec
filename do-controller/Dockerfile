FROM python:3.8-slim-buster
MAINTAINER joahnnes.zerwas@tum.de

RUN apt-get update && apt-get install -y vim gcc
RUN pip install ryu

COPY ./src /root/controller

