#!/bin/bash

[ $# -lt 2 ] && { echo "Usage: $0 image_version dcore_version [git_revision] [package_dir]"; exit 1; }

if [ $# -lt 3 ]; then GIT_REV=$2; else GIT_REV=$3; fi

if [ $# -lt 4 ]; then PACKAGE_DIR="$PWD/packages"; else PACKAGE_DIR=$4; fi

IMAGE=`docker images -q decent/debian/build:$1`
if [ -z $IMAGE ]; then
    echo "Building decent/debian/build:$1"
    docker build -t decent/debian/build:$1 -f debian/Dockerfile.build --build-arg IMAGE_VERSION=$1 debian
else
    echo "Using existing decent/debian/build:$1 image $IMAGE"
fi

docker run -it -w /root --rm --name debian.build.$1 \
    --mount type=bind,src=$PACKAGE_DIR,dst=/root/dcore-deb \
    --mount type=bind,src=$PWD/debian,dst=/root/debian,readonly \
    decent/debian/build:$1 debian/build.sh $2 $GIT_REV
