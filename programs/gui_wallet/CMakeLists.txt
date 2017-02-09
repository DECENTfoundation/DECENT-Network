
set(CMAKE_AUTOMOC ON)

find_package( Qt5Core REQUIRED )
find_package( Qt5Widgets REQUIRED )

add_executable( gui_wallet
                src/dgui/main_gui_wallet.cpp
                src/dgui/gui_wallet_mainwindow.cpp
                src/dgui/gui_wallet_centralwigdet.cpp
                src/dgui/browse_content_tab.cpp
                src/dgui/transactions_tab.cpp
                src/dgui/upload_tab.cpp
                src/dgui/overview_tab.cpp
                src/dgui/gui_wallet_global.cpp
                src/dgui/gui_wallet_connectdlg.cpp
                src/dgui/fc_rpc_gui.cpp
                src/dgui/gui_wallet_application.cpp
                src/dgui/text_display_dialog.cpp
                src/dgui/walletcontentdlg.cpp

                include/unnamedsemaphorelite.hpp
                src/dgui/gui_wallet_mainwindow.hpp
                src/dgui/gui_wallet_centralwigdet.hpp
                src/dgui/browse_content_tab.hpp
                src/dgui/transactions_tab.hpp
                src/dgui/upload_tab.hpp
                src/dgui/overview_tab.hpp
                src/dgui/gui_wallet_global.hpp
                src/dgui/gui_wallet_connectdlg.hpp
                src/dgui/fc_rpc_gui.hpp
                src/dgui/gui_wallet_application.hpp
                src/dgui/connected_api_instance.hpp
                src/dgui/text_display_dialog.hpp
                src/dgui/walletcontentdlg.hpp
              )

target_include_directories( gui_wallet PUBLIC
                            include
	                      )

target_link_libraries( gui_wallet PUBLIC
                       graphene_app graphene_net graphene_chain graphene_egenesis_brief graphene_utilities graphene_wallet fc Qt5::Core Qt5::Widgets
                     )

install( TARGETS
   gui_wallet

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
