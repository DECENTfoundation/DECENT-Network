
add_executable( decentd main.cpp )

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling decentd with TCMalloc")
    #    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

# We have to link against plugins because deficiency in our API infrastructure doesn't allow plugins to be fully abstracted
target_link_libraries( decentd
                       PRIVATE graphene_app graphene_account_history graphene_transaction_history decent_seeding decent_messaging decent_monitoring graphene_miner graphene_chain graphene_egenesis_decent fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   decentd

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
