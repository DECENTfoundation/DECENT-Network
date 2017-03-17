if( TARGET cryptopp )
    return()
endif()

unset( CRYPTOPP_LIBRARIES CACHE )
unset( CRYPTOPP_LIBRARIES )
unset( CRYPTOPP_INCLUDE_DIR CACHE )
unset( CRYPTOPP_INCLUDE_DIR )

message( STATUS "Looking for Crypto++" )

find_library( CRYPTOPP_LIBRARIES NAMES "cryptopp" )
find_path( CRYPTOPP_INCLUDE_DIR "cryptopp/integer.h" )
if( NOT CRYPTOPP_LIBRARIES OR NOT CRYPTOPP_INCLUDE_DIR )
    message( FATAL_ERROR "Crypto++ not found" )
endif()

message( STATUS "Crypto++ found at ${CRYPTOPP_LIBRARIES}" )

add_library( cryptopp UNKNOWN IMPORTED )
set_property( TARGET cryptopp PROPERTY IMPORTED_LOCATION "${CRYPTOPP_LIBRARIES}" )
set_property( TARGET cryptopp PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CRYPTOPP_INCLUDE_DIR}" )

mark_as_advanced(
    CRYPTOPP_LIBRARIES
    CRYPTOPP_INCLUDE_DIR )
