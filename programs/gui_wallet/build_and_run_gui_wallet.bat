#!/bin/bash
#
# Script      : build_gui_wallet.bat
# Created on  : 14 Dec, 2016
# Created by  : Davit Kalantaryan (davit.kalantaryan@desy.de)
#
# This script is compiling 'gui_wallet' application for DECENT
# To make the environment prepared by this script
#    persistant for caller shell 
#    following syntax should be used
# $. build_gui_wallet.bat  # before calling this script 
#                           # dot (.) on shell shoulb be typed
#

CURRENT_DIR=`pwd`

# cd to the project directory
cd prj/gui_wallet/lib_gui_wallet_qt

# prepare Makefile using qt (better to use qt5)
qmake lib_gui_wallet.pro

# make project
make

cd ../gui_wallet_qt

# prepare Makefile using qt (better to use qt5)
qmake “CONFIG+=USE_LIB” gui_wallet.pro

# make project
make

# cd to the directory where binaries created
cd $CURRENT_DIR

BINARY_DIR=.

if [[ "$OSTYPE" == "linux-gnu" ]]; then
	BINARY_DIR0=$CURRENT_DIR/sys/`lsb_release -c | cut -f 2`/bin
	BINARY_DIR=$CURRENT_DIR/sys/`lsb_release -c | cut -f 2`/bin
        #cd ../../../sys/`lsb_release -c | cut -f 2`/bin
elif [[ "$OSTYPE" == "darwin"* ]]; then
	BINARY_DIR0=$CURRENT_DIR/sys/mac/bin
	BINARY_DIR=$CURRENT_DIR/sys/mac/bin/gui_wallet.app/Contents/MacOS
	#BINARY_DIR=$BINARY_DIR ../../../sys/mac/bin
        #cd ../../../sys/mac/bin
elif [[ "$OSTYPE" == "cygwin" ]]; then
        echo "POSIX compatibility layer and Linux environment emulation for Windows"
elif [[ "$OSTYPE" == "msys" ]]; then
        echo " Lightweight shell and GNU utilities compiled for Windows (part of MinGW)"
elif [[ "$OSTYPE" == "win32" ]]; then
        echo "this can happen."
elif [[ "$OSTYPE" == "freebsd"* ]]; then
        echo " ..."
else
        echo "Unknown."
fi

export DYLD_LIBRARY_PATH=$BINARY_DIR0
echo “run following command to launch the GUI: ” $BINARY_DIR/gui_wallet
$BINARY_DIR/gui_wallet
