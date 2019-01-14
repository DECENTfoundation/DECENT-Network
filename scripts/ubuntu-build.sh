#!/bin/bash

[ $# -lt 3 ] && { echo "Usage: $0 image_version dcore_version git_revision"; exit 1; }

echo "Building DCore for Ubuntu $1..."
echo "DCore version is $2..."
echo "The git revision is $3..."

# build CMake and Boost if on Ubuntu 16.04
if [[ $1 == "16.04" ]]; then
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
   cd ..
fi

# build DCore
GIT_REV=$3
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
