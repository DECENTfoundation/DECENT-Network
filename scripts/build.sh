#!/bin/bash

[ $# -lt 3 ] && { echo "Usage: $0 base_image image_version dcore_version [git_revision] [build_type] [packages_dir]"; exit 1; }

BASE_IMAGE=$1
IMAGE_VERSION=$2
DCORE_VERSION=$3

if [ $# -lt 4 ]; then GIT_REV=$DCORE_VERSION; else GIT_REV=$4; fi

if [ $# -lt 5 ]; then BUILD_TYPE="Release"; else BUILD_TYPE=$5; fi

if [ $# -lt 6 ]; then PACKAGES_DIR="$PWD/packages/$BASE_IMAGE/$IMAGE_VERSION"; else PACKAGES_DIR=$6; fi

IMAGE_NAME=dcore.$BASE_IMAGE.build:$IMAGE_VERSION
IMAGE_HASH=`docker images -q $IMAGE_NAME`
if [ -z $IMAGE_HASH ]; then
    echo "Building $IMAGE_NAME"
    docker build -t $IMAGE_NAME -f $BASE_IMAGE/Dockerfile.build --build-arg IMAGE_VERSION=$IMAGE_VERSION $BASE_IMAGE
else
    echo "Using existing $IMAGE_NAME image $IMAGE_HASH"
fi

mkdir -p $PACKAGES_DIR
docker run -it -w /root --rm --name $BASE_IMAGE.build.$IMAGE_VERSION \
    --mount type=bind,src=$PACKAGES_DIR,dst=/root/packages \
    --mount type=bind,src=$PWD/$BASE_IMAGE,dst=/root/$BASE_IMAGE,readonly \
    $IMAGE_NAME $BASE_IMAGE/build.sh $DCORE_VERSION $GIT_REV $BUILD_TYPE
