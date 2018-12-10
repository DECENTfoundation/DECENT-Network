
file( GLOB HEADERS "*.hpp" "include/decent/package/*.hpp" )

add_library( package_manager
             package.cpp
             detail.cpp
             ipfs_transfer.cpp
             local.cpp
             ${HEADERS} )

target_link_libraries( package_manager fc decent_encrypt nlohmann_json ipfs-api graphene_utilities ${GMP_LIBRARIES} )
target_include_directories( package_manager
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

# A test/sanbox dev-only executable.
add_executable( package_manager_sandbox sandbox.cpp )

target_link_libraries( package_manager_sandbox package_manager fc decent_encrypt nlohmann_json ipfs-api graphene_utilities)

#install( TARGETS
#   package_manager
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
