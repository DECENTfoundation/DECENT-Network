#!/bin/bash

[ $# -lt 1 ] && { echo "Usage: $0 pbc_version [git_revision]"; exit 1; }


PBC_VERSION=$1
if [ $# -lt 2 ]; then GIT_REV=$PBC_VERSION; else GIT_REV=$2; fi

# build PBC
git clone --single-branch --branch $GIT_REV https://github.com/DECENTfoundation/pbc.git
cd pbc

./setup
./configure --prefix=/usr --libdir=/usr/lib/x86_64-linux-gnu
make install
rm -rf pbc-$PBC_VERSION pbc-$PBC_VERSION.tar.gz

cd ..
mkdir -p libpbc/usr/lib/x86_64-linux-gnu
cp /usr/lib/x86_64-linux-gnu/libpbc.* libpbc/usr/lib/x86_64-linux-gnu

mkdir -p libpbc/DEBIAN

cp pbc/debian/copyright libpbc/DEBIAN

echo "Package: libpbc" > libpbc/DEBIAN/control
echo "Version: $PBC_VERSION" >> libpbc/DEBIAN/control
echo "Maintainer: DECENT <support@decent.ch>" >> libpbc/DEBIAN/control
echo "Homepage: https://crypto.stanford.edu/pbc" >> libpbc/DEBIAN/control
echo "Source: https://crypto.stanford.edu/pbc/files/pbc-$PBC_VERSION.tar.gz" >> libpbc/DEBIAN/control
echo "Section: net" >> libpbc/DEBIAN/control
echo "Priority: optional" >> libpbc/DEBIAN/control
echo "Architecture: amd64" >> libpbc/DEBIAN/control
echo "Description: Pairing-Based Crypto library." >> libpbc/DEBIAN/control
echo " Pairing-based cryptography is a relatively young area of cryptography that" >> libpbc/DEBIAN/control
echo " revolves around a certain function with special properties. The PBC library is" >> libpbc/DEBIAN/control
echo " designed to be the backbone of implementations of pairing-based cryptosystems," >> libpbc/DEBIAN/control
echo " thus speed and portability are important goals. It provides routines such as" >> libpbc/DEBIAN/control
echo " elliptic curve generation, elliptic curve arithmetic and pairing computation." >> libpbc/DEBIAN/control

dpkg-deb --build libpbc dcore-deb
rm -rf libpbc
