#!/bin/bash

[ $# -lt 2 ] && { echo "Usage: $0 image_version dcore_version [git_revision]"; exit 1; }

if [ $# -lt 3 ]; then GIT_REV=$2; else GIT_REV=$3; fi

IMAGE=`docker images -q decent/ubuntu/build:$1`
if [ -z $IMAGE ]; then
    echo "Building decent/ubuntu/build:$1"
    docker build -t decent/ubuntu/build:$1 -f ubuntu/Dockerfile.build --build-arg IMAGE_VERSION=$1 ubuntu
else
    echo "Using existing decent/ubuntu/build:$1 image $IMAGE"
fi

docker run -it -w /root --rm --name ubuntu.build.$1 \
    --mount type=bind,src=$PWD/packages,dst=/root/dcore-deb \
    --mount type=bind,src=$PWD/ubuntu,dst=/root/ubuntu,readonly \
    decent/ubuntu/build:$1 ubuntu/build.sh $1 $2 $GIT_REV
docker build -t decent/ubuntu/dcore:$2 -f ubuntu/Dockerfile --build-arg DCORE_VERSION=$2 --build-arg IMAGE_VERSION=$1 packages
