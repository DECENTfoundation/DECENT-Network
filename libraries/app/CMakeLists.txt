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

target_link_libraries( graphene_app PUBLIC graphene_net PRIVATE package_manager graphene_egenesis_decent )
target_include_directories( graphene_app PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

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
