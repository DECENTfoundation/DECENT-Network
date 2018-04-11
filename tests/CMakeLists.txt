file(GLOB COMMON_SOURCES "common/*.cpp")

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling tests with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

set(TEST_FILES_LIST
    common/database_fixture.hpp
    common/database_fixture.cpp
    common/tempdir.hpp
    common/tempdir.cpp
#    tests/basic_tests.cpp
#    tests/block_tests.cpp
#    tests/database_tests.cpp
#    tests/serialization_tests.cpp
#    tests/operation_tests.cpp
#    tests/operation_tests2.cpp
#    tests/authority_tests.cpp
#    tests/fee_tests.cpp
    tests/uia_tests.cpp
    tests/messaging_tests.cpp
    tests/main.cpp
)

add_executable( chain_test ${TEST_FILES_LIST} )
target_link_libraries( chain_test graphene_chain graphene_app graphene_account_history graphene_egenesis_none decent_messaging fc ${PLATFORM_SPECIFIC_LIBS} )
if(MSVC)
  set_source_files_properties( tests/serialization_tests.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

#/////////////////////////////////////////////////////////////////////

set(APP_TEST_FILES
    common/tempdir.hpp
    common/tempdir.cpp
    app/main.cpp
)

add_executable( app_test ${APP_TEST_FILES} )
target_link_libraries( app_test graphene_app graphene_account_history graphene_net graphene_chain graphene_time graphene_egenesis_none fc ${PLATFORM_SPECIFIC_LIBS} )

#/////////////////////////////////////////////////////////////////////

set(PACKAGE_TEST_FILES
#        common/tempdir.hpp
#        common/tempdir.cpp
    package/main.cpp
)

add_executable( package_test ${PACKAGE_TEST_FILES} )
target_link_libraries( package_test package_manager   graphene_app graphene_account_history graphene_net graphene_chain graphene_time graphene_egenesis_none fc ${PLATFORM_SPECIFIC_LIBS} )



#
#file(GLOB PERFORMANCE_TESTS "performance/*.cpp")
#add_executable( performance_test ${PERFORMANCE_TESTS} ${COMMON_SOURCES} )
#target_link_libraries( performance_test graphene_chain graphene_app graphene_account_history graphene_egenesis_none fc ${PLATFORM_SPECIFIC_LIBS} )
#
#file(GLOB BENCH_MARKS "benchmarks/*.cpp")
#add_executable( chain_bench ${BENCH_MARKS} ${COMMON_SOURCES} )
#target_link_libraries( chain_bench graphene_chain graphene_app graphene_account_history graphene_time graphene_egenesis_none fc ${PLATFORM_SPECIFIC_LIBS} )
#
#file(GLOB INTENSE_SOURCES "intense/*.cpp")
#add_executable( intense_test ${INTENSE_SOURCES} ${COMMON_SOURCES} )
#target_link_libraries( intense_test graphene_chain graphene_app graphene_account_history graphene_egenesis_none fc ${PLATFORM_SPECIFIC_LIBS} )
#
#add_subdirectory( generate_empty_blocks )
