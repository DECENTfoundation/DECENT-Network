file(GLOB HEADERS "include/graphene/miner/*.hpp")

add_library( graphene_miner 
             miner.cpp
             ${HEADERS}
           )

target_link_libraries( graphene_miner PUBLIC graphene_app PRIVATE graphene_utilities )
target_include_directories( graphene_miner PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

#install( TARGETS
#   graphene_miner
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
