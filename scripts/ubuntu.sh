#!/bin/bash

[ $# -lt 2 ] && { echo "Usage: $0 image_version dcore_version [git_revision] [package_dir]"; exit 1; }

if [ $# -lt 3 ]; then GIT_REV=$2; else GIT_REV=$3; fi

if [ $# -lt 4 ]; then PACKAGE_DIR="$PWD/packages"; else PACKAGE_DIR=$4; fi

IMAGE=`docker images -q decent/ubuntu/build:$1`
if [ -z $IMAGE ]; then
    echo "Building decent/ubuntu/build:$1"
    docker build -t decent/ubuntu/build:$1 -f ubuntu/Dockerfile.build --build-arg IMAGE_VERSION=$1 ubuntu
else
    echo "Using existing decent/ubuntu/build:$1 image $IMAGE"
fi

docker run -it -w /root --rm --name ubuntu.build.$1 \
    --mount type=bind,src=$PACKAGE_DIR,dst=/root/dcore-deb \
    --mount type=bind,src=$PWD/ubuntu,dst=/root/ubuntu,readonly \
    decent/ubuntu/build:$1 ubuntu/build.sh $2 $GIT_REV
