#!/bin/bash

[ $# -lt 2 ] && { echo "Usage: $0 image_version dcore_version [git_revision] [package_dir]"; exit 1; }

PBC_VERSION=$2
if [ $# -lt 3 ]; then GIT_REV=$2; else GIT_REV=$3; fi

if [ $# -lt 4 ]; then PACKAGE_DIR="$PWD/packages"; else PACKAGE_DIR=$4; fi

docker run -it -w /root --rm --name debian.build.$1 \
    --mount type=bind,src=$PACKAGE_DIR,dst=/root/dcore-deb \
    --mount type=bind,src=$PWD/debian,dst=/root/debian,readonly \
    decent/debian/build:$1 debian/pbc-build.sh $PBC_VERSION $GIT_REV
