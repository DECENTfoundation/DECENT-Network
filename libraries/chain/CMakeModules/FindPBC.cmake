# check all libs and programs

FIND_LIBRARY(GMP_LIBRARIES NAMES gmp )
if(NOT GMP_LIBRARIES)
    message(FATAL_ERROR "gmp library not found")
endif()

find_program(FLEX_IBRARIES NAMES flex)
if(NOT FLEX_IBRARIES)
    message(FATAL_ERROR "FLEX progamm not found")
endif()

find_program(BISON_IBRARIES NAMES bison)
if(NOT BISON_IBRARIES)
    message(FATAL_ERROR "bison progamm not found")
endif()


set (LIBPBC_PATH ${PROJECT_SOURCE_DIR}/libraries/contrib/pbc)

if ( WIN32 )
else ( WIN32 )
    include(ExternalProject)
    ExternalProject_Add( project_pbc
            SOURCE_DIR ${LIBPBC_PATH}
            PREFIX ${LIBPBC_PATH}
            BUILD_IN_SOURCE 1
            CONFIGURE_COMMAND ${LIBPBC_PATH}/setup && ${LIBPBC_PATH}/configure --prefix=${LIBPBC_PATH}
            BUILD_COMMAND make libpbc.la
            INSTALL_COMMAND true
            BUILD_BYPRODUCTS ${LIBPBC_PATH}/.libs/libpbc.a
            )
endif( WIN32 )

add_library(pbc STATIC IMPORTED)
set_property(TARGET pbc PROPERTY IMPORTED_LOCATION ${LIBPBC_PATH}/.libs/libpbc.a)
