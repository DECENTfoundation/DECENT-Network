
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package( Qt5Core REQUIRED )
find_package( Qt5Widgets REQUIRED )
find_package( Qt5Svg REQUIRED )

set(MACOSX_BUNDLE_ICON_FILE desktop)
set(MACOSX_BUNDLE_BUNDLE_NAME DECENT)

set(GUI_WALLET_ICON ${CMAKE_CURRENT_SOURCE_DIR}/desktop.icns)
set_source_files_properties(${GUI_WALLET_ICON} PROPERTIES
       MACOSX_PACKAGE_LOCATION "Resources")


add_executable(DECENT MACOSX_BUNDLE
               ${GUI_WALLET_ICON}
               src/browse_content_tab.cpp
               src/browse_content_tab.hpp
               src/decent_button.cpp
               src/decent_button.hpp
               src/decent_label.cpp
               src/decent_label.hpp
               src/decent_line_edit.cpp
               src/decent_line_edit.hpp
               src/decent_text_edit.cpp
               src/decent_text_edit.hpp
               src/gui_design.hpp
               src/gui_design.cpp
               src/gui_wallet_centralwidget.cpp
               src/gui_wallet_centralwidget.hpp
               src/gui_wallet_global.cpp
               src/gui_wallet_global.hpp
               src/gui_wallet_tabcontentmanager.cpp
               src/gui_wallet_tabcontentmanager.hpp
               src/main_gui_wallet.cpp
               src/mainwindow.cpp
               src/mainwindow.hpp
               src/overview_tab.cpp
               src/overview_tab.hpp
               src/purchased_tab.cpp
               src/purchased_tab.hpp
               src/richdialog.cpp
               src/richdialog.hpp
               src/transactions_tab.cpp
               src/transactions_tab.hpp
               src/upload_popup.cpp
               src/upload_popup.hpp
               src/upload_tab.cpp
               src/upload_tab.hpp

               resources.qrc
               desktop.icns

               stdafx.h
             )



target_include_directories( DECENT PUBLIC
                            include
                            .
                          )

target_link_libraries(  DECENT PUBLIC
                        Qt5::Core Qt5::Widgets Qt5::Svg graphene_app graphene_net graphene_chain  graphene_utilities graphene_wallet wallet_utility package_manager decent_encrypt pbc gmp graphene_account_history decent_seeding graphene_witness graphene_debug_witness graphene_egenesis_decent fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}
                     )

set_target_properties(DECENT PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "DECENT")
set_target_properties(DECENT PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in")



install( TARGETS
   DECENT

   RUNTIME DESTINATION bin
   BUNDLE DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
