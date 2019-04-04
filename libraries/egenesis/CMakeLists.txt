file(GLOB HEADERS "include/graphene/egenesis/*.hpp")

add_library( graphene_egenesis_decent
             egenesis_decent.cpp
             ${HEADERS}
           )

target_link_libraries( graphene_egenesis_decent PRIVATE graphene_chain )
target_include_directories( graphene_egenesis_decent PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_library( graphene_egenesis_none
             egenesis_none.cpp
             ${HEADERS}
           )

target_link_libraries( graphene_egenesis_none PRIVATE graphene_chain )
target_include_directories( graphene_egenesis_none PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

#install( TARGETS
#   graphene_egenesis_decent graphene_egenesis_none
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
