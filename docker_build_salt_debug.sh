#!/bin/bash

IMAGETAG=kktech/firmware:v1

docker pull $IMAGETAG

docker run -t \
  -v $(pwd):/root/keepkey-firmware \
  -v $(pwd)/../device-protocol:/root/device-protocol $IMAGETAG /bin/sh -c "\
	cd /root/keepkey-firmware/libopencm3 && \
	make clean && \
    make && \
	cd /root/keepkey-firmware && \
	./b -d -dl -salt && \
	mkdir -p bin/debug/salt && \
    mv build/arm-none-gnu-eabi/debug/bin/*.bin bin/debug/salt/ &&
    mv build/arm-none-gnu-eabi/debug/bin/*.map bin/debug/salt/ &&
    mv build/arm-none-gnu-eabi/debug/bin/*.elf bin/debug/salt/ &&
    mv build/arm-none-gnu-eabi/debug/bin/*.srec bin/debug/salt/"
