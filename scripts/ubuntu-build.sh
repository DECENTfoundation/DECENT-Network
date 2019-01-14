#!/bin/bash

[ $# -lt 2 ] && { echo "Usage: $0 dcore_version git_revision"; exit 1; }

echo "Building DCore version $1..."
echo "The git revision is $2..."

# build DCore
GIT_REV=$2
git clone --single-branch --branch $GIT_REV https://github.com/DECENTfoundation/DECENT-Network.git
cd DECENT-Network
git submodule update --init --recursive
mkdir build
cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..
make -j 3.0 decentd cli_wallet
make install

# copy the binaries
mkdir -p ../../dcore-deb/dcore-deb/usr/local/bin
mkdir -p ../../dcore-deb/dcore-deb/DEBIAN
cp programs/decentd/decentd programs/cli_wallet/cli_wallet ../../dcore-deb/dcore-deb/usr/local/bin
cd ../..
# rm -rf DECENT-Network

# generate the control file
echo "Source: decent" > dcore-deb/dcore-deb/DEBIAN/control
echo "Section: base" >> dcore-deb/dcore-deb/DEBIAN/control
echo "Priority: optional" >> dcore-deb/dcore-deb/DEBIAN/control
echo "Maintainer: DECENT <support@decent.ch>" >> dcore-deb/dcore-deb/DEBIAN/control
echo "Package: decent" >> dcore-deb/dcore-deb/DEBIAN/control
echo "Version: 1.3-2" >> dcore-deb/dcore-deb/DEBIAN/control
echo "Architecture: amd64" >> dcore-deb/dcore-deb/DEBIAN/control
echo "Description: DECENT" >> dcore-deb/dcore-deb/DEBIAN/control
echo " DECENT server installation package containing decentd and cli_wallet," >> dcore-deb/dcore-deb/DEBIAN/control
echo " without DECENT GUI" >> dcore-deb/dcore-deb/DEBIAN/control

# build the deb package
cd dcore-deb
dpkg-deb --build dcore-deb
