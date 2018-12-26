file(GLOB HEADERS "include/graphene/seeding/*.hpp")

add_library( decent_seeding 
             seeding.cpp
             ${HEADERS}
           )

target_link_libraries( decent_seeding PRIVATE graphene_db graphene_chain graphene_net graphene_app graphene_utilities decent_encrypt package_manager )
target_include_directories( decent_seeding PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

#install( TARGETS
#   decent_seeding
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
