add_executable( delayed_node main.cpp )

target_link_libraries( delayed_node
                       PRIVATE graphene_app graphene_account_history graphene_market_history graphene_delayed_node graphene_chain graphene_egenesis_full fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

#install( TARGETS
#   delayed_node
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
