
add_executable( cli_wallet main.cpp )

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling cli_wallet with TCMalloc")
    #list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

target_link_libraries( cli_wallet
        PRIVATE decent_monitoring graphene_app graphene_net graphene_chain graphene_egenesis_decent graphene_utilities graphene_wallet package_manager decent_encrypt
        pbc
        fc
        ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}
)

if( WIN32 )
  target_link_libraries( cli_wallet PRIVATE ${GMP_LIBRARIES} )
else()                       
  target_link_libraries( cli_wallet PRIVATE gmp )
endif()                       

if(MSVC)
  set_source_files_properties( main.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   cli_wallet

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
