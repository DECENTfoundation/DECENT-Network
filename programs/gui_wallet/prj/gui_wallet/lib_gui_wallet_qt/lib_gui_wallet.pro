# File lib_gui_wallet.pro
#
# File created : 13 Dec 2016
# Created by   : Davit Kalantaryan (davit.kalantaryan@desy.de)
#
# This file can be used to produce Makefile for gui_wallet GUI application
# for Decent
#

BOOST_ROOT= ../../../../../opt/boost_1_57_0_unix
DECENT_ROOT = ../../../../DECENT-Network
DECENT_LIB = $$DECENT_ROOT/libraries

QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-variable
QMAKE_CXXFLAGS_WARN_ON += -Wno-sign-compare
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-function
QMAKE_CXXFLAGS_WARN_ON -= -Wunused-function

INCLUDEPATH += $$DECENT_LIB/wallet/include
INCLUDEPATH += $$DECENT_LIB/app/include
INCLUDEPATH += $$DECENT_LIB/encrypt/include

INCLUDEPATH += $$BOOST_ROOT/include
INCLUDEPATH += ../../../../DECENT-Network/libraries/fc/include
INCLUDEPATH +=  ../../../../DECENT-Network/libraries/app/include
INCLUDEPATH += ../../../../DECENT-Network/libraries/chain/include
INCLUDEPATH += ../../../../DECENT-Network/libraries/db/include
INCLUDEPATH += ../../../../DECENT-Network/libraries/plugins/market_history/include
INCLUDEPATH += ../../../../DECENT-Network/libraries/net/include
INCLUDEPATH += ../../../../DECENT-Network/libraries/plugins/debug_witness/include
INCLUDEPATH += ../../../../DECENT-Network/libraries/egenesis/include
INCLUDEPATH += ../../../../DECENT-Network/libraries/utilities/include
INCLUDEPATH += ../../../../DECENT-Network/libraries/wallet/include
INCLUDEPATH += ../../../../DECENT-Network/libraries/fc/vendor/secp256k1-zkp/include
INCLUDEPATH += ../../../../DECENT-Network/libraries/fc/vendor/websocketpp
INCLUDEPATH += ../../../../DECENT-Network/libraries/fc/vendor/secp256k1-zkp

DEFINES += USE_NUM_GMP

win32:SYSTEM_PATH = ../../../sys/win64
else { 
    CODENAME = $$system(lsb_release -c | cut -f 2)
    SYSTEM_PATH = ../../../sys/$$CODENAME
}

# Debug:DESTDIR = debug1
TEMPLATE = lib
DESTDIR = $$SYSTEM_PATH/bin
OBJECTS_DIR = $$SYSTEM_PATH/.objects
CONFIG += debug
CONFIG += c++11
greaterThan(QT_MAJOR_VERSION, 4):QT += widgets
#QT -= core
#QT -= gui
#QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter

QMAKE_CXXFLAGS += -msse4.2
QMAKE_CFLAGS += -msse4.2

LIBS += -L$$BOOST_ROOT/lib

LIBS += $$DECENT_LIB/wallet/libgraphene_wallet.a
LIBS += $$DECENT_LIB/utilities/libgraphene_utilities.a
LIBS += $$DECENT_LIB/egenesis/libgraphene_egenesis_brief.a
LIBS += $$DECENT_LIB/app/libgraphene_app.a
LIBS += $$DECENT_LIB/chain/libgraphene_chain.a
LIBS += $$DECENT_LIB/contrib/pbc/.libs/libpbc.a
LIBS += $$DECENT_LIB/wallet/libgraphene_wallet.a
LIBS += $$DECENT_LIB/encrypt/libdecent_encrypt.a
LIBS += $$DECENT_LIB/fc/libfc_debug.a
LIBS += $$DECENT_LIB/fc/vendor/secp256k1-zkp/src/project_secp256k1-build/.libs/libsecp256k1.a

LIBS += -lboost_system
LIBS += -lboost_thread
LIBS += -lboost_context
LIBS += -lboost_chrono
LIBS += -lboost_coroutine
LIBS += -lboost_date_time
LIBS += -lboost_filesystem

#LIBS += -lgmp
LIBS += -lssl
LIBS += -lz
LIBS += -lcrypto++
LIBS += -lcrypto
