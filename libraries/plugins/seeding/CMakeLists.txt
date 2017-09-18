
file(GLOB HEADERS "include/graphene/seeding/*.hpp")

add_library( decent_seeding 
             seeding.cpp
             ${HEADERS}
           )

target_link_libraries( decent_seeding graphene_chain graphene_app graphene_time decent_encrypt package_manager fc )
target_include_directories( decent_seeding
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

#install( TARGETS
#   decent_seeding
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
