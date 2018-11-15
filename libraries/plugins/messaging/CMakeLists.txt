file(GLOB HEADERS "include/graphene/messaging/*.hpp")

add_library( decent_messaging 
             messaging.cpp
             ${HEADERS}
           )

target_link_libraries( decent_messaging graphene_chain graphene_app graphene_time decent_encrypt package_manager fc )
target_include_directories( decent_messaging
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

#install( TARGETS
#   decent_messaging
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
