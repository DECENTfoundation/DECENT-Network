# File lib_gui_wallet.pro
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

include(../lib_gui_wallet_qt/common_gui_wallet.pri)


INCLUDEPATH += $$DECENT_LIB/wallet/include
INCLUDEPATH += $$DECENT_LIB/app/include
INCLUDEPATH += $$DECENT_LIB/encrypt/include

INCLUDEPATH += $$DECENT_LIB/app/include
INCLUDEPATH += $$DECENT_LIB/chain/include
INCLUDEPATH += $$DECENT_LIB/db/include
INCLUDEPATH += $$DECENT_LIB/plugins/market_history/include
INCLUDEPATH += $$DECENT_LIB/net/include
INCLUDEPATH += $$DECENT_LIB/plugins/debug_witness/include
INCLUDEPATH += $$DECENT_LIB/egenesis/include
INCLUDEPATH += $$DECENT_LIB/utilities/include
INCLUDEPATH += $$DECENT_LIB/wallet/include
INCLUDEPATH += $$DECENT_LIB/contrib/fc/vendor/secp256k1-zkp/include
INCLUDEPATH += $$DECENT_LIB/contrib/fc/vendor/websocketpp
INCLUDEPATH += $$DECENT_LIB/contrib/fc/vendor/secp256k1-zkp


INCLUDEPATH += /usr/local/include
INCLUDEPATH += $$DECENT_LIB/contrib/pbc/include
INCLUDEPATH += $$DECENT_LIB/contrib/pbc
INCLUDEPATH += $$DECENT_LIB/contrib/json/src


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

LIBS += $$DECENT_LIB/wallet/libgraphene_wallet.a
LIBS += $$DECENT_LIB/utilities/libgraphene_utilities.a
LIBS += $$DECENT_LIB/egenesis/libgraphene_egenesis_brief.a
LIBS += $$DECENT_LIB/app/libgraphene_app.a
LIBS += $$DECENT_LIB/chain/libgraphene_chain.a
#LIBS += $$DECENT_LIB/contrib/pbc/.libs/libpbc.a
LIBS += $$DECENT_LIB/wallet/libgraphene_wallet.a
LIBS += $$DECENT_LIB/encrypt/libdecent_encrypt.a
LIBS += $$DECENT_LIB/contrib/fc/libfc_debug.a
LIBS += $$DECENT_LIB/contrib/fc/vendor/secp256k1-zkp/src/project_secp256k1-build/.libs/libsecp256k1.a
LIBS += $$DECENT_LIB/package/libpackage_manager.a
LIBS += $$DECENT_LIB/contrib/libtorrent/libtorrent-rasterbar.a
LIBS += $$DECENT_LIB/contrib/cpp-ipfs-api/libipfs-api.a
LIBS += $$DECENT_LIB/encrypt/libdecent_encrypt.a
LIBS += -lcurl

LIBS += -L$$BOOST_ROOT_QT/lib
# http://askubuntu.com/questions/486006/cannot-find-boost-thread-mt-library
exists( $$BOOST_ROOT_QT/lib/libboost_thread-mt* ) {
    LIBS += -lboost_thread-mt
}else{
    LIBS += -lboost_thread
}

exists( $$BOOST_ROOT_QT/lib/libboost_context-mt* ) {
    LIBS += -lboost_context-mt
}else{
    LIBS += -lboost_context
}
LIBS += -lboost_system
LIBS += -lboost_chrono
LIBS += -lboost_coroutine
LIBS += -lboost_date_time
LIBS += -lboost_filesystem
LIBS += -lboost_iostreams

LIBS += -lgmp
LIBS += -lssl
LIBS += -lz
#LIBS += -lcrypto

SOURCES += ../../../src/ui_wallet/fc_rpc_gui.cpp \
    ../../../src/ui_wallet/ui_wallet_functions.cpp \
    ../../../src/utils/decent_tools_rwlock.cpp
HEADERS += ../../../src/ui_wallet/fc_rpc_gui.hpp \
    ../../../include/unnamedsemaphorelite.hpp \
    ../../../include/ui_wallet_functions.hpp \
    ../../../include/ui_wallet_functions_base.hpp \
    ../../../include/decent_tool_fifo.hpp \
    ../../../include/decent_tool_fifo.tos \
    ../../../include/decent_tools_rwlock.hpp \
    ../../../include/debug_decent_application.h


options = $$find(CONFIG, "ALL_LIBS_FOUND")
count(options, 1) {
    # LIBS +=
}else{
    SOURCES += \
        $$DECENT_LIB/contrib/pbc/ecc/curve.c \
        $$DECENT_LIB/contrib/pbc/misc/extend_printf.c \
        $$DECENT_LIB/contrib/pbc/arith/field.c \
        $$DECENT_LIB/contrib/pbc/arith/multiz.c \
        $$DECENT_LIB/contrib/pbc/misc/darray.c \
        $$DECENT_LIB/contrib/pbc/ecc/pairing.c \
        $$DECENT_LIB/contrib/pbc/misc/utils.c \
        $$DECENT_LIB/contrib/pbc/misc/memory.c \
        $$DECENT_LIB/contrib/pbc/arith/random.c \
        $$DECENT_LIB/contrib/pbc/arith/init_random.c \
        $$DECENT_LIB/contrib/pbc/ecc/param.c \
        $$DECENT_LIB/contrib/pbc/arith/fieldquadratic.c \
        $$DECENT_LIB/contrib/pbc/arith/fp.c \
        $$DECENT_LIB/contrib/pbc/arith/fastfp.c \
        $$DECENT_LIB/contrib/pbc/arith/fasterfp.c \
        $$DECENT_LIB/contrib/pbc/arith/montfp.c \
        $$DECENT_LIB/contrib/pbc/arith/naivefp.c \
        $$DECENT_LIB/contrib/pbc/arith/poly.c \
        $$DECENT_LIB/contrib/pbc/ecc/hilbert.c \
        $$DECENT_LIB/contrib/pbc/ecc/mpc.c \
        $$DECENT_LIB/contrib/pbc/ecc/a_param.c \
        $$DECENT_LIB/contrib/pbc/ecc/d_param.c \
        $$DECENT_LIB/contrib/pbc/ecc/e_param.c \
        $$DECENT_LIB/contrib/pbc/ecc/f_param.c \
        $$DECENT_LIB/contrib/pbc/ecc/g_param.c \
        $$DECENT_LIB/contrib/pbc/ecc/eta_T_3.c \
        $$DECENT_LIB/contrib/pbc/arith/ternary_extension_field.c \
        $$DECENT_LIB/contrib/pbc/misc/symtab.c
}




