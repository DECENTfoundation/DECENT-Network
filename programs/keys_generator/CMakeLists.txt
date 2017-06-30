
add_executable( keys_generator main.cpp )

if( WIN32 )
  target_link_libraries( keys_generator
                       PRIVATE graphene_net graphene_chain graphene_egenesis_decent graphene_utilities graphene_wallet package_manager decent_encrypt pbc ${GMP_LIBRARIES} fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
else()
  target_link_libraries( keys_generator
                       PRIVATE graphene_utilities graphene_chain fc )
endif()

if(MSVC)
  set_source_files_properties( main.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

#install( TARGETS
#   account_creator
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
