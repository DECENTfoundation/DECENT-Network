add_executable( decentd main.cpp )
if( WIN32 )
    add_executable( decentd winsvc.cpp winsvc.hpp )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling decentd with TCMalloc")
    #    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

# We have to link against plugins because deficiency in our API infrastructure doesn't allow plugins to be fully abstracted
target_link_libraries( decentd
                       PRIVATE graphene_db graphene_chain graphene_net graphene_app graphene_utilities graphene_account_history graphene_transaction_history decent_seeding decent_messaging graphene_miner package_manager decent_encrypt graphene_egenesis_decent ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
   decentd

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
