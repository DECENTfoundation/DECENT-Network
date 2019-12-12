add_executable( delayed_node
                delayed_node_plugin.cpp
                delayed_node_plugin.hpp
                main.cpp
              )

target_link_libraries( delayed_node PRIVATE
                       graphene_app
                       graphene_account_history
                       graphene_transaction_history
                       ${CMAKE_DL_LIBS}
                       ${PLATFORM_SPECIFIC_LIBS}
                     )

if(MSVC)
  set_source_files_properties( delayed_node_plugin.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

#install( TARGETS
#   delayed_node
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
