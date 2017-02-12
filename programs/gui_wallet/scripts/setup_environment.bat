#!/bin/sh
#
# Script      : setup_environment.bat
# Created on  : 04 Dec, 2016
# Created by  : Davit Kalantaryan (davit.kalantaryan@desy.de)
#
# This script is setting up the environment
# To make the environment persistant for caller shell 
#    following syntax should be used
# $. setup_environment.bat  # before calling this script 
#                           # dot (.) on shell shoulb be typed
#

# Home for boost libraries
# This variable should be overwriten, depending on host
#   or may be for the future this variable can be passed as 
#   an argument
BOOST_ROOT=~/works/.private/opt/boost_1_57_0_unix

# Add boost library path to LD_LIBRARY_PATH
LD_LIBRARY_PATH=$BOOST_ROOT/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH

# setting up default language for output
export LC_ALL="en_US.UTF-8"
