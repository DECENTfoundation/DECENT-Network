file(GLOB HEADERS "include/graphene/app/*.hpp")

set(GRAPHENE_APP_PRECOMPILED_SOURCE_FILE "")
set(GRAPHENE_APP_PRECOMPILED_HEADER_FILE stdafx.h)
if(WIN32)
    list(APPEND GRAPHENE_APP_PRECOMPILED_SOURCE_FILE stdafx.cpp)
endif()

add_library( graphene_app
             ${GRAPHENE_APP_PRECOMPILED_SOURCE_FILE} # must be the first file
             ${GRAPHENE_APP_PRECOMPILED_HEADER_FILE}
             api.cpp
             application.cpp
             database_api.cpp
             impacted.cpp
             balance.cpp
             plugin.cpp
             ${HEADERS}
           )

target_link_libraries( graphene_app graphene_chain fc graphene_db graphene_net graphene_time graphene_utilities decent_monitoring nlohmann_json package_manager )
target_include_directories( graphene_app PUBLIC
                            "${CMAKE_CURRENT_SOURCE_DIR}/include"
                            "${CMAKE_CURRENT_SOURCE_DIR}/../egenesis/include"
                            "${CMAKE_CURRENT_SOURCE_DIR}/../monitoring/include"
                            "${CMAKE_CURRENT_SOURCE_DIR}/../package/include"
 )

if(MSVC)
  set_source_files_properties( application.cpp api.cpp database_api.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

set_target_properties(graphene_app PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
set_target_properties(graphene_app PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT ${GRAPHENE_APP_PRECOMPILED_HEADER_FILE})
cotire(graphene_app)

#install( TARGETS
#   graphene_app
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
