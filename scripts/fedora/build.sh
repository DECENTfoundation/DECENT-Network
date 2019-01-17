#!/bin/bash

[ $# -lt 2 ] && { echo "Usage: $0 image_version dcore_version [git_revision]"; exit 1; }

DCORE_VERSION=$2
PBC_VERSION=0.5.14
if [ $# -lt 3 ]; then GIT_REV=$2; else GIT_REV=$3; fi

BASEDIR=$(dirname "$0")
echo "Building DCore $DCORE_VERSION (git revision $GIT_REV) for Fedora $1"

rpmbuild -bb -D "pbc_version $PBC_VERSION" $BASEDIR/libpbc.spec
rpm -i /root/rpmbuild/RPMS/x86_64/libpbc*
rpmbuild -bb -D "dcore_version $DCORE_VERSION" -D "pbc_version $PBC_VERSION" -D "git_revision $GIT_REV" $BASEDIR/DCore.spec
