file(GLOB HEADERS "include/graphene/debug_miner/*.hpp")

add_library( graphene_debug_miner
             debug_api.cpp
             debug_miner.cpp
           )

target_link_libraries( graphene_debug_miner graphene_chain graphene_app )
target_include_directories( graphene_debug_miner
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

#install( TARGETS
#   graphene_debug_miner
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
