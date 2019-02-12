#!/bin/bash

[ $# -lt 2 ] && { echo "Usage: $0 image_name image_version"; exit 1; }

IMAGE_NAME=$1
IMAGE_VERSION=$2

docker tag $IMAGE_NAME:$IMAGE_VERSION decentnetwork/$IMAGE_NAME:$IMAGE_VERSION
docker tag $IMAGE_NAME:$IMAGE_VERSION decentnetwork/$IMAGE_NAME:latest
docker push decentnetwork/$IMAGE_NAME:$IMAGE_VERSION
docker push decentnetwork/$IMAGE_NAME:latest
