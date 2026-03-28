FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
  cmake \
  gcc-arm-none-eabi \
  build-essential \
  git \
  libnewlib-arm-none-eabi \
  python3 \
  python3-pip \
  file \
  binutils \
  && apt-get clean

WORKDIR /opt
RUN git clone -b master https://github.com/raspberrypi/pico-sdk.git \
  && cd pico-sdk && git submodule update --init

ENV PICO_SDK_PATH=/opt/pico-sdk
WORKDIR /workspace
