if( TARGET pbc )
    return()
endif()

unset( GMP_LIBRARIES CACHE )
unset( GMP_LIBRARIES )
unset( GMP_INCLUDE_DIR CACHE )
unset( GMP_INCLUDE_DIR )
unset( GMP_LIB_DIR CACHE )
unset( GMP_LIB_DIR )
unset( PBC_LIBRARIES CACHE )
unset( PBC_LIBRARIES )
unset( PBC_INCLUDE_DIR CACHE )
unset( PBC_INCLUDE_DIR )

# Locate GMP.
#message( STATUS "Looking for GMP" )
if( WIN32 )
  set( GMP_INCLUDE_DIR $ENV{GENERATEDGMP})
  set( GMP_LIBRARIES $ENV{GENERATEDGMP})
else()
   find_library( GMP_LIBRARIES NAMES "gmp" )
   find_path( GMP_INCLUDE_DIR "gmp.h" )
endif()

if( NOT GMP_LIBRARIES )
   message( FATAL_ERROR "GMP library not found" )
endif()
if( NOT GMP_INCLUDE_DIR )
   message( FATAL_ERROR "GMP includes not found" )
endif()


get_filename_component( GMP_LIB_DIR "${GMP_LIBRARIES}" DIRECTORY )

message( STATUS "Looking for PBC" )
if( WIN32 )
   find_library( PBC_LIBRARIES NAMES "pbclib.lib" )
   find_path( PBC_INCLUDE_DIR "pbc/include/pbc.h")
else()
   find_library( PBC_LIBRARIES NAMES "libpbc.a" )
   find_path( PBC_INCLUDE_DIR "pbc/pbc.h")
endif()

if( NOT PBC_LIBRARIES OR NOT PBC_INCLUDE_DIR )
    if( WIN32 )

        set( PBC_INCLUDE_DIR $ENV{PBCROOT}include )
        set( PBC_LIBRARIES $ENV{PBCROOT}pbcwin/lib/x64/Debug/pbclib.lib )

    else()

        # We need flex and bison to build PBC - check them.

        unset( FLEX_PROGRAM CACHE )
        unset( FLEX_PROGRAM )
        unset( BISON_PROGRAM CACHE )
        unset( BISON_PROGRAM )

#        message( STATUS "Looking for flex")
        find_program( FLEX_PROGRAM NAMES "flex" )
        if( NOT FLEX_PROGRAM )
            message( FATAL_ERROR "flex not found" )
        endif()

#        message( STATUS "Looking for bison" )
        find_program( BISON_PROGRAM NAMES "bison" )
        if( NOT BISON_PROGRAM )
            message( FATAL_ERROR "bison not found" )
        endif()

        include( ExternalProject )

        message( STATUS "Building PBC library" )
        # Download, configure and build PBC.
#        set( PBC_PREFIX "${PROJECT_BINARY_DIR}/libraries/contrib/pbc" )
#        set( PBC_DIR "${PBC_PREFIX}/artifacts/prefix" )
#        ExternalProject_Add( build_pbc
#            PREFIX            "${PBC_PREFIX}"
#            SOURCE_DIR        "${PBC_PREFIX}/source"
#            INSTALL_DIR       "${PBC_DIR}"
#            DOWNLOAD_DIR      "${PBC_PREFIX}/download"
#            TMP_DIR           "${PBC_PREFIX}/temp"
#            STAMP_DIR         "${PBC_PREFIX}/stamp"
#            URL               "https://crypto.stanford.edu/pbc/files/pbc-0.5.14.tar.gz"
#            BUILD_IN_SOURCE   1
#            UPDATE_COMMAND    ""
#            CONFIGURE_COMMAND ./configure LEX=flex YACC=bison\ -y CPPFLAGS=-I${GMP_INCLUDE_DIR} LDFLAGS=-L${GMP_LIB_DIR} --prefix=${PBC_DIR}
#            BUILD_COMMAND     make
#            INSTALL_COMMAND   make install
#        )

        # Build the PBC submodule in-source.
        set( PBC_DIR "${PROJECT_BINARY_DIR}/libraries/contrib/pbc/artifacts/prefix" )
        ExternalProject_Add( build_pbc
            PREFIX            "${PROJECT_SOURCE_DIR}/libraries/contrib/pbc"
            SOURCE_DIR        "${PROJECT_SOURCE_DIR}/libraries/contrib/pbc"
            INSTALL_DIR       "${PBC_DIR}"
            TMP_DIR           "${PROJECT_BINARY_DIR}/libraries/contrib/pbc/temp"
            STAMP_DIR         "${PROJECT_BINARY_DIR}/libraries/contrib/pbc/stamp"
            BUILD_IN_SOURCE   1
            CONFIGURE_COMMAND ./setup && ./configure LEX=flex YACC=bison\ -y CPPFLAGS=-I${GMP_INCLUDE_DIR} LDFLAGS=-L${GMP_LIB_DIR} --prefix=${PBC_DIR}
            BUILD_COMMAND     make
            INSTALL_COMMAND   make install
        )

        set( PBC_LIBRARIES "${PBC_DIR}/lib/libpbc.a" )
        set( PBC_INCLUDE_DIR "${PBC_DIR}/include" )
        file( MAKE_DIRECTORY "${PBC_DIR}/include" )

        message( STATUS "PBC will be taken from ${PBC_LIBRARIES}" )
    endif()

    if( NOT PBC_INCLUDE_DIR )
        message( FATAL_ERROR "PBC includes not found" )
    endif()
    if( NOT PBC_LIBRARIES OR NOT PBC_INCLUDE_DIR )
        message( FATAL_ERROR "PBC not found" )
    endif()
else()
    message( STATUS "PBC found at ${PBC_LIBRARIES}" )
endif()

add_library( pbc UNKNOWN IMPORTED )
set_property( TARGET pbc PROPERTY IMPORTED_LOCATION "${PBC_LIBRARIES}" )
set_property( TARGET pbc PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${PBC_INCLUDE_DIR}" "${GMP_INCLUDE_DIR}" )

if( TARGET build_pbc )
    add_dependencies( pbc build_pbc )
endif()

mark_as_advanced(
    FLEX_PROGRAM
    BISON_PROGRAM
    GMP_LIBRARIES
    GMP_INCLUDE_DIR
    GMP_LIB_DIR
    PBC_LIBRARIES
    PBC_INCLUDE_DIR )
