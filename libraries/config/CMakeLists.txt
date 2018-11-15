file(GLOB HEADERS "include/decent/config/*.hpp")

add_library(decent_config
            decent_log_config.cpp
            ${HEADERS}
)

target_link_libraries( decent_config fc )

target_include_directories( decent_config
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

#install( TARGETS
#   graphene_time
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
