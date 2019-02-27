file(GLOB HEADERS "include/graphene/app/*.hpp")

add_library( graphene_app
             api.cpp
             application.cpp
             database_api.cpp
             impacted.cpp
             balance.cpp
             plugin.cpp
             ${HEADERS}
           )

target_link_libraries( graphene_app PRIVATE graphene_chain graphene_db graphene_net graphene_time package_manager decent_encrypt )
target_include_directories( graphene_app PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../egenesis/include" )

if(MSVC)
  set_source_files_properties( application.cpp api.cpp database_api.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

#install( TARGETS
#   graphene_app
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
