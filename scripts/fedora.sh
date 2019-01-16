#!/bin/bash

[ $# -lt 2 ] && { echo "Usage: $0 image_version dcore_version [git_revision]"; exit 1; }

if [ $# -lt 3 ]; then GIT_REV=$2; else GIT_REV=$3; fi

mkdir -p packages
docker run -it -w /root --rm --name fedora.build.$1 \
    --mount type=bind,src=$PWD/packages,dst=/root/rpmbuild/RPMS/x86_64 \
    --mount type=bind,src=$PWD/fedora,dst=/root/fedora,readonly \
    decent/fedora/build:$1 fedora/build.sh $1 $2 $GIT_REV
docker build -t decent/fedora/dcore:$2 -f Dockerfile.fedora --build-arg DCORE_VERSION=$2 --build-arg IMAGE_VERSION=$1 packages
