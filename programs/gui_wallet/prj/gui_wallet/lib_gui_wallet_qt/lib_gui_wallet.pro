# File lib_gui_wallet.pro
#
# File created : 13 Dec 2016
# Created by   : Davit Kalantaryan (davit.kalantaryan@desy.de)
#
# This file can be used to produce Makefile for gui_wallet GUI application
# for Decent
#

#http://doc.qt.io/qt-5/osx-deployment.html
#CONFIG += ALL_LIBS_FOUND
# For making test: '$qmake "CONFIG+=TEST" gui_wallet.pro'  , then '$make'

TEMPLATE = lib
include(common_gui_wallet.pri)
QT -= core
QT -= gui
include(lib_gui_wallet.pri)
