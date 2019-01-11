#!/bin/bash

[ $# -lt 1 ] && { echo "Usage: $0 dcore_version"; exit 1; }

dnf install -y rpm-build rpm-devel rpmlint rpmdevtools qt5-qtbase-devel
rpmdev-setuptree
rpmbuild -bb –define "dcore_version $1" DCore.spec
