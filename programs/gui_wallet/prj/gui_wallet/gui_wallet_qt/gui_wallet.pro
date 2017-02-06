# File gui_wallet.pro
#
# File created : 18 Nov 2016
# Created by   : Davit Kalantaryan (davit.kalantaryan@desy.de)
#
# This file can be used to produce Makefile for gui_wallet GUI application
# for Decent
#

#http://doc.qt.io/qt-5/osx-deployment.html
#CONFIG += TEST
# For making test: '$qmake "CONFIG+=TEST" gui_wallet.pro'  , then '$make'

#DEFINES += WALLET_API_DIRECT_CALLS
#DEFINES += LIST_ACCOUNT_BALANCES_DIRECT_CALL

options = $$find(CONFIG, "TEST")

count(options, 1) {

DEFINES += TEST_SIMPLE_APP
SOURCES += ../../../src/dgui/main_gui_wallet.cpp

}else{


#DECENT_ROOT_DEFAULT = ../../../../DECENT-Network
DECENT_ROOT_DEFAULT = ../../../../..
USE_LIB_OR_NOT = not_use_lib

DECENT_ROOT_DEV = $$(DECENT_ROOT)
equals(DECENT_ROOT_DEV, ""): DECENT_ROOT_DEV = $$DECENT_ROOT_DEFAULT

DECENT_LIB = $$DECENT_ROOT_DEV/libraries

BOOST_ROOT_QT = $$(BOOST_ROOT)
#equals(BOOST_ROOT_QT, ""): BOOST_ROOT_QT = /usr/local/opt/boost
#equals(BOOST_ROOT_QT, ""): BOOST_ROOT_QT = ../../../../../opt/boost_1_57_0_unix
equals(BOOST_ROOT_QT, ""){
    exists( /usr/local/opt/boost/libboost_thread* ){
        BOOST_ROOT_QT = /usr/local/opt/boost
    }else{
        BOOST_ROOT_QT = ../../../../../opt/boost_1_57_0_unix
    }
}
message("!!!!!! BOOST_ROOT is '"$$BOOST_ROOT_QT"'")
#message("!!!!!! BOOST_ROOT is" $$(BOOST_ROOT))

QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-variable
QMAKE_CXXFLAGS_WARN_ON += -Wno-sign-compare
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-function
QMAKE_CXXFLAGS_WARN_ON -= -Wunused-function

INCLUDEPATH += $$DECENT_LIB/wallet/include
INCLUDEPATH += $$DECENT_LIB/app/include
INCLUDEPATH += $$DECENT_LIB/encrypt/include

INCLUDEPATH += $$BOOST_ROOT_QT/include
INCLUDEPATH += $$DECENT_LIB/contrib/fc/include
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
INCLUDEPATH += ../../../include

DEFINES += USE_NUM_GMP

win32{
    SYSTEM_PATH = ../../../sys/win64
    LIBS += -lcrypto++
}
else {
    macx{
        #QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.12
        LIBS += -isysroot$(DEVELOPER_SDK_DIR)/MacOSX$(MACOSX_DEPLOYMENT_TARGET).sdk
        SYSTEM_PATH = ../../../sys/mac

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
    }
    else{
        CODENAME = $$system(lsb_release -c | cut -f 2)
        SYSTEM_PATH = ../../../sys/$$CODENAME
        LIBS += -lcrypto++
    }
}

# Debug:DESTDIR = debug1
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

equals(USE_LIB_OR_NOT, "not_use_lib") {

message( "Preparing all object files localy..." )
LIBS += -L$$BOOST_ROOT_QT/lib

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

LIBS += $$DECENT_LIB/encrypt/libdecent_encrypt.a


#LIBS += ../../../../DECENT-Network/libraries/app/libgraphene_app.a
#LIBS += ../../../../DECENT-Network/libraries/chain/libgraphene_chain.a
#LIBS += ../../../../DECENT-Network/libraries/contrib/fc/libfc_debug.a
#LIBS += ../../../../DECENT-Network/libraries/contrib/fc/vendor/secp256k1-zkp/src/project_secp256k1-build/.libs/libsecp256k1.a
#LIBS += ../../../../DECENT-Network/libraries/contrib/fc/vendor/udt4/libudt_debug.a
#LIBS += ../../../../DECENT-Network/libraries/contrib/libtorrent/libtorrent-rasterbar.a
#LIBS += ../../../../DECENT-Network/libraries/db/libgraphene_db.a
#LIBS += ../../../../DECENT-Network/libraries/deterministic_openssl_rand/libdeterministic_openssl_rand.a
#LIBS += ../../../../DECENT-Network/libraries/egenesis/libgraphene_egenesis_brief.a
#LIBS += ../../../../DECENT-Network/libraries/egenesis/libgraphene_egenesis_full.a
#LIBS += ../../../../DECENT-Network/libraries/egenesis/libgraphene_egenesis_none.a
#LIBS += ../../../../DECENT-Network/libraries/encrypt/libdecent_encrypt.a
#LIBS += ../../../../DECENT-Network/libraries/net/libgraphene_net.a
#LIBS += ../../../../DECENT-Network/libraries/package/libpackage_manager.a
#LIBS += ../../../../DECENT-Network/libraries/plugins/account_history/libgraphene_account_history.a
#LIBS += ../../../../DECENT-Network/libraries/plugins/debug_witness/libgraphene_debug_witness.a
#LIBS += ../../../../DECENT-Network/libraries/plugins/delayed_node/libgraphene_delayed_node.a
#LIBS += ../../../../DECENT-Network/libraries/plugins/market_history/libgraphene_market_history.a
#LIBS += ../../../../DECENT-Network/libraries/plugins/seeding/libdecent_seeding.a
#LIBS += ../../../../DECENT-Network/libraries/plugins/witness/libgraphene_witness.a
#LIBS += ../../../../DECENT-Network/libraries/time/libgraphene_time.a
#LIBS += ../../../../DECENT-Network/libraries/utilities/libgraphene_utilities.a
#LIBS += ../../../../DECENT-Network/libraries/wallet/libgraphene_wallet.a

# http://askubuntu.com/questions/486006/cannot-find-boost-thread-mt-library

exists( $$BOOST_ROOT_QT/lib/libboost_thread-mt* ) {
      #message( "Configuring for multi-threaded Qt..." )
      #CONFIG += thread
    LIBS += -lboost_thread-mt
}else{
    LIBS += -lboost_thread
}

exists( $$BOOST_ROOT_QT/lib/libboost_context-mt* ) {
      #message( "Configuring for multi-threaded Qt..." )
      #CONFIG += thread
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

#LIBS += -lgmp
LIBS += -lssl
LIBS += -lz
LIBS += -lcrypto

}else{
LIBS += -L$$SYSTEM_PATH/bin
LIBS += -llib_gui_wallet
}

SOURCES += ../../../src/dgui/main_gui_wallet.cpp \
    ../../../src/dgui/gui_wallet_mainwindow.cpp \
    ../../../src/dgui/gui_wallet_centralwigdet.cpp \
    ../../../src/dgui/browse_content_tab.cpp \
    ../../../src/dgui/transactions_tab.cpp \
    ../../../src/dgui/upload_tab.cpp \
    ../../../src/dgui/overview_tab.cpp \
    ../../../src/dgui/gui_wallet_global.cpp \
    ../../../src/dgui/gui_wallet_connectdlg.cpp \
    ../../../src/ui_wallet/fc_rpc_gui.cpp \
    ../../../src/dgui/gui_wallet_application.cpp
HEADERS += ../../../src/dgui/gui_wallet_mainwindow.hpp \
    ../../../src/dgui/gui_wallet_centralwigdet.hpp \
    ../../../src/dgui/browse_content_tab.hpp \
    ../../../src/dgui/transactions_tab.hpp \
    ../../../src/dgui/upload_tab.hpp \
    ../../../src/dgui/overview_tab.hpp \
    ../../../src/dgui/gui_wallet_global.hpp \
    ../../../src/dgui/gui_wallet_connectdlg.hpp \
    ../../../src/ui_wallet/fc_rpc_gui.hpp \
    ../../../include/unnamedsemaphorelite.hpp \
    ../../../src/dgui/gui_wallet_application.hpp

}

SOURCES += \
    ../../../src/dgui/text_display_dialog.cpp \
    ../../../src/dgui/walletcontentdlg.cpp \
    ../../../src/utils/richdialog.cpp \
    ../../../src/dgui/cliwalletdlg.cpp \
    ../../../src/ui_wallet/ui_wallet_functions.cpp \
    ../../../src/dgui/decent_gui_inguiloopcaller.cpp \
    ../../../src/utils/decent_tools_rwlock.cpp

HEADERS += \
    ../../../src/dgui/text_display_dialog.hpp \
    ../../../src/dgui/walletcontentdlg.hpp \
    ../../../include/richdialog.hpp \
    ../../../src/dgui/cliwalletdlg.hpp \
    ../../../src/dgui/qt_commonheader.hpp \
    ../../../include/ui_wallet_functions.hpp \
    ../../../include/ui_wallet_functions_base.hpp \
    ../../../include/decent_tool_fifo.hpp \
    ../../../include/decent_tool_fifo.tos \
    ../../../include/decent_gui_inguiloopcaller_glb.hpp \
    ../../../src/dgui/decent_gui_inguiloopcaller.hpp \
    ../../../include/decent_tools_rwlock.hpp \
    ../../../include/debug_decent_application.h

INCLUDEPATH += /usr/local/include
INCLUDEPATH += $$DECENT_LIB/contrib/pbc/include
INCLUDEPATH += $$DECENT_LIB/contrib/pbc

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
