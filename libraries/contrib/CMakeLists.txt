add_subdirectory( fc )
#add_subdirectory( pbc )
add_subdirectory( libtorrent )
target_include_directories( torrent-rasterbar
	INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libtorrent/include>"
	INTERFACE "$<INSTALL_INTERFACE:include>" )
if( APPLE )
	set_property(TARGET torrent-rasterbar APPEND PROPERTY LINK_FLAGS "-Wl,-exported_symbol,_OPENSSL_ia32cap_P")
endif()
add_subdirectory( json )
set(JSON_FOR_MODERN_CXX_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/json/src" CACHE INTERNAL "" FORCE)
add_subdirectory( cpp-ipfs-api )
