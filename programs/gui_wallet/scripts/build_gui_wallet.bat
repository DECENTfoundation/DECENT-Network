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

# cd to the project directory
cd projects/gui_wallet/gui_wallet_qt

# prepare Makefile using qt (better to use qt5)
qmake gui_wallet.pro

# make project
make

# cd to the directory where binaries created

if [[ "$OSTYPE" == "linux-gnu" ]]; then
        cd ../../../sys/`lsb_release -c | cut -f 2`/bin
elif [[ "$OSTYPE" == "darwin"* ]]; then
        cd ../../../sys/mac/bin
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
