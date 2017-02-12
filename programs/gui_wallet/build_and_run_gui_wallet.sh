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

GUI_BASE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
#echo ${BASH_SOURCE[0]}
#echo $DIR
#exit 0
CURRENT_DIR=`pwd`

# cd to the project directory
cd $GUI_BASE/prj/gui_wallet/lib_gui_wallet_qt

# prepare Makefile using qt (better to use qt5)
qmake lib_gui_wallet.pro

# make project
make

cd ../gui_wallet_qt

# prepare Makefile using qt (better to use qt5)
#qmake “CONFIG+=USE_LIB” gui_wallet.pro
qmake CONFIG+=USE_LIB gui_wallet.pro

# make project
make

# cd to the directory where binaries created
cd $CURRENT_DIR

BINARY_DIR_BASE=.

if [[ "$OSTYPE" == "linux-gnu" ]]; then
	BINARY_DIR_BASE=$GUI_BASE/sys/`lsb_release -c | cut -f 2`/bin
	BINARY_DIR=$BINARY_DIR_BASE
elif [[ "$OSTYPE" == "darwin"* ]]; then
	BINARY_DIR_BASE=$GUI_BASE/sys/mac/bin
	BINARY_DIR=$BINARY_DIR_BASE/gui_wallet.app/Contents/MacOS
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

export DYLD_LIBRARY_PATH=$BINARY_DIR_BASE
echo “run following command to launch the GUI: ” $BINARY_DIR/gui_wallet
$BINARY_DIR/gui_wallet
