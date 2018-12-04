set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package( Qt5Widgets REQUIRED )

set(MACOSX_BUNDLE_ICON_FILE desktop)
set(MACOSX_BUNDLE_BUNDLE_NAME DECENT)

set(GUI_WALLET_ICON ${CMAKE_CURRENT_SOURCE_DIR}/desktop.icns)
set_source_files_properties(${GUI_WALLET_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

set(GUI_WALLET_RESOURCES resources.qrc)
set(GUI_WALLET_PRECOMPILED_SOURCE_FILE "")
set(GUI_WALLET_PRECOMPILED_HEADER_FILE stdafx.h)
if( WIN32 )
    list(APPEND GUI_WALLET_RESOURCES DECENT.rc)
    list(APPEND GUI_WALLET_PRECOMPILED_SOURCE_FILE stdafx.cpp)
endif()

add_executable(DECENT MACOSX_BUNDLE
               ${GUI_WALLET_PRECOMPILED_SOURCE_FILE} # must be the first file
               ${GUI_WALLET_PRECOMPILED_HEADER_FILE}
               ${GUI_WALLET_RESOURCES}
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
               src/gui_wallet_global.cpp
               src/gui_wallet_global.hpp
               src/gui_wallet_tabcontentmanager.cpp
               src/gui_wallet_tabcontentmanager.hpp
               src/main_gui_wallet.cpp
               src/mainwindow.cpp
               src/mainwindow.hpp
               src/mining_vote_popup.cpp
               src/mining_vote_popup.hpp
               src/mining_vote_tab.cpp
               src/mining_vote_tab.hpp
               src/overview_tab.cpp
               src/overview_tab.hpp
               src/purchased_tab.cpp
               src/purchased_tab.hpp
               src/rev_history_dlg.cpp
               src/rev_history_dlg.hpp
               src/richdialog.cpp
               src/richdialog.hpp
               src/transactions_tab.cpp
               src/transactions_tab.hpp
               src/update_manager.cpp
               src/update_manager.hpp
               src/update_prog_bar.cpp
               src/update_prog_bar.hpp
               src/upload_popup.cpp
               src/upload_popup.hpp
               src/upload_tab.cpp
               src/upload_tab.hpp
             )

if( UPDATE_MANAGER )
add_definitions(-DUPDATE_MANAGER=1)
endif()

if( WIN32 )
  if( UPDATE_MANAGER )
    target_link_libraries(  DECENT PUBLIC
                        Qt5::Widgets graphene_app graphene_net graphene_chain graphene_utilities graphene_wallet wallet_utility decent_messaging decent_monitoring package_manager decent_encrypt decent_config pbc ${GMP_LIBRARIES} graphene_account_history graphene_transaction_history decent_seeding graphene_miner graphene_egenesis_decent fc msi update ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}
                     )
  else()
    target_link_libraries(  DECENT PUBLIC
                        Qt5::Widgets graphene_app graphene_net graphene_chain graphene_utilities graphene_wallet wallet_utility decent_messaging decent_monitoring package_manager decent_encrypt decent_config pbc ${GMP_LIBRARIES} graphene_account_history graphene_transaction_history decent_seeding graphene_miner graphene_egenesis_decent fc msi ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}
                     )
  endif()
else()
  if( UPDATE_MANAGER )
    target_link_libraries(  DECENT PUBLIC
                        Qt5::Widgets graphene_app graphene_net graphene_chain graphene_utilities graphene_wallet wallet_utility decent_messaging decent_monitoring package_manager decent_encrypt decent_config pbc gmp graphene_account_history graphene_transaction_history decent_seeding graphene_miner graphene_egenesis_decent fc update ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}
                     )
  else()
    target_link_libraries(  DECENT PUBLIC
                        Qt5::Widgets graphene_app graphene_net graphene_chain graphene_utilities graphene_wallet wallet_utility decent_messaging decent_monitoring package_manager decent_encrypt decent_config pbc gmp graphene_account_history graphene_transaction_history decent_seeding graphene_miner graphene_egenesis_decent fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}
                     )
  endif()
endif()

set_target_properties(DECENT PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "DECENT")
set_target_properties(DECENT PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in")

if(WIN32 OR APPLE)
    set_target_properties(DECENT PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
    set_target_properties(DECENT PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT ${GUI_WALLET_PRECOMPILED_HEADER_FILE})
    cotire(DECENT)
endif()

install( TARGETS
   DECENT

   RUNTIME DESTINATION bin
   BUNDLE DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
