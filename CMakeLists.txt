cmake_minimum_required( VERSION 2.8.12 FATAL_ERROR )

project( Decent )

set( BLOCKCHAIN_NAME "Decent" )

set( CLI_CLIENT_EXECUTABLE_NAME "decent_client" )
set( GUI_CLIENT_EXECUTABLE_NAME "decent" )
set( CUSTOM_URL_SCHEME "gcs" )
set( INSTALLER_APP_ID "68ad7005-8eee-49c9-95ce-9eed97e5b347" )

if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" )
    if( CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5 )
        message( FATAL_ERROR "GCC version must be at least 5!" )
    endif()
elseif( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
    if( CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.5 )
        message( FATAL_ERROR "Clang version must be at least 3.5!" )
    endif()
endif()

set(CMAKE_C_FLAGS_DEBUG "-DDEBUG")


set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_CXX_STANDARD_REQUIRED YES )

list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules" )

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
              "${HOMEBREW_PREFIX}/opt/openssl"
              "${HOMEBREW_PREFIX}/opt/boost@1.60"
              "${HOMEBREW_PREFIX}/opt/pbc"
              "${HOMEBREW_PREFIX}/opt/gmp"
              "${HOMEBREW_PREFIX}/opt/cryptopp"
              "${HOMEBREW_PREFIX}/opt/berkeley-db"
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

if( WIN32 )
    set( BOOST_ROOT $ENV{BOOST_ROOT} )
    set( Boost_USE_MULTITHREADED ON )
    set( BOOST_ALL_DYN_LINK OFF ) # force dynamic linking for all libraries
endif()

set( Boost_USE_STATIC_LIBS ON CACHE STRING "ON or OFF" )

find_package( Boost 1.60 REQUIRED
    COMPONENTS thread
               date_time
               system
               filesystem
               program_options
               signals
               serialization
               chrono
               unit_test_framework
               context
               coroutine
               locale )

find_package( CryptoPP REQUIRED )
find_package( PBC REQUIRED )

if( WIN32 )

    message( STATUS "Configuring Decent on WIN32" )
    set( DB_VERSION 60 )
    set( BDB_STATIC_LIBS 1 )

    set( ZLIB_LIBRARIES "" )
    set( DEFAULT_EXECUTABLE_INSTALL_DIR "bin/" )

    set( CRYPTO_LIB )

    # Looks like this flag can have different default on some machines.
    set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO" )
    set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO" )

    # Probably cmake has a bug and vcxproj generated for executable in Debug conf. has disabled debug info
    set( CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG" )

    # On windows tcl should be installed to the directory pointed by setenv.bat script
    set( TCL_INCLUDE_PATH "$ENV{TCL_ROOT}/include" )
    message( STATUS "tcl INCLUDE PATH: ${TCL_INCLUDE_PATH}" )

    find_package( TCL )
    message( STATUS "tcl_library: ${TCL_LIBRARY}" )

    set( TCL_LIBS "optimized;${TCL_LIBRARY};debug;" )
    get_filename_component( TCL_LIB_PATH "${TCL_LIBRARY}" PATH )
    get_filename_component( TCL_LIB_NAME "${TCL_LIBRARY}" NAME_WE )
    get_filename_component( TCL_LIB_EXT "${TCL_LIBRARY}" EXT )

    set( TCL_LIBS "${TCL_LIBS}${TCL_LIB_PATH}/${TCL_LIB_NAME}g${TCL_LIB_EXT}" )
    set( TCL_LIBRARY ${TCL_LIBS} )
    
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

    if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-memcmp" )
        if( NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6 )
            set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive" )
        endif()
    endif()

    if( "${CMAKE_GENERATOR}" STREQUAL "Ninja" )
        if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
            set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics" )
        endif()
    endif()

    # Based on http://www.delorie.com/gnu/docs/gdb/gdb_70.html
    # uncomment this line to tell GDB about macros (slows compile times)
#    set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gdwarf-2 -g3" )

endif()

find_package( BerkeleyDB )

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


add_subdirectory( libraries )
add_subdirectory( programs )
#add_subdirectory( tests )


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
