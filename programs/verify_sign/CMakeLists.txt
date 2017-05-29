
add_executable( verify_sign main.cpp )
target_link_libraries( verify_sign
                       PRIVATE graphene_chain fc cryptopp ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
