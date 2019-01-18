#!/bin/bash

[ $# -lt 1 ] && { echo "Usage: $0 dcore_version [git_revision]"; exit 1; }

DCORE_VERSION=$1
PBC_VERSION=0.5.14
if [ $# -lt 2 ]; then GIT_REV=$DCORE_VERSION; else GIT_REV=$2; fi

. /etc/os-release

BASEDIR=$(dirname "$0")
echo "Building DCore $DCORE_VERSION (git revision $GIT_REV) for $PRETTY_NAME"

# build CMake and Boost
export BOOST_ROOT=/boost/boost-1.65.1_prefix
mkdir boost
cd boost
wget https://sourceforge.net/projects/boost/files/boost/1.65.1/boost_1_65_1.tar.gz
tar xvf boost_1_65_1.tar.gz
mkdir boost-1.65.1_prefix
cd boost_1_65_1
./bootstrap.sh --prefix=$BOOST_ROOT
./b2 install
cd ..
rm -rf boost_1_65_1 boost_1_65_1.tar.gz
cd ..
export CMAKE_ROOT=/cmake/cmake-3.13.1_prefix
export PATH=$CMAKE_ROOT/bin:$PATH
mkdir cmake
cd cmake
wget https://cmake.org/files/v3.13/cmake-3.13.1.tar.gz
tar xvf cmake-3.13.1.tar.gz
mkdir cmake-3.13.1_prefix
cd cmake-3.13.1
./configure --prefix=$CMAKE_ROOT
make
make install
cd ../..

# install PBC
wget https://github.com/DECENTfoundation/pbc/releases/download/$PBC_VERSION/libpbc_$PBC_VERSION-debian${VERSION_ID}_amd64.deb
wget https://github.com/DECENTfoundation/pbc/releases/download/$PBC_VERSION/libpbc-dev_$PBC_VERSION-debian${VERSION_ID}_amd64.deb

dpkg -i libpbc_$PBC_VERSION-debian${VERSION_ID}_amd64.deb
dpkg -i libpbc-dev_$PBC_VERSION-debian${VERSION_ID}_amd64.deb

# build DCore
git clone --single-branch --branch $GIT_REV https://github.com/DECENTfoundation/DECENT-Network.git
cd DECENT-Network
git submodule update --init --recursive
mkdir build
cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..
make install

# copy the binaries
mkdir -p ../../dcore-node/usr/bin
mkdir -p ../../dcore-node/DEBIAN
mkdir -p ../../dcore-node/etc/systemd/system
cp ../DCore.service ../../dcore-node/etc/systemd/system
cp programs/decentd/decentd programs/cli_wallet/cli_wallet ../../dcore-node/usr/bin
mkdir -p ../../dcore-gui/usr/bin
mkdir -p ../../dcore-gui/DEBIAN
cp programs/gui_wallet/DECENT ../../dcore-gui/usr/bin
cd ../..

# generate the control files
echo "Package: DCore" > dcore-node/DEBIAN/control
echo "Version: $DCORE_VERSION" >> dcore-node/DEBIAN/control
echo "Maintainer: DECENT <support@decent.ch>" >> dcore-node/DEBIAN/control
echo "Homepage: https://decent.ch" >> dcore-node/DEBIAN/control
echo "Source: https://github.com/DECENTfoundation/DECENT-Network/archive/$DCORE_VERSION.tar.gz" >> dcore-node/DEBIAN/control
echo "Section: net" >> dcore-node/DEBIAN/control
echo "Priority: optional" >> dcore-node/DEBIAN/control
echo "Architecture: amd64" >> dcore-node/DEBIAN/control
echo "Depends: libpbc, libreadline7, libcrypto++6, libssl1.1, libcurl3" >> dcore-node/DEBIAN/control
echo "Description: Fast, powerful and cost-efficient blockchain." >> dcore-node/DEBIAN/control
echo " DCore is the blockchain you can easily build on. As the world’s first blockchain" >> dcore-node/DEBIAN/control
echo " designed for digital content, media and entertainment, it provides user-friendly" >> dcore-node/DEBIAN/control
echo " software development kits (SDKs) that empower developers and businesses to build" >> dcore-node/DEBIAN/control
echo " decentralized applications for real-world use cases. DCore packed-full of" >> dcore-node/DEBIAN/control
echo " customizable features making it the ideal blockchain for any size project." >> dcore-node/DEBIAN/control

echo "Package: DCore-GUI" > dcore-gui/DEBIAN/control
echo "Version: $DCORE_VERSION" >> dcore-gui/DEBIAN/control
echo "Maintainer: DECENT <support@decent.ch>" >> dcore-gui/DEBIAN/control
echo "Homepage: https://decent.ch" >> dcore-gui/DEBIAN/control
echo "Source: https://github.com/DECENTfoundation/DECENT-Network/archive/$DCORE_VERSION.tar.gz" >> dcore-gui/DEBIAN/control
echo "Section: net" >> dcore-gui/DEBIAN/control
echo "Priority: optional" >> dcore-gui/DEBIAN/control
echo "Architecture: amd64" >> dcore-gui/DEBIAN/control
echo "Depends: libpbc, libreadline7, libcrypto++6, libssl1.1, libcurl3, qt5-default" >> dcore-gui/DEBIAN/control
echo "Description: Fast, powerful and cost-efficient blockchain." >> dcore-gui/DEBIAN/control
echo " DCore is the blockchain you can easily build on. As the world’s first blockchain" >> dcore-gui/DEBIAN/control
echo " designed for digital content, media and entertainment, it provides user-friendly" >> dcore-gui/DEBIAN/control
echo " software development kits (SDKs) that empower developers and businesses to build" >> dcore-gui/DEBIAN/control
echo " decentralized applications for real-world use cases. DCore packed-full of" >> dcore-gui/DEBIAN/control
echo " customizable features making it the ideal blockchain for any size project." >> dcore-gui/DEBIAN/control

cp $BASEDIR/postinst dcore-node/DEBIAN
cp $BASEDIR/prerm dcore-node/DEBIAN

# build the deb packages
dpkg-deb --build dcore-node dcore-deb
mv dcore-deb/dcore_${DCORE_VERSION}_amd64.deb dcore-deb/dcore_${DCORE_VERSION}-debian${VERSION_ID}_amd64.deb

dpkg-deb --build dcore-gui dcore-deb
mv dcore-deb/dcore-gui_${DCORE_VERSION}_amd64.deb dcore-deb/dcore-gui_${DCORE_VERSION}-debian${VERSION_ID}_amd64.deb

# clean up
rm -rf DECENT-Network dcore-node dcore-gui
