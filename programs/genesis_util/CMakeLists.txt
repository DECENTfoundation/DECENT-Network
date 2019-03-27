add_executable( embed_genesis embed_genesis.cpp )

target_link_libraries( embed_genesis PRIVATE graphene_chain graphene_db graphene_app graphene_egenesis_none decent_encrypt )

set( embed_genesis_args
   -t "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_brief.cpp.tmpl---${CMAKE_CURRENT_BINARY_DIR}/egenesis_brief.cpp"
   -t "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_full.cpp.tmpl---${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp"
   )

if( GRAPHENE_EGENESIS_JSON )
   MESSAGE( STATUS "egenesis: " ${GRAPHENE_EGENESIS_JSON} )
   list( APPEND embed_genesis_args --genesis-json "${GRAPHENE_EGENESIS_JSON}" )
else( GRAPHENE_EGENESIS_JSON )
   MESSAGE( STATUS "egenesis: none" )
endif( GRAPHENE_EGENESIS_JSON )

MESSAGE( STATUS "embed_genesis_args: " ${embed_genesis_args} )

add_custom_command(
   OUTPUT
      "${CMAKE_CURRENT_BINARY_DIR}/egenesis_brief.cpp"
      "${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp"
   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
   COMMAND embed_genesis ${embed_genesis_args}
   DEPENDS
      "${GRAPHENE_EGENESIS_JSON}"
      "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_brief.cpp.tmpl"
      "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_full.cpp.tmpl"
      embed_genesis )

add_library( graphene_egenesis_brief "${CMAKE_CURRENT_BINARY_DIR}/egenesis_brief.cpp" ${HEADERS} )
add_library( graphene_egenesis_full  "${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp" ${HEADERS} )

target_link_libraries( graphene_egenesis_brief PRIVATE graphene_chain graphene_db decent_encrypt )
target_link_libraries( graphene_egenesis_full PRIVATE graphene_chain graphene_db decent_encrypt )

target_include_directories( graphene_egenesis_brief
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
target_include_directories( graphene_egenesis_full
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_executable( genesis_update genesis_update.cpp )

target_link_libraries( genesis_update
                       PRIVATE graphene_app graphene_chain graphene_egenesis_none fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   genesis_update

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

add_executable( get_dev_key get_dev_key.cpp )

target_link_libraries( get_dev_key
                       PRIVATE graphene_app graphene_chain graphene_egenesis_none fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

#install( TARGETS
#   embed_genesis graphene_egenesis_brief graphene_egenesis_full get_dev_key
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
