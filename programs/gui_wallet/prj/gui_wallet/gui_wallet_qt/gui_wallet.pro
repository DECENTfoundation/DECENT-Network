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
# For useing libs: '$qmake "CONFIG+=USE_LIB" gui_wallet.pro'  , then '$make'

include(../lib_gui_wallet_qt/common_gui_wallet.pri)

greaterThan(QT_MAJOR_VERSION, 4):QT += widgets
#QT -= core
#QT -= gui

SOURCES += ../../../src/dgui/main_gui_wallet.cpp

options1 = $$find(CONFIG, "TEST")

count(options1, 1):DEFINES += TEST_SIMPLE_APP
else{

options2 = $$find(CONFIG, "USE_LIB")
count(options2, 1){

    LIBS += -L$$SYSTEM_PATH/bin
    LIBS += -llib_gui_wallet

}else{

    message( "Preparing all object files localy..." )
    include(../lib_gui_wallet_qt/lib_gui_wallet.pri)

} # else of count(options2, 1)

SOURCES += ../../../src/dgui/gui_wallet_mainwindow.cpp \
    ../../../src/dgui/gui_wallet_centralwigdet.cpp \
    ../../../src/dgui/browse_content_tab.cpp \
    ../../../src/dgui/transactions_tab.cpp \
    ../../../src/dgui/upload_tab.cpp \
    ../../../src/dgui/overview_tab.cpp \
    ../../../src/dgui/gui_wallet_global.cpp \
    ../../../src/dgui/gui_wallet_connectdlg.cpp \
    ../../../src/dgui/gui_wallet_application.cpp \
    ../../../src/dgui/text_display_dialog.cpp \
    ../../../src/dgui/walletcontentdlg.cpp \
    ../../../src/utils/richdialog.cpp \
    ../../../src/dgui/cliwalletdlg.cpp \
    ../../../src/dgui/decent_gui_inguiloopcaller.cpp

HEADERS += ../../../src/dgui/gui_wallet_mainwindow.hpp \
    ../../../src/dgui/gui_wallet_centralwigdet.hpp \
    ../../../src/dgui/browse_content_tab.hpp \
    ../../../src/dgui/transactions_tab.hpp \
    ../../../src/dgui/upload_tab.hpp \
    ../../../src/dgui/overview_tab.hpp \
    ../../../src/dgui/gui_wallet_global.hpp \
    ../../../src/dgui/gui_wallet_connectdlg.hpp \
    ../../../include/unnamedsemaphorelite.hpp \
    ../../../src/dgui/gui_wallet_application.hpp \
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
    ../../../include/debug_decent_application.h

} #else of count(options1, 1)
