/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace graphene { namespace chain {
   class database;

   /**
    * @class account_statistics_object
    * @ingroup object
    * @ingroup implementation
    *
    * This object contains regularly updated statistical data about an account. It is provided for the purpose of
    * separating the account data that changes frequently from the account data that is mostly static, which will
    * minimize the amount of data that must be backed up as part of the undo history everytime a transfer is made.
    */
   class account_statistics_object : public graphene::db::abstract_object<implementation_ids, impl_account_statistics_object_type, account_statistics_object>
   {
      public:
         account_id_type  owner;

         /**
          * Keep the most recent operation as a root pointer to a linked list of the transaction history.
          */
         account_transaction_history_id_type most_recent_op;
         uint32_t                            total_ops = 0;

         /**
          * When calculating votes it is necessary to know how much is stored in orders (and thus unavailable for
          * transfers). Rather than maintaining an index of [asset,owner,order_id] we will simply maintain the running
          * total here and update it every time an order is created or modified.
          */
         share_type total_core_in_orders;


         /**
          * Tracks the fees paid by this account which have not been disseminated to the various parties that receive
          * them yet (registrar, referrer, lifetime referrer, network, etc). This is used as an optimization to avoid
          * doing massive amounts of uint128 arithmetic on each and every operation.
          *
          * These fees will be paid out as vesting cash-back, and this counter will reset during the maintenance
          * interval.
          */
         share_type pending_fees;
         /**
          * Same as @ref pending_fees, except these fees will be paid out as pre-vested cash-back (immediately
          * available for withdrawal) rather than requiring the normal vesting period.
          */
         share_type pending_vested_fees;
   };

   /**
    * @brief Tracks the balance of a single account/asset pair
    * @ingroup object
    *
    * This object is indexed on owner and asset_type so that black swan
    * events in asset_type can be processed quickly.
    */
   class account_balance_object : public graphene::db::abstract_object<implementation_ids, impl_account_balance_object_type, account_balance_object>
   {
      public:
         account_id_type   owner;
         asset_id_type     asset_type;
         share_type        balance;

         asset get_balance()const { return asset(balance, asset_type); }
         void  adjust_balance(const asset& delta);
   };


   /**
    * @brief This class represents an account on the object graph
    * @ingroup object
    * @ingroup protocol
    *
    * Accounts are the primary unit of authority on the graphene system. Users must have an account in order to use
    * assets, trade in the markets, vote for committee_members, etc.
    */
   class account_object : public graphene::db::abstract_object<protocol_ids, account_object_type, account_object>
   {
      public:
         ///The account that paid the fee to register this account. Receives a percentage of referral rewards.
         account_id_type registrar;

         /// The account's name. This name must be unique among all account names on the graph. May not be empty.
         string name;

         /**
          * The owner authority represents absolute control over the account. Usually the keys in this authority will
          * be kept in cold storage, as they should not be needed very often and compromise of these keys constitutes
          * complete and irrevocable loss of the account. Generally the only time the owner authority is required is to
          * update the active authority.
          */
         authority owner;
         /// The owner authority contains the hot keys of the account. This authority has control over nearly all
         /// operations the account may perform.
         authority active;

         account_options options;
         publishing_rights rights_to_publish;

         /// The reference implementation records the account's statistics in a separate object. This field contains the
         /// ID of that object.
         account_statistics_id_type statistics;

         /**
          * Vesting balance which receives cashback_reward deposits.
          */
         optional<vesting_balance_id_type> cashback_vb;

         /**
          * This flag is set when the top_n logic sets both authorities,
          * and gets reset when authority is set.
          */
         uint8_t top_n_control_flags = 0;
         static const uint8_t top_n_control_owner  = 1;
         static const uint8_t top_n_control_active = 2;

         template<typename DB>
         const vesting_balance_object& cashback_balance(const DB& db)const
         {
            FC_ASSERT(cashback_vb);
            return db.get(*cashback_vb);
         }
         bool is_publishing_manager() const { return rights_to_publish.is_publishing_manager; }

   };

   /**
    *  @brief This secondary index will allow a reverse lookup of all accounts that a particular key or account
    *  is an potential signing authority.
    */
   class account_member_index : public graphene::db::secondary_index
   {
      public:
         virtual void object_inserted( const graphene::db::object& obj ) override;
         virtual void object_removed( const graphene::db::object& obj ) override;
         virtual void about_to_modify( const graphene::db::object& before ) override;
         virtual void object_modified( const graphene::db::object& after  ) override;


         /** given an account or key, map it to the set of accounts that reference it in an active or owner authority */
         map< account_id_type, set<account_id_type> > account_to_account_memberships;
         map< public_key_type, set<account_id_type> > account_to_key_memberships;
         /** some accounts use address authorities in the genesis block */
         

      protected:
         set<account_id_type>  get_account_members( const account_object& a )const;
         set<public_key_type>  get_key_members( const account_object& a )const;
   

         set<account_id_type>  before_account_members;
         set<public_key_type>  before_key_members;
   };

   using namespace boost::multi_index;

   struct by_account_asset;
   struct by_asset_balance;
   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      account_balance_object,
      indexed_by<
         graphene::db::object_id_index,
         ordered_unique< tag<by_account_asset>,
            composite_key<
               account_balance_object,
               member<account_balance_object, account_id_type, &account_balance_object::owner>,
               member<account_balance_object, asset_id_type, &account_balance_object::asset_type>
            >
         >,
         ordered_unique< tag<by_asset_balance>,
            composite_key<
               account_balance_object,
               member<account_balance_object, asset_id_type, &account_balance_object::asset_type>,
               member<account_balance_object, share_type, &account_balance_object::balance>,
               member<account_balance_object, account_id_type, &account_balance_object::owner>
            >,
            composite_key_compare<
               std::less< asset_id_type >,
               std::greater< share_type >,
               std::less< account_id_type >
            >
         >
      >
   > account_balance_object_multi_index_type;

   /**
    * @ingroup object_index
    */
   typedef graphene::db::generic_index<account_balance_object, account_balance_object_multi_index_type> account_balance_index;


   struct by_name;
   struct by_publishing_manager_and_name;

   template <typename TAG, typename _t_object>
   struct key_extractor;

   template <>
   struct key_extractor<graphene::db::by_id, account_object>
   {
      static graphene::db::object_id_type get(account_object const& ob)
      {
         return ob.id;
      }
   };

   template <>
   struct key_extractor<by_name, account_object>
   {
      static std::string get(account_object const& ob)
      {
         return ob.name;
      }
   };

   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      account_object,
      indexed_by<
         graphene::db::object_id_index,
         ordered_unique< tag<by_name>, member<account_object, std::string, &account_object::name>  >,
         ordered_unique< tag< by_publishing_manager_and_name>,
            composite_key< account_object,
               const_mem_fun<account_object, bool, &account_object::is_publishing_manager>,
               member<account_object, string, &account_object::name>
            >
         >
      >
   > account_multi_index_type;

   /**
    * @ingroup object_index
    */
   typedef graphene::db::generic_index<account_object, account_multi_index_type> account_index;

}}

FC_REFLECT_DERIVED( graphene::chain::account_object,
                    (graphene::db::object),
                    (registrar)
                    (name)(owner)(active)(options)(rights_to_publish)(statistics)
                    (cashback_vb)(top_n_control_flags)
                    )

FC_REFLECT_DERIVED( graphene::chain::account_balance_object,
                    (graphene::db::object),
                    (owner)(asset_type)(balance) )

FC_REFLECT_DERIVED( graphene::chain::account_statistics_object,
                    (graphene::db::object),
                    (owner)
                    (most_recent_op)
                    (total_ops)
                    (total_core_in_orders)
                    (pending_fees)
                    (pending_vested_fees)
                  )
