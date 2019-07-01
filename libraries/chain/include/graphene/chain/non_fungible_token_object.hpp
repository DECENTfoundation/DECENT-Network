/* (c) 2019 DECENT Services. For details refers to LICENSE.txt */
#pragma once
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/global_fun.hpp>

namespace graphene { namespace chain {

   /**
    *  @brief tracks the parameters of a non fungible token
    *  @ingroup object
    *
    *  All non fungible tokens have a globally unique symbol name that controls how they are traded and an issuer who
    *  has authority over the parameters of the token.
    */
   class non_fungible_token_object : public graphene::db::abstract_object<protocol_ids, non_fungible_token_object_type, non_fungible_token_object>
   {
      public:
         /// Symbol for this token
         string symbol;
         /// Options for this token
         non_fungible_token_options options;
         /// Definitions of data that are assigned to each token instance, fee will be charged proportional to size of definitions
         non_fungible_token_data_definitions definitions;
         /// False when token can not be transfered to other owner
         bool transferable = true;
         /// The number of tokens currently in existence
         uint32_t current_supply = 0;

         static account_id_type get_issuer(const non_fungible_token_object& nft_obj) { return nft_obj.options.issuer; }
   };

   class non_fungible_token_data_object : public graphene::db::abstract_object<protocol_ids, non_fungible_token_data_object_type, non_fungible_token_data_object>
   {
      public:
         /// ID of token which issued this token data
         non_fungible_token_id_type nft_id;
         /// ID of the account this token data belongs to
         account_id_type owner;
         /// The token instance data
         fc::variants data;
   };

   using namespace boost::multi_index;

   struct by_symbol;
   struct by_account;
   typedef multi_index_container<
      non_fungible_token_object,
      indexed_by<
         graphene::db::object_id_index,
         ordered_unique< tag<by_symbol>, member<non_fungible_token_object, string, &non_fungible_token_object::symbol> >,
         ordered_non_unique< tag<by_account>, global_fun<const non_fungible_token_object&, account_id_type, &non_fungible_token_object::get_issuer> >
        >
   > non_fungible_token_object_multi_index_type;

   typedef graphene::db::generic_index<non_fungible_token_object, non_fungible_token_object_multi_index_type> non_fungible_token_index;

   struct by_nft;
   typedef multi_index_container<
      non_fungible_token_data_object,
      indexed_by<
         graphene::db::object_id_index,
         ordered_non_unique< tag<by_nft>, member<non_fungible_token_data_object, non_fungible_token_id_type, &non_fungible_token_data_object::nft_id> >,
         ordered_non_unique< tag<by_account>, member<non_fungible_token_data_object, account_id_type, &non_fungible_token_data_object::owner> >
        >
   > non_fungible_token_data_object_multi_index_type;

   typedef graphene::db::generic_index<non_fungible_token_data_object, non_fungible_token_data_object_multi_index_type> non_fungible_token_data_index;

} } // graphene::chain

FC_REFLECT_DERIVED( graphene::chain::non_fungible_token_object, (graphene::db::object),
                    (symbol)
                    (options)
                    (definitions)
                    (transferable)
                    (current_supply)
                  )

FC_REFLECT_DERIVED( graphene::chain::non_fungible_token_data_object, (graphene::db::object),
                    (nft_id)
                    (owner)
                    (data)
                  )
