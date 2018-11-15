file(GLOB HEADERS "include/graphene/transaction_history/*.hpp")

add_library( graphene_transaction_history
        transaction_history_plugin.cpp
        ${HEADERS}
        )

target_link_libraries( graphene_transaction_history graphene_chain graphene_app )
target_include_directories( graphene_transaction_history
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(MSVC)
    set_source_files_properties( transaction_history_plugin.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

#install( TARGETS
#   graphene_transaction_history
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
