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

target_link_libraries( graphene_app graphene_chain fc graphene_db graphene_net graphene_time graphene_utilities nlohmann_json package_manager )
target_include_directories( graphene_app PUBLIC
                            "${CMAKE_CURRENT_SOURCE_DIR}/include"
                            "${CMAKE_CURRENT_SOURCE_DIR}/../egenesis/include"
                            "${CMAKE_CURRENT_SOURCE_DIR}/../monitoring/include"
                            "${CMAKE_CURRENT_SOURCE_DIR}/../package/include"
 )

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
