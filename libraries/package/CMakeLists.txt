file( GLOB HEADERS "*.hpp" "include/decent/package/*.hpp" )

add_library( package_manager
             package.cpp
             detail.cpp
             ipfs_check.cpp
             ipfs_transfer.cpp
             local.cpp
             ${HEADERS}
           )

target_link_libraries( package_manager PUBLIC graphene_utilities decent_encrypt PRIVATE ipfs-api )
target_include_directories( package_manager PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

#install( TARGETS
#   package_manager
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
