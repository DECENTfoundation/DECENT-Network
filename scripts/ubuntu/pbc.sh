#!/bin/bash

[ $# -lt 2 ] && { echo "Usage: $0 image_version pbc_version [git_revision]"; exit 1; }

PBC_VERSION=$2
if [ $# -lt 3 ]; then GIT_REV=$2; else GIT_REV=$3; fi

docker run -it -w /root --rm --name ubuntu.build.$1 \
    --mount type=bind,src=$PWD/packages,dst=/root/dcore-deb \
    --mount type=bind,src=$PWD/ubuntu,dst=/root/ubuntu,readonly \
    decent/ubuntu/build:$1 ubuntu/pbc-build.sh $PBC_VERSION $GIT_REV
