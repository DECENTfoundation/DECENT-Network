
add_executable( size_checker main.cpp )
target_link_libraries( size_checker
                       PRIVATE graphene_chain graphene_egenesis_none fc cryptopp ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

#install( TARGETS
#   size_checker
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
