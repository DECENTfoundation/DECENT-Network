find_package(CURL REQUIRED)

file(GLOB HEADERS "include/graphene/elasticsearch/*.hpp")

add_library( decent_elasticsearch
             adaptor.cpp
             elasticsearch_plugin.cpp
             ${HEADERS}
           )

target_link_libraries( decent_elasticsearch PUBLIC graphene_app PRIVATE ${CURL_LIBRARIES} )
target_include_directories( decent_elasticsearch PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" PRIVATE ${CURL_INCLUDE_DIRS} )

if(MSVC)
  set_source_files_properties(elasticsearch_plugin.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

#install( TARGETS
#   decent_elasticsearch

#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
