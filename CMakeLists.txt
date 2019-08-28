if (POLICY CMP0025)
    # Compiler id for Apple Clang is now AppleClang
    cmake_policy(SET CMP0025 NEW)
endif()

if (POLICY CMP0048)
    # The ``project()`` command manages VERSION variables.
    cmake_policy(SET CMP0048 NEW)
endif()

cmake_minimum_required( VERSION 3.10.2 FATAL_ERROR )
project( DCore VERSION 1.4.2 LANGUAGES CXX )

if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
    if( CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.1 )
        message( FATAL_ERROR "GCC version must be at least 5.1" )
    endif()
elseif( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
    if( CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.5 )
        message( FATAL_ERROR "Clang version must be at least 3.5" )
    endif()
elseif( CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" )
    if( CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.10 )
        message( FATAL_ERROR "MSVC version must be at least 19.10" )
    endif()
endif()

# Compile in C++14 mode
set( CMAKE_CXX_STANDARD 14 )
set( CMAKE_CXX_STANDARD_REQUIRED YES )
set( CMAKE_EXPORT_COMPILE_COMMANDS YES )

list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules" )
include( cotire )

list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libraries/contrib/fc/GitVersionGen" )
include( GetGitRevisionDescription )
get_git_head_revision( GIT_REFSPEC GIT_SHA2 )

if( MSVC )
    option( USE_PCH "Use precompiled header" ON )
else()
    option( USE_PCH "Use precompiled header" OFF )
endif()

if( WIN32 )
    message( STATUS "Configuring DCore on WIN32" )
    add_compile_options( /D_WIN32_WINNT=0x0601 /D_CRT_SECURE_NO_WARNINGS )

    if( "${VCPKG_TARGET_TRIPLET}" MATCHES "windows-static" )
        foreach( flag_var FLAGS FLAGS_DEBUG FLAGS_RELEASE FLAGS_MINSIZEREL FLAGS_RELWITHDEBINFO )
            set( c_flag_var "CMAKE_C_${flag_var}" )
            if(${c_flag_var} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${c_flag_var} "${${c_flag_var}}")
            endif()

            set( cxx_flag_var "CMAKE_CXX_${flag_var}" )
            if(${cxx_flag_var} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${cxx_flag_var} "${${cxx_flag_var}}")
            endif()
        endforeach(flag_var)
    endif()
else() # Apple and Linux
    if( APPLE )
        # Apple Specific Options Here
        message( STATUS "Configuring DCore on OS X" )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -Wall" )

        find_program( BREW_PROGRAM brew )
        if( BREW_PROGRAM )
            execute_process( COMMAND "${BREW_PROGRAM}" "--prefix" OUTPUT_VARIABLE HOMEBREW_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE )
            message( STATUS "Homebrew prefix: ${HOMEBREW_PREFIX}")
            list( APPEND CMAKE_PREFIX_PATH
                  "${HOMEBREW_PREFIX}/opt/openssl"
                  "${HOMEBREW_PREFIX}/opt/qt"
                )
        endif()
    else()
        # Linux Specific Options Here
        message( STATUS "Configuring DCore on Linux" )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall" )
    endif()

    if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-memcmp" )
        if( NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6 )
            set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive" )
        endif()
    endif()

    if( CMAKE_GENERATOR STREQUAL "Ninja" )
        if( CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" )
            set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics" )
        endif()
    endif()

    # Based on http://www.delorie.com/gnu/docs/gdb/gdb_70.html
    # uncomment this line to tell GDB about macros (slows compile times)
#    set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gdwarf-2 -g3" )
endif()

set( ENABLE_COVERAGE_TESTING FALSE CACHE BOOL "Build DCore for code coverage analysis" )

if( ENABLE_COVERAGE_TESTING )
    set( CMAKE_CXX_FLAGS "--coverage ${CMAKE_CXX_FLAGS}" )
endif()

if(UNIX)
    set(CMAKE_SKIP_RPATH FALSE CACHE INTERNAL "")
    set(CMAKE_SKIP_BUILD_RPATH FALSE CACHE INTERNAL "")
    set(CMAKE_SKIP_INSTALL_RPATH FALSE CACHE INTERNAL "")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE CACHE INTERNAL "")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE CACHE INTERNAL "")
#   set(CMAKE_NO_BUILTIN_CHRPATH TRUE)
    if(APPLE)
        set(CMAKE_INSTALL_RPATH "@executable_path" CACHE INTERNAL "")
        set(CMAKE_MACOSX_RPATH TRUE CACHE INTERNAL "")
#       set(CMAKE_OSX_DEPLOYMENT_TARGET 10.10 CACHE INTERNAL "")
    else()
        set(CMAKE_INSTALL_RPATH "\$ORIGIN" CACHE INTERNAL "")
    endif()
endif()

if( VCPKG_TARGET_TRIPLET )
    message( STATUS "Using static Crypto++" )
    find_package( CryptoPP REQUIRED )

    # vcpkg has only Debug and Release
    set( CRYPTOPP_NAME cryptopp-static )
    get_target_property( _CRYPTOPP_LANGUAGES ${CRYPTOPP_NAME} IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE )
    get_target_property( _CRYPTOPP_LIBRARIES ${CRYPTOPP_NAME} IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE )
    get_target_property( _CRYPTOPP_LOCATION ${CRYPTOPP_NAME} IMPORTED_LOCATION_RELEASE )

    set_property( TARGET ${CRYPTOPP_NAME} APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO )
    set_target_properties( ${CRYPTOPP_NAME} PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "${_CRYPTOPP_LANGUAGES}"
        IMPORTED_LINK_INTERFACE_LIBRARIES_RELWITHDEBINFO "${_CRYPTOPP_LIBRARIES}"
        IMPORTED_LOCATION_RELWITHDEBINFO "${_CRYPTOPP_LOCATION}" )

    set_property( TARGET ${CRYPTOPP_NAME} APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL )
    set_target_properties( ${CRYPTOPP_NAME} PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "${_CRYPTOPP_LANGUAGES}"
        IMPORTED_LINK_INTERFACE_LIBRARIES_MINSIZEREL "${_CRYPTOPP_LIBRARIES}"
        IMPORTED_LOCATION_MINSIZEREL "${_CRYPTOPP_LOCATION}" )

    string( TOUPPER "IMPORTED_LOCATION_${CMAKE_BUILD_TYPE}" _CRYPTOPP_LOCATION )
    get_target_property( CRYPTOPP_LIBRARIES ${CRYPTOPP_NAME} ${_CRYPTOPP_LOCATION} )
    get_target_property( CRYPTOPP_INCLUDE_DIR ${CRYPTOPP_NAME} INTERFACE_INCLUDE_DIRECTORIES )
else()
    find_path( CRYPTOPP_INCLUDE_DIR "cryptopp/integer.h" )
    find_library( CRYPTOPP_LIBRARIES NAMES "cryptopp" )

    if( NOT CRYPTOPP_INCLUDE_DIR )
        message( FATAL_ERROR "Crypto++ includes not found")
    endif()
    if( NOT CRYPTOPP_LIBRARIES )
        message( FATAL_ERROR "Crypto++ libraries not found")
    endif()

    set( CRYPTOPP_NAME cryptopp )
    add_library( ${CRYPTOPP_NAME} STATIC IMPORTED )
    set_property( TARGET ${CRYPTOPP_NAME} PROPERTY IMPORTED_LOCATION "${CRYPTOPP_LIBRARIES}" )
    set_property( TARGET ${CRYPTOPP_NAME} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CRYPTOPP_INCLUDE_DIR}" )
endif()

mark_as_advanced( CRYPTOPP_LIBRARIES CRYPTOPP_INCLUDE_DIR )
message( STATUS "Found Crypto++: ${CRYPTOPP_INCLUDE_DIR}, ${CRYPTOPP_LIBRARIES}" )

find_path( GMP_INCLUDE_DIR "gmp.h" )
find_library( GMP_LIBRARIES NAMES "gmp" "mpir.lib" )

if( NOT GMP_INCLUDE_DIR )
   message( FATAL_ERROR "GMP includes not found" )
endif()
if( NOT GMP_LIBRARIES )
   message( FATAL_ERROR "GMP library not found" )
endif()

mark_as_advanced( GMP_LIBRARIES GMP_INCLUDE_DIR )
message( STATUS "Found GMP: ${GMP_INCLUDE_DIR}, ${GMP_LIBRARIES}" )

add_library( gmp UNKNOWN IMPORTED )
set_property( TARGET gmp PROPERTY IMPORTED_LOCATION "${GMP_LIBRARIES}" )
set_property( TARGET gmp PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${GMP_INCLUDE_DIR}" )

find_path( PBC_INCLUDE_DIR NAMES "pbc.h" "pbc/pbc.h" )
find_library( PBC_LIBRARIES NAMES "pbc" "pbclib.lib" )

if( NOT PBC_INCLUDE_DIR )
   message( FATAL_ERROR "PBC includes not found" )
endif()
if( NOT PBC_LIBRARIES )
   message( FATAL_ERROR "PBC library not found" )
endif()

mark_as_advanced( PBC_LIBRARIES PBC_INCLUDE_DIR )
message( STATUS "Found PBC: ${PBC_INCLUDE_DIR}, ${PBC_LIBRARIES}" )

add_library( pbc UNKNOWN IMPORTED )
set_property( TARGET pbc PROPERTY IMPORTED_LOCATION "${PBC_LIBRARIES}" )
set_property( TARGET pbc PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${PBC_INCLUDE_DIR}" )

add_subdirectory( libraries )
add_subdirectory( programs )
add_subdirectory( tests )
