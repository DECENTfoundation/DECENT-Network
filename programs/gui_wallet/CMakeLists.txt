
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package( Qt5Core REQUIRED )
find_package( Qt5Widgets REQUIRED )

add_executable( gui_wallet MACOSX_BUNDLE
                src/dgui/browse_content_tab.cpp
                src/dgui/cliwalletdlg.cpp
                src/dgui/decent_gui_contentdlg.cpp
                src/dgui/decent_gui_inguiloopcaller.cpp
                src/dgui/decent_wallet_ui_gui_contentdetailsbase.cpp
                src/dgui/decent_wallet_ui_gui_contentdetailsbougth.cpp
                src/dgui/decent_wallet_ui_gui_contentdetailsgeneral.cpp
                src/dgui/decent_wallet_ui_gui_jsonparserqt.cpp
                src/dgui/decent_wallet_ui_gui_newcheckbox.cpp
                src/dgui/gui_wallet_application.cpp
                src/dgui/gui_wallet_centralwidget.cpp
                src/dgui/gui_wallet_connectdlg.cpp
                src/dgui/gui_wallet_global.cpp
                src/dgui/gui_wallet_mainwindow.cpp
                src/dgui/main_gui_wallet.cpp
                src/dgui/main_window_overview.cpp
                src/dgui/main_window_purchased.cpp
                src/dgui/main_window_transactions.cpp
                src/dgui/main_window_upload.cpp
                src/dgui/overview_tab.cpp
                src/dgui/text_display_dialog.cpp
                src/dgui/transactions_tab.cpp
                src/dgui/purchased_tab.cpp
                src/dgui/upload_tab.cpp
                src/ui_wallet/fc_rpc_gui.cpp
                src/ui_wallet/ui_wallet_functions.cpp
                src/utils/decent_tools_rwlock.cpp
                src/utils/richdialog.cpp

                include/debug_decent_application.h
                include/decent_gui_inguiloopcaller_glb.hpp
                include/decent_tool_fifo.hpp
                include/decent_tool_fifo.tos
                include/richdialog.hpp
                include/ui_wallet_functions.hpp
                include/ui_wallet_functions_base.hpp
                include/unnamedsemaphorelite.hpp
                src/dgui/browse_content_tab.hpp
                src/dgui/cliwalletdlg.hpp
                src/dgui/decent_gui_contentdlg.hpp
                src/dgui/decent_gui_inguiloopcaller.hpp
                src/dgui/decent_wallet_ui_gui_common.tos
                src/dgui/decent_wallet_ui_gui_contentdetailsbase.hpp
                src/dgui/decent_wallet_ui_gui_contentdetailsbougth.hpp
                src/dgui/decent_wallet_ui_gui_contentdetailsgeneral.hpp
                src/dgui/decent_wallet_ui_gui_jsonparserqt.hpp
                src/dgui/decent_wallet_ui_gui_newcheckbox.hpp
                src/dgui/purchased_tab.hpp
                src/dgui/gui_wallet_application.hpp
                src/dgui/gui_wallet_centralwidget.hpp
                src/dgui/gui_wallet_connectdlg.hpp
                src/dgui/gui_wallet_global.hpp
                src/dgui/gui_wallet_mainwindow.hpp
                src/dgui/overview_tab.hpp
                src/dgui/qt_commonheader.hpp
                src/dgui/text_display_dialog.hpp
                src/dgui/transactions_tab.hpp
                src/dgui/upload_tab.hpp
                src/ui_wallet/fc_rpc_gui.hpp

                resources.qrc
             )

target_include_directories( gui_wallet PUBLIC
                            include
                          )

target_link_libraries( gui_wallet PUBLIC
                       Qt5::Core Qt5::Widgets graphene_app graphene_net graphene_chain graphene_egenesis_brief graphene_utilities graphene_wallet package_manager decent_encrypt pbc gmp fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}
                     )

install( TARGETS
   gui_wallet

   RUNTIME DESTINATION bin
   BUNDLE DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

install(DIRECTORY images DESTINATION .)
