file(GLOB HEADERS "include/graphene/chain/*.hpp" "include/graphene/chain/protocol/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( graphene_chain

             db_balance.cpp
             db_block.cpp
             db_getter.cpp
             db_init.cpp
             db_maint.cpp
             db_management.cpp
             db_decent.cpp
             db_update.cpp
             db_miner_schedule.cpp
             fork_database.cpp

             protocol/types.cpp
             protocol/authority.cpp
             protocol/asset.cpp
             protocol/assert.cpp
             protocol/account.cpp
             protocol/transfer.cpp
             protocol/miner.cpp
             protocol/proposal.cpp
             protocol/withdraw_permission.cpp
             protocol/asset_ops.cpp
             protocol/memo.cpp
             protocol/custom.cpp
             protocol/operations.cpp
             protocol/transaction.cpp
             protocol/block.cpp
             protocol/fee_schedule.cpp
             protocol/non_fungible_token.cpp
             protocol/vote.cpp
             protocol/decent.cpp
             protocol/subscription.cpp

             genesis_state.cpp
             get_config.cpp

             pts_address.cpp

             evaluator.cpp
             account_evaluator.cpp
             assert_evaluator.cpp
             miner_evaluator.cpp
             asset_evaluator.cpp
             transfer_evaluator.cpp
             proposal_evaluator.cpp
        vesting_balance_evaluator.cpp
             withdraw_permission_evaluator.cpp
        decent_evaluator.cpp
             subscription_evaluator.cpp
             custom_evaluator.cpp
             non_fungible_token_evaluator.cpp

             account_object.cpp
             asset_object.cpp
             content_object.cpp
             proposal_object.cpp
             vesting_balance_object.cpp
             transaction_detail_object.cpp
             message_object.cpp

             block_database.cpp

             ${HEADERS}
           )

target_link_libraries( graphene_chain PUBLIC graphene_utilities PRIVATE graphene_db decent_encrypt )

target_include_directories( graphene_chain PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(MSVC)
  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_database.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

#install( TARGETS
#   graphene_chain
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
