FROM ubuntu:14.04

MAINTAINER darin@keepkey.com

# Install toolchain
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -yq make git scons gcc-arm-none-eabi python-protobuf protobuf-compiler fabric exuberant-ctags wget

# Install nanopb
WORKDIR /root
RUN git clone --branch nanopb-0.2.9.2 https://code.google.com/p/nanopb/
WORKDIR /root/nanopb/generator/proto
RUN git checkout maintenance_0.2
RUN make

# Setup environment
ENV PATH /root/nanopb/generator:$PATH
