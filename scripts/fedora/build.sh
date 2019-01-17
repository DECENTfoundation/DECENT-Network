#!/bin/bash

[ $# -lt 2 ] && { echo "Usage: $0 image_version dcore_version [git_revision]"; exit 1; }

DCORE_VERSION=$2
PBC_VERSION=0.5.14
if [ $# -lt 3 ]; then GIT_REV=$2; else GIT_REV=$3; fi

BASEDIR=$(dirname "$0")
echo "Building DCore $DCORE_VERSION (git revision $GIT_REV) for Fedora $1"

if rpm -q --quiet libpbc-devel; then
    PBC=`rpm -q --queryformat "%{VERSION}" libpbc-devel`
    echo "Using installed PBC $PBC"
else
    echo "Downloading PBC $PBC_VERSION"
    FEDORA=`rpm -E "%{fedora}"`
    wget https://github.com/DECENTfoundation/pbc/releases/download/$PBC_VERSION/libpbc-$PBC_VERSION-1.fc$FEDORA.x86_64.rpm
    wget https://github.com/DECENTfoundation/pbc/releases/download/$PBC_VERSION/libpbc-devel-$PBC_VERSION-1.fc$FEDORA.x86_64.rpm
    rpm -i libpbc*
    rm libpbc*
fi

rpmbuild -bb -D "dcore_version $DCORE_VERSION" -D "pbc_version $PBC_VERSION" -D "git_revision $GIT_REV" $BASEDIR/DCore.spec
