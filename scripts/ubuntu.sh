#!/bin/bash

[ $# -lt 2 ] && { echo "Usage: $0 image_version dcore_version [git_revision]"; exit 1; }

if [ $# -lt 3 ]; then GIT_REV=$2; else GIT_REV=$3; fi

mkdir -p packages
docker run -it -w /root --name DCore --mount type=bind,src=$PWD/ubuntu-build.sh,dst=/root/ubuntu-build.sh,readonly --mount type=bind,src=$PWD/packages,dst=/root/dcore-deb decent/ubuntu/build:$1 ./ubuntu-build.sh "$2" "$GIT_REV"
# docker rm DCore
