#!/bin/bash

IMAGETAG=kktech/firmware:v1

docker pull $IMAGETAG

docker run -t -v $(pwd):/root/keepkey-firmware $IMAGETAG /bin/sh -c "\
	cd /root/keepkey-firmware/libopencm3 && \
	make clean && \
    make && \
	cd /root/keepkey-firmware && \
	./b -d && \
	mkdir -p bin/debug/keepkey && \
    mv build/arm-none-gnu-eabi/debug/bin/*.bin bin/debug/keepkey/"
