
add_executable( account_generator main.cpp )

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling cli_wallet with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

if( WIN32 )
  target_link_libraries( account_generator
                       PRIVATE graphene_app graphene_net graphene_chain graphene_egenesis_decent graphene_utilities graphene_wallet package_manager decent_encrypt pbc ${GMP_LIBRARIES} fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
else()                    
  target_link_libraries( account_generator
                       PRIVATE graphene_app graphene_net graphene_chain graphene_egenesis_decent graphene_utilities graphene_wallet package_manager decent_encrypt pbc gmp fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
endif()
                  
if(MSVC)
  set_source_files_properties( main.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

#install( TARGETS
#   account_generator
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
