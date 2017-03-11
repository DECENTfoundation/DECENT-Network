
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package( Qt5Core REQUIRED )
find_package( Qt5Widgets REQUIRED )

set(MACOSX_BUNDLE_ICON_FILE desktop)
set(MACOSX_BUNDLE_BUNDLE_NAME DECENT)

set(GUI_WALLET_ICON ${CMAKE_CURRENT_SOURCE_DIR}/desktop.icns)
set_source_files_properties(${GUI_WALLET_ICON} PROPERTIES
       MACOSX_PACKAGE_LOCATION "Resources")


add_executable( DECENT MACOSX_BUNDLE
                ${GUI_WALLET_ICON}
                src/dgui/browse_content_tab.cpp
                src/dgui/cliwalletdlg.cpp
                src/dgui/decent_gui_contentdlg.cpp 
                src/dgui/decent_gui_inguiloopcaller.cpp
                src/dgui/decent_wallet_ui_gui_contentdetailsbase.cpp
                src/dgui/decent_wallet_ui_gui_contentdetailsgeneral.cpp
                src/dgui/decent_wallet_ui_gui_jsonparserqt.cpp
                src/dgui/decent_wallet_ui_gui_newcheckbox.cpp
                src/dgui/decent_button.cpp
                src/dgui/gui_wallet_application.cpp
                src/dgui/gui_wallet_centralwidget.cpp
                src/dgui/gui_wallet_connectdlg.cpp
                src/dgui/gui_wallet_global.cpp
                src/dgui/gui_wallet_mainwindow.cpp
                src/dgui/main_gui_wallet.cpp
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
                src/dgui/decent_wallet_ui_gui_contentdetailsgeneral.hpp
                src/dgui/decent_wallet_ui_gui_jsonparserqt.hpp
                src/dgui/decent_wallet_ui_gui_newcheckbox.hpp
                src/dgui/decent_button.hpp
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
                desktop.icns
             )



target_include_directories( DECENT PUBLIC
                            include
                          )

target_link_libraries( DECENT PUBLIC
                       Qt5::Core Qt5::Widgets graphene_app graphene_net graphene_chain  graphene_utilities graphene_wallet package_manager decent_encrypt pbc gmp graphene_account_history graphene_market_history decent_seeding graphene_witness graphene_debug_witness graphene_egenesis_decent fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}
                     )

set_target_properties(DECENT PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "DECENT")



install( TARGETS
   DECENT

   RUNTIME DESTINATION bin
   BUNDLE DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
