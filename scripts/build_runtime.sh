#!/bin/bash

[ $# -lt 3 ] && { echo "Usage: $0 os_name os_image_version dcore_version [package_dir]"; exit 1; }

if [ $# -lt 4 ]; then PACKAGE_DIR="$PWD/packages"; else PACKAGE_DIR=$4; fi

BASEIMAGE_NAME="decent/$1/dcore"
DOCKERFILE_NAME="$1/Dockerfile"
docker build -t $BASEIMAGE_NAME:$3 -f $DOCKERFILE_NAME --build-arg DCORE_VERSION=$3 --build-arg IMAGE_VERSION=$2 $PACKAGE_DIR
