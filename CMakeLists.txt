if (POLICY CMP0025)
    # Compiler id for Apple Clang is now AppleClang
    cmake_policy(SET CMP0025 NEW)
endif()

if (POLICY CMP0048)
    # The ``project()`` command manages VERSION variables.
    cmake_policy(SET CMP0048 NEW)
endif()

cmake_minimum_required( VERSION 3.10.2 FATAL_ERROR )
project( Decent LANGUAGES CXX )

set( BLOCKCHAIN_NAME "Decent" )

if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
    if( CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9.2 )
        message( FATAL_ERROR "GCC version must be at least 4.9.2!" )
    endif()
elseif( CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" )
    if( CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.5 )
        message( FATAL_ERROR "Clang version must be at least 3.5!" )
    endif()
endif()

# Compile in C++11 mode
set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_CXX_STANDARD_REQUIRED YES )

list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules" )

if( MSVC )
    option( USE_PCH "Use precompiled header" ON )
else()
    option( USE_PCH "Use precompiled header" OFF )
endif()

if( APPLE )
    find_program( BREW_PROGRAM brew )
    if( BREW_PROGRAM )
        find_program( SED_PROGRAM sed )
        find_program( BASH_PROGRAM bash )
        if( BASH_PROGRAM AND SED_PROGRAM )
            execute_process( COMMAND "${BASH_PROGRAM}" "-c" "${BREW_PROGRAM} config | ${SED_PROGRAM} -n -E 's/^HOMEBREW_PREFIX: (.+)$$/\\1/p'" OUTPUT_VARIABLE HOMEBREW_PREFIX )
            string( STRIP "${HOMEBREW_PREFIX}" HOMEBREW_PREFIX )
        else()
            set( HOMEBREW_PREFIX "/usr/local" )
        endif()

        list( APPEND CMAKE_PREFIX_PATH
              "${HOMEBREW_PREFIX}/opt/openssl@1.1"
              "${HOMEBREW_PREFIX}/opt/openssl"
              "${HOMEBREW_PREFIX}/opt/boost"
              "${HOMEBREW_PREFIX}/opt/pbc"
              "${HOMEBREW_PREFIX}/opt/gmp"
              "${HOMEBREW_PREFIX}/opt/cryptopp"
              "${HOMEBREW_PREFIX}/opt/qt5"
            )
    endif()
endif()

set( CMAKE_EXPORT_COMPILE_COMMANDS YES )
set( ENABLE_INSTALLER YES )

include( cotire )

list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libraries/contrib/fc/CMakeModules" )
list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libraries/contrib/fc/GitVersionGen" )
include( GetGitRevisionDescription )
get_git_head_revision( GIT_REFSPEC GIT_SHA2 )

find_package( CryptoPP REQUIRED )

message( STATUS "Looking for GMP" )
find_path( GMP_INCLUDE_DIR "gmp.h" )
find_library( GMP_LIBRARIES NAMES "gmp" "mpir.lib" )

if( NOT GMP_INCLUDE_DIR )
   message( FATAL_ERROR "GMP includes not found" )
endif()
if( NOT GMP_LIBRARIES )
   message( FATAL_ERROR "GMP library not found" )
endif()

mark_as_advanced( GMP_LIBRARIES GMP_INCLUDE_DIR )
message( STATUS "GMP found at ${GMP_INCLUDE_DIR}, ${GMP_LIBRARIES}" )

add_library( gmp UNKNOWN IMPORTED )
set_property( TARGET gmp PROPERTY IMPORTED_LOCATION "${GMP_LIBRARIES}" )
set_property( TARGET gmp PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${GMP_INCLUDE_DIR}" )

message( STATUS "Looking for PBC" )
find_path( PBC_INCLUDE_DIR NAMES "pbc.h" "pbc/pbc.h" )
find_library( PBC_LIBRARIES NAMES "pbc" "pbclib.lib" )

if( NOT PBC_INCLUDE_DIR )
   message( FATAL_ERROR "PBC includes not found" )
endif()
if( NOT PBC_LIBRARIES )
   message( FATAL_ERROR "PBC library not found" )
endif()

mark_as_advanced( PBC_LIBRARIES PBC_INCLUDE_DIR )
message( STATUS "PBC found at ${PBC_INCLUDE_DIR}, ${PBC_LIBRARIES}" )

add_library( pbc UNKNOWN IMPORTED )
set_property( TARGET pbc PROPERTY IMPORTED_LOCATION "${PBC_LIBRARIES}" )
set_property( TARGET pbc PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${PBC_INCLUDE_DIR}" )

if( WIN32 )

    message( STATUS "Configuring Decent on WIN32" )

    set( ZLIB_LIBRARIES "" )
    set( DEFAULT_EXECUTABLE_INSTALL_DIR "bin/" )

    set( CRYPTO_LIB )

    # Looks like this flag can have different default on some machines.
    set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO" )
    set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO" )

    # Probably cmake has a bug and vcxproj generated for executable in Debug conf. has disabled debug info
    set( CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG" )

    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DCRYPTOPP_IMPORTS)

else() # Apple and Linux

    find_library( READLINE_LIBRARIES NAMES readline )
    find_path( READLINE_INCLUDE_DIR "readline/readline.h" )
    if( NOT READLINE_INCLUDE_DIR OR NOT READLINE_LIBRARIES )
        message(FATAL_ERROR "Readline not found")
    endif()

    if( APPLE )
        # Apple Specific Options Here
        message( STATUS "Configuring Decent on OS X" )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -Wall" )
    else()
        # Linux Specific Options Here
        message( STATUS "Configuring Decent on Linux" )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall" )
        set( rt_library rt )
        set( pthread_library pthread )
        if( NOT DEFINED crypto_library )
            # I'm not sure why this is here, I guess someone has openssl and can't detect it with find_package()?
            # if you have a normal install, you can define crypto_library to the empty string to avoid a build error
            set( crypto_library crypto )
        endif()
        if( FULL_STATIC_BUILD )
            set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc" )
        endif()
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

set( ENABLE_COVERAGE_TESTING FALSE CACHE BOOL "Build Decent for code coverage analysis" )

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

SET(BOOST_COMPONENTS thread filesystem program_options serialization unit_test_framework coroutine iostreams)
SET(Boost_USE_STATIC_LIBS ON CACHE STRING "ON or OFF")

find_package(Boost 1.65.1 REQUIRED COMPONENTS ${BOOST_COMPONENTS})

add_subdirectory( libraries )
add_subdirectory( programs )

set(DECENT_NETWORK_TESTS true)

if(DECENT_NETWORK_TESTS)
    add_subdirectory( tests )
endif()


if( ENABLE_INSTALLER )

    set( VERSION_MAJOR 0 )
    set( VERSION_MINOR 1 )
    set( VERSION_PATCH 0 )

    include( InstallRequiredSystemLibraries )

    if( CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT )
        set( CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/artifacts/prefix" CACHE PATH "..." FORCE )
    else()
    	set( CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/artifacts/prefix" )
    endif()
    set( CPACK_OUTPUT_FILE_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/artifacts/packages" )
    set( CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/artifacts/package" )
    set( CPACK_PACKAGE_NAME "Decent" )
    set( CPACK_PACKAGE_VENDOR "Decent Services" )
    set( CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}" )
    set( CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}" )
    set( CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}" )
    set( CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}" )
    set( CPACK_PACKAGE_DESCRIPTION "A client for the Decent network" )
    set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "A client for the Decent network" )
    set( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md" )
    set( CPACK_PACKAGE_INSTALL_DIRECTORY "Decent ${CPACK_PACKAGE_VERSION}" )

    if( WIN32 )
        set( CPACK_GENERATOR "ZIP;NSIS" )
        set( CPACK_PACKAGE_NAME "Decent" ) # override above
        set( CPACK_NSIS_EXECUTABLES_DIRECTORY "." )
        set( CPACK_NSIS_PACKAGE_NAME "Decent v${CPACK_PACKAGE_VERSION}" )
        set( CPACK_NSIS_DISPLAY_NAME "${CPACK_NSIS_PACKAGE_NAME}" )
        set( CPACK_NSIS_DEFINES "  !define MUI_STARTMENUPAGE_DEFAULTFOLDER \\\"Decent\\\"" )
        # It seems like windows zip files usually don't have a single directory inside them, unix tgz frequently do.
        set( CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0 )
    elseif( APPLE )
        set( CPACK_GENERATOR "DragNDrop;TGZ" )
    elseif( LINUX )
        # Linux gets a .tgz
        set( CPACK_GENERATOR "TGZ" )
        set( CPACK_INCLUDE_TOPLEVEL_DIRECTORY 1 )
    endif()

    include( CPack )
endif()
