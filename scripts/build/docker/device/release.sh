#!/bin/bash -e

KEEPKEY_FIRMWARE="$(dirname "$(dirname "$(dirname "$(dirname "$( cd "$(dirname "$0")" ; pwd -P )")")")")"
cd $KEEPKEY_FIRMWARE

IMAGETAG=kktech/firmware:v6-beta

docker pull $IMAGETAG

docker run -t \
  -v $(pwd):/root/keepkey-firmware \
  $IMAGETAG /bin/sh -c "\
      mkdir /root/build && cd /root/build && \
      cmake -C /root/keepkey-firmware/cmake/caches/device.cmake /root/keepkey-firmware \
        -DCMAKE_BUILD_TYPE=Release && \
      make && \
      mkdir -p /root/keepkey-firmware/bin && \
      cp bin/*.bin /root/keepkey-firmware/bin/"
