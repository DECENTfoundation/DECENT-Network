#!/bin/bash

[ $# -lt 2 ] && { echo "Usage: $0 image_version dcore_version [git_revision]"; exit 1; }

if [ $# -lt 3 ]; then GIT_REV=$2; else GIT_REV=$3; fi

rpmbuild -bb libpbc.spec
rpm -i /root/rpmbuild/RPMS/x86_64/libpbc*
rpmbuild -bb -D "dcore_version $2" -D "git_revision $GIT_REV" DCore.spec
