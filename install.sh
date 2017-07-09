apt-get update
apt-get install build-essential autotools-dev automake autoconf libtool make cmake checkinstall realpath gcc g++ clang flex bison doxygen gettext git qt5-default libqt5svg5-dev libreadline-dev libcrypto++-dev libgmp-dev libdb-dev libdb++-dev libssl-dev libncurses5-dev libboost-all-dev libcurl4-openssl-dev python-dev libicu-dev libbz2-dev -y

echo "install gc++ and cmake"
# Install GCC 5 and Clang 3.5
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install gcc-5 g++-5 clang-3.5 -y
# Now use either gcc-5 and g++-5, or clang-3.5 and clang++-3.5 as C and C++ compilers.
export CC=gcc-5
export CXX=g++-5

# Download and build CMake 3.7.2
mkdir -p ~/dev/DECENTfoundation/DECENT-Network-third-party
cd ~/dev/DECENTfoundation/DECENT-Network-third-party
rm -rf cmake-3.7.2*
wget https://cmake.org/files/v3.7/cmake-3.7.2.tar.gz
tar xvf cmake-3.7.2.tar.gz
mkdir cmake-3.7.2_prefix
cd cmake-3.7.2
CMAKE_ROOT=$(realpath ../cmake-3.7.2_prefix)
./configure --prefix=$CMAKE_ROOT
make
make install
cd ..
rm -rf cmake-3.7.2 cmake-3.7.2.tar.gz
export PATH=$CMAKE_ROOT/bin:$PATH

# Download and build Boost 1.60.0
mkdir -p ~/dev/DECENTfoundation/DECENT-Network-third-party
cd ~/dev/DECENTfoundation/DECENT-Network-third-party
rm -rf boost_1_60_0* boost-1.60.0*
wget https://sourceforge.net/projects/boost/files/boost/1.60.0/boost_1_60_0.tar.gz
tar xvf boost_1_60_0.tar.gz
mkdir boost-1.60.0_prefix
cd boost_1_60_0
export BOOST_ROOT=$(realpath ../boost-1.60.0_prefix)
./bootstrap.sh --prefix=$BOOST_ROOT
./b2 install
cd ..
rm -rf boost_1_60_0 boost_1_60_0.tar.gz

# Clone the repo.
 mkdir -p ~/dev/DECENTfoundation
 cd ~/dev/DECENTfoundation
 git clone https://github.com/DECENTfoundation/DECENT-Network.git
 cd DECENT-Network
 git submodule update --init --recursive

# Build and install Decent.
 mkdir -p ~/dev/DECENTfoundation/DECENT-Network-build
 cd ~/dev/DECENTfoundation/DECENT-Network-build
 cmake -G "Unix Makefiles" -D CMAKE_BUILD_TYPE=Debug ~/dev/DECENTfoundation/DECENT-Network
 cmake --build . --target all -- -j -l 3.0
 cmake --build . --target install
 
cd ~/dev/DECENTfoundation/DECENT-Network/programs
mkdir ~/decent
cp ./cli_wallet/cli_wallet ~/decent/
cp ./decentd/decentd ~/decent
cd ~/decent
ls
echo "all done!"

