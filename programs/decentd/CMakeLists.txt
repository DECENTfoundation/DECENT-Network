add_executable( decentd main.cpp )
if( WIN32 )
   target_sources( decentd PRIVATE winsvc.cpp winsvc.hpp )
endif()

target_link_libraries( decentd PRIVATE
                       graphene_app
                       graphene_account_history
                       graphene_transaction_history
                       decent_seeding
                       decent_elasticsearch
                       graphene_miner
                       ${CMAKE_DL_LIBS}
                       ${PLATFORM_SPECIFIC_LIBS}
                     )

install( TARGETS
   decentd

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
