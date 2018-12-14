
file(GLOB headers "include/graphene/utilities/*.hpp")

set(sources deterministic_openssl_rand.cpp
            ${headers})

add_library( deterministic_openssl_rand 
             ${sources}
             ${HEADERS} )
target_link_libraries( deterministic_openssl_rand fc )
target_include_directories( deterministic_openssl_rand
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
                            "${CMAKE_CURRENT_SOURCE_DIR}/../blockchain/include"
                          )

#install( TARGETS
#   deterministic_openssl_rand
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
