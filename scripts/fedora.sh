#!/bin/bash

[ $# -lt 2 ] && { echo "Usage: $0 image_version dcore_version [git_revision]"; exit 1; }

if [ $# -lt 3 ]; then GIT_REV=$2; else GIT_REV=$3; fi

mkdir -p packages
docker run -it -w /root --rm --name fedora.build.$1 --mount type=bind,src=$PWD/DCore.spec,dst=/root/DCore.spec,readonly \
    --mount type=bind,src=$PWD/packages,dst=/root/rpmbuild/RPMS/x86_64 decent/fedora/build:$1 \
    rpmbuild -bb -D "dcore_version $2" -D "git_revision $GIT_REV" DCore.spec
docker build -t decent/fedora/dcore:$2 -f Dockerfile.fedora --build-arg DCORE_FILE=packages/DCore-$2-1.fc$1.x86_64.rpm --build-arg IMAGE_VERSION=$1 .
