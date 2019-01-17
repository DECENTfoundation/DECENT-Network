#!/bin/bash

[ $# -lt 2 ] && { echo "Usage: $0 image_version pbc_version [git_revision]"; exit 1; }

PBC_VERSION=$2
if [ $# -lt 3 ]; then GIT_REV=$2; else GIT_REV=$3; fi

BASEDIR=$(dirname "$0")
echo "Building PBC $PBC_VERSION (git revision $GIT_REV) for Fedora $1"

docker run -it -w /root --rm --name fedora.build.$1 \
    --mount type=bind,src=$PWD/packages,dst=/root/rpmbuild/RPMS/x86_64 \
    --mount type=bind,src=$PWD/fedora,dst=/root/fedora,readonly \
    decent/fedora/build:$1 rpmbuild -bb -D "pbc_version $PBC_VERSION" -D "git_revision $GIT_REV" $BASEDIR/libpbc.spec
