file( GLOB HEADERS "include/decent/wallet_utility/*.hpp" )

add_library( wallet_utility
             wallet_utility.cpp
             ${HEADERS} )

target_link_libraries( wallet_utility
        PRIVATE graphene_db graphene_chain graphene_net graphene_app graphene_wallet package_manager decent_encrypt
)

target_include_directories( wallet_utility
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(MSVC)
   set_source_files_properties( wallet_utility.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

#install( TARGETS
#   package_manager
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
