file(GLOB HEADERS "include/decent/monitoring/*.hpp")

set(CMAKE_VERBOSE_MAKEFILE ON)

#SET(BOOST_COMPONENTS)
#LIST(APPEND BOOST_COMPONENTS date_time system filesystem program_options signals serialization chrono unit_test_framework context locale iostreams)

add_library(decent_monitoring
            monitoring.cpp
            monitoring_fc.cpp
)

FIND_PACKAGE(Boost 1.53 REQUIRED COMPONENTS ${BOOST_COMPONENTS})

target_link_libraries( decent_monitoring fc graphene_utilities)

target_include_directories( decent_monitoring
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

#install( TARGETS
#   graphene_time
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
