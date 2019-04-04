file(GLOB HEADERS "include/graphene/net/*.hpp")

add_library( graphene_net
             node.cpp
             stcp_socket.cpp
             core_messages.cpp
             peer_database.cpp
             peer_connection.cpp
             message_oriented_connection.cpp
             ${HEADERS}
           )

target_link_libraries( graphene_net PUBLIC graphene_chain )
target_include_directories( graphene_net PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(MSVC)
  set_source_files_properties( node.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

#install( TARGETS
#   graphene_net
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
