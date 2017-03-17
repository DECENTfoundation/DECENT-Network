# File common_gui_wallet.pri
#
# File created : 06 Feb 2017
# Created by   : Davit Kalantaryan (davit.kalantaryan@desy.de)
#
# This file can be used to produce Makefile for gui_wallet GUI application
# for Decent
#

#http://doc.qt.io/qt-5/osx-deployment.html
#CONFIG += ALL_LIBS_FOUND
# For making test: '$qmake "CONFIG+=TEST" gui_wallet.pro'  , then '$make'

DECENT_ROOT_DEFAULT = ../../../../..

DECENT_ROOT_DEV = $$(DECENT_ROOT)
equals(DECENT_ROOT_DEV, ""): DECENT_ROOT_DEV = $$DECENT_ROOT_DEFAULT
message("!!!!!! DECENT_ROOT is '"$$DECENT_ROOT_DEV"'")

DECENT_LIB = $$DECENT_ROOT_DEV/libraries


# calculation of BOOST_ROOT_DEFAULT
win32{
    BOOST_ROOT_DEFAULT = ../../../..
}else{
    exists( /usr/local/opt/boost/libboost_thread* ){
        BOOST_ROOT_DEFAULT = /usr/local/opt/boost
    }else{
        macx{
            BOOST_ROOT_DEFAULT   = /usr/local/Cellar/boost160/1.60.0
        }else{
            BOOST_ROOT_DEFAULT = ../../../../../opt/boost_1_57_0_unix
        }
    } # else of exists( /usr/local/opt/boost/libboost_thread* )
} # else of win32


BOOST_ROOT_QT = $$(BOOST_ROOT)
equals(BOOST_ROOT_QT, ""): BOOST_ROOT_QT = $$BOOST_ROOT_DEFAULT
message("!!!!!! BOOST_ROOT is '"$$BOOST_ROOT_QT"'")


# disabling some non important warnings
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-variable
QMAKE_CXXFLAGS_WARN_ON += -Wno-sign-compare
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-function
QMAKE_CXXFLAGS_WARN_ON -= -Wunused-function

INCLUDEPATH += $$DECENT_LIB/contrib/fc/include
INCLUDEPATH += ../../../include
INCLUDEPATH += $$BOOST_ROOT_QT/include

DEFINES += USE_NUM_GMP

win32{
    SYSTEM_PATH = ../../../sys/win64
}else {
    macx{
        SYSTEM_PATH = ../../../sys/mac
    }else{
        CODENAME = $$system(lsb_release -c | cut -f 2)
        SYSTEM_PATH = ../../../sys/$$CODENAME
    } # else of macx
} # else of win32

# Debug:DESTDIR = debug1
DESTDIR = $$SYSTEM_PATH/bin
OBJECTS_DIR = $$SYSTEM_PATH/.objects
CONFIG += debug
CONFIG += c++11
#greaterThan(QT_MAJOR_VERSION, 4):QT += widgets
#QT -= core
#QT -= gui

QMAKE_CXXFLAGS += -msse4.2
QMAKE_CFLAGS += -msse4.2


win32{
    LIBS += -lcrypto++
}else {
    macx{
        #QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.12
        LIBS += -isysroot$(DEVELOPER_SDK_DIR)/MacOSX$(MACOSX_DEPLOYMENT_TARGET).sdk

        OPEN_SSL_ROOT_PATH = $$(OPENSSL_ROOT_DIR)
        equals(OPEN_SSL_ROOT_PATH, ""): OPEN_SSL_ROOT_PATH = /usr/local/opt/openssl

        CRIPTOPP_ROOT_PATH = $$(CRIPTOPP_ROOT_DIR)
        equals(CRIPTOPP_ROOT_PATH, ""): CRIPTOPP_ROOT_PATH = /usr/local/opt/cryptopp
        #CRIPTOPP_ROOT_PATH = $$CALLER_PATH/cryptopp/$$CRIPTOPP_VERSION

        #INCLUDEPATH += $$CALLER_PATH/openssl/1.0.2j/include
        INCLUDEPATH += $$OPEN_SSL_ROOT_PATH/include
        INCLUDEPATH += $$CRIPTOPP_ROOT_PATH/include
        LIBS += -L$$CRIPTOPP_ROOT_PATH/lib
        LIBS += -L$$OPEN_SSL_ROOT_PATH/lib
        LIBS += -lcryptopp
    }else{
        LIBS += -lcrypto++
    } # else of macx
} # else of win32


INCLUDEPATH += $$DECENT_LIB/contrib/json/src

LIBS += -lcrypto



