file(GLOB HEADERS "include/decent/monitoring/*.hpp")

add_library(decent_monitoring
            monitoring.cpp
            monitoring_fc.cpp
            ${HEADERS}
)

target_link_libraries( decent_monitoring PRIVATE graphene_utilities )

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
