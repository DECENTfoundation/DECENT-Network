#ifndef STDAFX_APP_H
#include <boost/multiprecision/cpp_int.hpp>
#endif

#include <graphene/app/balance.hpp>
#include <graphene/app/api.hpp>

namespace graphene { namespace app {

using namespace fc;
using namespace graphene::chain;

namespace detail {

    void split_payout_to_coauthors(asset paid_price, account_id_type author,
                                   const map<account_id_type, uint32_t> &co_authors,
                                   flat_map<account_id_type, asset> &result) {
        if (co_authors.empty())
            result.insert(make_pair(author, paid_price));
        else {
            asset price = paid_price;
            boost::multiprecision::int128_t price_for_co_author;
            for (auto const &element : co_authors) {
                price_for_co_author = (paid_price.amount.value * element.second) / 10000ll;
                result.insert(make_pair(element.first,
                                        asset(static_cast<share_type>(price_for_co_author), price.asset_id)));
                price.amount -= price_for_co_author;
            }

            if (price.amount != 0) {
                FC_ASSERT(price.amount > 0);
                auto find = result.find(author);
                if (find != result.end()) {
                    find->second += price;
                } else {
                    result.insert(make_pair(author, price));
                }
            }
        }
    };

} //namespace


struct get_balance_history_visitor
{
   account_id_type _account;
   asset_array& _balance;
   asset& _fee;

   get_balance_history_visitor( account_id_type account, asset_array& balance, asset& fee ) : _account(account), _balance(balance), _fee(fee) {}
   typedef void result_type;

   void operator()( const transfer_operation& op )
   {
       if (op.from == _account) {
           _balance.asset0 = asset(-op.amount.amount, op.amount.asset_id);
           _balance.asset1 = asset();
       }
       else {
           _balance.asset0 = op.amount;
           _balance.asset1 = asset();
       }
       if (op.fee_payer() == _account)
           _fee = op.fee;
   }

   void operator()( const transfer2_operation& op )
   {
       if (op.from == _account) {
           _balance.asset0 = asset(-op.amount.amount, op.amount.asset_id);
           _balance.asset1 = asset();
       }

       if( op.to.is<account_id_type>() && op.to.as<account_id_type>() == _account) {
           _balance.asset0 = op.amount;
           _balance.asset1 = asset();
       }
       if (op.fee_payer() == _account)
           _fee = op.fee;
   }

   void operator()( const asset_issue_operation& op )
   {
       if (op.issue_to_account == _account) {
           _balance.asset0 = op.asset_to_issue;
           _balance.asset1 = asset();
       }
       else {
           _balance.asset0 = _balance.asset1 = asset();
       }
       if (op.fee_payer() == _account)
           _fee = op.fee;
   }

   void operator()( const asset_fund_pools_operation& op )
   {
       if (op.from_account != _account) {
           _balance.asset0 = _balance.asset1 = asset();
           return;
       }

       if( op.uia_asset.amount > 0) {
           _balance.asset0 = asset(-op.uia_asset.amount, op.uia_asset.asset_id);
       }
       if( op.dct_asset.amount > 0) {
           _balance.asset1 = asset(-op.dct_asset.amount, op.dct_asset.asset_id);
       }
       if (op.fee_payer() == _account)
           _fee = op.fee;
   }

   void operator()( const asset_reserve_operation& op )
   {
       if (op.fee_payer() == _account)
           _fee = op.fee;

       if (op.payer != _account) {
           _balance.asset0 = _balance.asset1 = asset();
           return;
       }

       _balance.asset0 = asset(-op.amount_to_reserve.amount, op.amount_to_reserve.asset_id);
       _balance.asset1 = asset();
   }

   void operator()( const asset_claim_fees_operation& op )
   {
       if (op.fee_payer() == _account)
           _fee = op.fee;

       if (op.issuer != _account) {
           _balance.asset0 = _balance.asset1 = asset();
           return;
       }

       if( op.uia_asset.amount > 0) {
           _balance.asset0 = asset(op.uia_asset.amount, op.uia_asset.asset_id);
       }
       if( op.dct_asset.amount > 0) {
           _balance.asset1 = asset(op.dct_asset.amount, op.dct_asset.asset_id);
       }
   }

   void operator()( const vesting_balance_create_operation& op )
   {
       if (op.fee_payer() == _account)
           _fee = op.fee;

       if (op.creator != _account) {
           _balance.asset0 = _balance.asset1 = asset();
           return;
       }

       _balance.asset0 = asset(-op.amount.amount, op.amount.asset_id);
       _balance.asset1 = asset();
   }

   void operator()( const vesting_balance_withdraw_operation& op )
   {
       if (op.fee_payer() == _account)
           _fee = op.fee;

       if (op.owner != _account) {
           _balance.asset0 = _balance.asset1 = asset();
           return;
       }

       _balance.asset0 = op.amount;
       _balance.asset1 = asset();
   }

   void operator()( const withdraw_permission_claim_operation& op )
   {
       if (op.fee_payer() == _account)
           _fee = op.fee;

       if (op.withdraw_from_account == _account) {
           _balance.asset0 = asset(-op.amount_to_withdraw.amount, op.amount_to_withdraw.asset_id);
           _balance.asset1 = asset();
       }
       else if (op.withdraw_to_account == _account) {
           _balance.asset0 = op.amount_to_withdraw;
           _balance.asset1 = asset();
       }
       else {
           _balance.asset0 = _balance.asset1 = asset();
       }
   }

   void operator()( const request_to_buy_operation& op)
   {
       if (op.fee_payer() == _account)
           _fee = op.fee;

       if (op.consumer == _account) {
           _balance.asset0 = -op.price;
           _balance.asset1 = asset();
       }
   }

   void operator()( const return_escrow_buying_operation& op)
   {
       if (op.fee_payer() == _account)
           _fee = op.fee;

       if (op.consumer == _account) {
           _balance.asset0 = op.escrow;
           _balance.asset1 = asset();
       }
   }

   void operator()( const return_escrow_submission_operation& op)
   {
       if (op.fee_payer() == _account)
           _fee = op.fee;

       if (op.author == _account) {
           _balance.asset0 = op.escrow;
           _balance.asset1 = asset();
       }
   }

   void operator()( const content_submit_operation& op)
   {
       if (op.fee_payer() == _account)
           _fee = op.fee;

       if (op.author == _account) {
           _balance.asset0 = -op.publishing_fee;
           _balance.asset1 = asset();
       }
   }

   void operator()( const subscribe_operation& op)
   {
       if (op.fee_payer() == _account)
           _fee = op.fee;

       if (op.from == _account) {
           _balance.asset0 = asset(-op.price.amount, op.price.asset_id);
           _balance.asset1 = asset();
       }
       else if (op.to == _account) {
           _balance.asset0 = op.price;
           _balance.asset1 = asset();
       }
   }

   void operator()( const finish_buying_operation& op)
   {
       //NOTE: consumer balance is already changed in 'request_to_buy_operation'
//     if (fop.consumer == account) {
//        result.a0 = asset(-fop.payout.amount, fop.payout.asset_id);
//        result.a1 = asset();
//     }

       if (op.fee_payer() == _account)
           _fee = op.fee;

       if (op.author == _account && op.co_authors.empty()) {
           _balance.asset0 = op.payout;
           _balance.asset1 = asset();
       }
       else {
           //calculate split to author and co-authors
           flat_map<account_id_type, asset> co_authors_split;
           detail::split_payout_to_coauthors(op.payout, op.author, op.co_authors, co_authors_split);

           auto find = co_authors_split.find(_account);
           if (find != co_authors_split.end()) {
               _balance.asset0 = find->second;
               _balance.asset1 = asset();
           }
       }
   }

   void operator()( const pay_seeder_operation& op)
   {
       if (op.fee_payer() == _account)
           _fee = op.fee;

//NOTE: author balance is changed in 'content_submit_operation'
//     if (psop.author == account) {
//         result.a0 = asset(-psop.payout.amount, psop.payout.asset_id);
//         result.a1 = asset();
//     }
       if (op.seeder == _account) {
           _balance.asset0 = op.payout;
           _balance.asset1 = asset();
       }
   }

   void operator()( const account_create_operation& op ) {}
   void operator()( const account_update_operation& op ) {}
   void operator()( const asset_create_operation& op ) {}
   void operator()( const update_monitored_asset_operation& op ) {}
   void operator()( const update_user_issued_asset_operation& op ) {}
   void operator()( const update_user_issued_asset_advanced_operation& op ) {}
   void operator()( const asset_publish_feed_operation& op ) {}
   void operator()( const miner_create_operation& op ) {}
   void operator()( const miner_update_operation& op ) {}
   void operator()( const miner_update_global_parameters_operation& op ) {}

   void operator()( const proposal_create_operation& op ) {}
   void operator()( const proposal_update_operation& op ) {}
   void operator()( const proposal_delete_operation& op ) {}

   void operator()( const withdraw_permission_create_operation& op ) {}
   void operator()( const withdraw_permission_update_operation& op ) {}
   void operator()( const withdraw_permission_delete_operation& op ) {}

   void operator()( const custom_operation& op ) {}
   void operator()( const assert_operation& op ) {}
   void operator()( const set_publishing_manager_operation& op ) {}
   void operator()( const set_publishing_right_operation& op ) {}
   void operator()( const content_cancellation_operation& op) {}

   void operator()( const leave_rating_and_comment_operation& op) {}
   void operator()( const ready_to_publish_operation& op) {}
   void operator()( const ready_to_publish2_operation& op) {}

   void operator()( const proof_of_custody_operation& op) {}
   void operator()( const deliver_keys_operation& op) {}

   void operator()( const subscribe_by_author_operation& op) {}
   void operator()( const automatic_renewal_of_subscription_operation& op) {}
   void operator()( const disallow_automatic_renewal_of_subscription_operation& op) {}
   void operator()( const renewal_of_subscription_operation& op) {}

   void operator()( const report_stats_operation& op) {}

/////////////////////////////////////////////////////////////////////////////////

};

void operation_get_balance_history( const operation& op, account_id_type account, asset_array& result, asset& fee_result )
{
   get_balance_history_visitor vtor = get_balance_history_visitor(account, result, fee_result );
   op.visit( vtor );
}

} } // graphene::app
