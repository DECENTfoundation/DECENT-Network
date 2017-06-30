add_subdirectory( fc )
add_subdirectory( json )
set(JSON_FOR_MODERN_CXX_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/json/src" CACHE INTERNAL "" FORCE)
add_subdirectory( cpp-ipfs-api )
# add_subdirectory( pbc ) # pbc is still somehow involved in the build process