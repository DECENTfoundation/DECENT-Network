# File gui_wallet.pro
#
# File created : 28 Nov 2016
# Created by   : Davit Kalantaryan (davit.kalantaryan@desy.de)
#
# This file can be used to produce Makefile for gui_wallet GUI application
# for Decent
#

DEFINES += USE_NUM_GMP
DEFINES += USE_FIELD_10X26
DEFINES += USE_FIELD_INV_BUILTIN
DEFINES += USE_SCALAR_4X64
DEFINES += USE_SCALAR_INV_BUILTIN
DEFINES += HAVE___INT128

win32{
    SYSTEM_PATH = ../../../sys/win64
}
else {
    macx{
        SYSTEM_PATH = ../../../sys/mac
    }
    else{
        CODENAME = $$system(lsb_release -c | cut -f 2)
        SYSTEM_PATH = ../../../sys/$$CODENAME
    }
}

# Debug:DESTDIR = debug1
DESTDIR = $$SYSTEM_PATH/bin
OBJECTS_DIR = $$SYSTEM_PATH/.objects
CONFIG += debug
CONFIG += c++11
#greaterThan(QT_MAJOR_VERSION, 4):QT += widgets
QT -= core
QT -= gui
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-variable
QMAKE_CXXFLAGS_WARN_ON += -Wno-sign-compare
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-function
QMAKE_CXXFLAGS_WARN_ON -= -Wunused-function

QMAKE_CXXFLAGS += -msse4.2
QMAKE_CFLAGS += -msse4.2

#BOOST_ROOT= /doocs/develop/kalantar/programs/cpp/works/.private/opt/boost_1_57_0
BOOST_ROOT= ../../../../../opt/boost_1_57_0_unix
BOOST_ROOT_QT= $$BOOST_ROOT

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

LIBS += -L$$BOOST_ROOT/lib
LIBS += -lboost_program_options
LIBS += -lboost_filesystem
LIBS += -lboost_system
LIBS += -lboost_chrono
LIBS += -lboost_date_time
LIBS += -lboost_coroutine
LIBS += -lssl
LIBS += -lcrypto++
LIBS += -lcrypto
LIBS += -lz
LIBS += -lgmp

#INCLUDEPATH += /doocs/develop/kalantar/programs/cpp/works/.private/opt/boost_1_57_0/include
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
INCLUDEPATH += ../../../../DECENT-Network/libraries/encrypt/include

#disabled sources
#SOURCES += ../../../../DECENT-Network/libraries/fc/src/crypto/elliptic_mixed.cpp
#../../../../DECENT-Network/libraries/fc/src/crypto/elliptic_impl_pub.cpp
#../../../../DECENT-Network/libraries/utilities/git_revision.cpp
#../../../../DECENT-Network/libraries/utilities/git_revision.cpp
# ../../../../DECENT-Network/libraries/chain/special_authority.cpp  this file is not there anymore ?

SOURCES += \
    ../../../../DECENT-Network/programs/cli_wallet/main.cpp \
    ../../../../DECENT-Network/libraries/fc/src/log/logger.cpp \
    ../../../../DECENT-Network/libraries/fc/src/variant_object.cpp \
    ../../../../DECENT-Network/libraries/fc/src/log/log_message.cpp \
    ../../../../DECENT-Network/libraries/fc/src/rpc/http_api.cpp \
    ../../../../DECENT-Network/libraries/fc/src/filesystem.cpp \
    ../../../../DECENT-Network/libraries/fc/src/log/file_appender.cpp \
    ../../../../DECENT-Network/libraries/fc/src/variant.cpp \
    ../../../../DECENT-Network/libraries/fc/src/crypto/sha256.cpp \
    ../../../../DECENT-Network/libraries/fc/src/crypto/pke.cpp \
    ../../../../DECENT-Network/libraries/fc/src/crypto/elliptic_impl_priv.cpp \
    ../../../../DECENT-Network/libraries/utilities/key_conversion.cpp \
    ../../../../DECENT-Network/libraries/chain/protocol/types.cpp \
    ../../../../DECENT-Network/libraries/fc/src/io/json.cpp \
\
    ../../../../DECENT-Network/libraries/egenesis/egenesis_brief.cpp \
    ../../../../DECENT-Network/libraries/fc/src/network/http/websocket.cpp \
    ../../../../DECENT-Network/libraries/fc/src/exception.cpp \
\
    ../../../../DECENT-Network/libraries/wallet/wallet.cpp \
    ../../../../DECENT-Network/libraries/fc/src/network/ip.cpp \
    ../../../../DECENT-Network/libraries/fc/src/network/http/http_server.cpp \
\
    ../../../../DECENT-Network/libraries/fc/src/interprocess/signals.cpp \
    ../../../../DECENT-Network/libraries/fc/src/string.cpp \
    ../../../../DECENT-Network/libraries/fc/src/crypto/ripemd160.cpp \
    ../../../../DECENT-Network/libraries/chain/protocol/transaction.cpp \
    ../../../../DECENT-Network/libraries/chain/protocol/fee_schedule.cpp \
\
    ../../../../DECENT-Network/libraries/fc/src/shared_ptr.cpp \
    ../../../../DECENT-Network/libraries/fc/src/thread/future.cpp \
    ../../../../DECENT-Network/libraries/fc/src/time.cpp \
    ../../../../DECENT-Network/libraries/chain/protocol/vote.cpp \
\
    ../../../../DECENT-Network/libraries/fc/src/rpc/websocket_api.cpp \
    ../../../../DECENT-Network/libraries/fc/src/uint128.cpp \
    ../../../../DECENT-Network/libraries/fc/src/io/datastream.cpp \
\
    ../../../../DECENT-Network/libraries/fc/src/crypto/city.cpp \
    ../../../../DECENT-Network/libraries/fc/src/io/iostream.cpp \
    ../../../../DECENT-Network/libraries/fc/src/rpc/cli.cpp \
\
    ../../../../DECENT-Network/libraries/fc/src/thread/spin_lock.cpp \
    ../../../../DECENT-Network/libraries/fc/src/log/logger_config.cpp \
    ../../../../DECENT-Network/libraries/fc/src/thread/thread.cpp \
\
    ../../../../DECENT-Network/libraries/fc/src/rpc/state.cpp \
    ../../../../DECENT-Network/libraries/fc/src/utf8.cpp \
    ../../../../DECENT-Network/libraries/fc/src/io/fstream.cpp \
    ../../../../DECENT-Network/libraries/fc/src/thread/task.cpp \
\
    ../../../../DECENT-Network/libraries/fc/src/crypto/base64.cpp \
    ../../../../DECENT-Network/libraries/fc/src/crypto/hex.cpp \
    ../../../../DECENT-Network/libraries/fc/src/io/sstream.cpp \
\
    ../../../../DECENT-Network/libraries/fc/src/crypto/_digest_common.cpp \
    ../../../../DECENT-Network/libraries/fc/src/crypto/sha1.cpp \
    ../../../../DECENT-Network/libraries/fc/src/crypto/elliptic_common.cpp \
\
    ../../../../DECENT-Network/libraries/fc/vendor/secp256k1-zkp/src/secp256k1.c \
    ../../../../DECENT-Network/libraries/fc/src/crypto/base58.cpp \
    ../../../../DECENT-Network/libraries/fc/src/crypto/bigint.cpp \
    ../../../../DECENT-Network/libraries/fc/src/asio.cpp \
    ../../../../DECENT-Network/libraries/fc/src/crypto/sha512.cpp \
\
    ../../../../DECENT-Network/libraries/chain/protocol/memo.cpp \
    ../../../../DECENT-Network/libraries/utilities/words.cpp \
    ../../../../DECENT-Network/libraries/fc/src/crypto/aes.cpp \
    ../../../../DECENT-Network/libraries/chain/asset_object.cpp \
    ../../../../DECENT-Network/libraries/chain/protocol/confidential.cpp \
\
    ../../../../DECENT-Network/libraries/fc/src/crypto/elliptic_secp256k1.cpp \
    ../../../../DECENT-Network/libraries/chain/protocol/block.cpp \
    ../../../../DECENT-Network/libraries/chain/vesting_balance_object.cpp \
\
    ../../../../DECENT-Network/libraries/wallet/api_documentation.cpp \
    ../../../../DECENT-Network/libraries/fc/src/thread/mutex.cpp \
    ../../../../DECENT-Network/libraries/fc/git_revision.cpp \
    ../../../src/tests/utilites_git_revision.cpp \
    ../../../../DECENT-Network/libraries/chain/protocol/account.cpp \
    ../../../../DECENT-Network/libraries/chain/protocol/asset.cpp \
\
    ../../../../DECENT-Network/libraries/fc/src/network/tcp_socket.cpp \
    ../../../../DECENT-Network/libraries/fc/src/network/http/http_connection.cpp \
    ../../../../DECENT-Network/libraries/chain/protocol/operations.cpp \
    ../../../../DECENT-Network/libraries/chain/protocol/transfer.cpp \
    ../../../../DECENT-Network/libraries/chain/protocol/asset_ops.cpp \
    ../../../../DECENT-Network/libraries/chain/protocol/proposal.cpp \
    ../../../../DECENT-Network/libraries/chain/protocol/withdraw_permission.cpp \
    ../../../../DECENT-Network/libraries/chain/protocol/custom.cpp \
    ../../../../DECENT-Network/libraries/chain/protocol/assert.cpp \
\
    ../../../../DECENT-Network/libraries/fc/src/thread/spin_yield_lock.cpp \
    ../../../../DECENT-Network/libraries/fc/src/log/appender.cpp \
    ../../../../DECENT-Network/libraries/fc/src/log/console_appender.cpp \
\
    ../../../../DECENT-Network/libraries/fc/src/crypto/sha224.cpp \
    ../../../../DECENT-Network/libraries/fc/src/crypto/openssl.cpp \
    ../../../../DECENT-Network/libraries/fc/src/network/url.cpp \
\
    ../../../../DECENT-Network/libraries/chain/protocol/market.cpp \
    ../../../../DECENT-Network/libraries/chain/protocol/witness.cpp \
    ../../../../DECENT-Network/libraries/chain/protocol/committee_member.cpp \
\
    ../../../../DECENT-Network/libraries/fc/src/log/gelf_appender.cpp \
    ../../../../DECENT-Network/libraries/db/object_database.cpp \
    ../../../../DECENT-Network/libraries/fc/src/network/resolve.cpp \
    ../../../../DECENT-Network/libraries/fc/src/network/udp_socket.cpp \
    ../../../../DECENT-Network/libraries/fc/src/compress/zlib.cpp \
    ../../../../DECENT-Network/libraries/db/undo_database.cpp


HEADERS += \
    ../../../../DECENT-Network/libraries/fc/include/fc/log/logger.hpp \
    ../../../../DECENT-Network/libraries/fc/src/crypto/_digest_common.hpp \
    ../../../../tests/src/tests/custodyutils.h \
    ../../../../DECENT-Network/libraries/fc/vendor/websocketpp/websocketpp/extensions/permessage_deflate/disabled.hpp
