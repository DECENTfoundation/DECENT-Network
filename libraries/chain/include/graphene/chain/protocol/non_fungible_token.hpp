/* (c) 2019 DECENT Services. For details refers to LICENSE.txt */
#pragma once
#include <graphene/db/generic_index.hpp>
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/memo.hpp>

namespace graphene { namespace chain {

   struct non_fungible_token_data_type {
      /// False when owner can issue non data unique token instances
      bool unique = false;

      /// False when owner can not change token instance data
      bool modifiable = false;

      /// Type of data
      enum { string, integer, boolean } type = string;

      /// Data user name
      std::string name;

      /// Perform internal consistency checks.
      /// @throws fc::exception if any check fails
      void validate() const;
   };

   typedef std::vector<non_fungible_token_data_type> non_fungible_token_data_definitions;

   /**
    * @brief The non_fungible_token_options struct contains options available on all non fungible tokens in the network
    * @note Changes to this struct will break protocol compatibility
    */
   struct non_fungible_token_options {
      /// The owner of non fungible token
      account_id_type issuer;

      /// The maximum supply of non fungible token instances which may exist at any given time
      uint32_t max_supply = 0;

      /// String that describes the meaning/purpose of non fungible token
      std::string description;

      /// Perform internal consistency checks.
      /// @throws fc::exception if any check fails
      void validate() const;
   };

   /**
    * @ingroup operations
    * Creates a non fungible token.
    */
   struct non_fungible_token_create_operation : public base_operation<false>
   {
      struct fee_parameters_type {
         uint64_t basic_fee = 1*GRAPHENE_BLOCKCHAIN_PRECISION/1000;
         uint32_t price_per_kbyte = 10;
      };

      asset fee;
      /// The ticker symbol of this token
      std::string symbol;
      /// Options for this token
      non_fungible_token_options options;
      /// Definitions of data that are assigned to each token instance
      non_fungible_token_data_definitions definitions;
      /// False when issuer can change max_supply
      bool fixed_max_supply = false;
      /// False when token can not be transfered to other owner
      bool transferable = true;
      /// Future operation extensions
      extensions_type extensions;

      /// Perform internal consistency checks.
      /// @throws fc::exception if any check fails
      void validate() const;

      account_id_type fee_payer() const { return options.issuer; }
      share_type calculate_fee( const fee_parameters_type& k ) const;
   };

   /**
    * @ingroup operations
    * Updates the non fungible token.
    */
   struct non_fungible_token_update_operation : public base_operation<false>
   {
      struct fee_parameters_type {
         uint64_t fee = 5*GRAPHENE_BLOCKCHAIN_PRECISION/1000;
      };

      asset fee;
      /// The non fungible token to update
      non_fungible_token_id_type nft_id;
      /// Options for this token
      non_fungible_token_options options;
      /// Future operation extensions
      extensions_type extensions;

      /// Perform internal consistency checks.
      /// @throws fc::exception if any check fails
      void validate() const;

      account_id_type fee_payer() const { return options.issuer; }
   };

   /**
    * @ingroup operations
    * Issues a non fungible token instance.
    */
   struct non_fungible_token_issue_operation : public base_operation<false>
   {
      struct fee_parameters_type {
         uint64_t fee = 5*GRAPHENE_BLOCKCHAIN_PRECISION/1000;
         uint32_t price_per_kbyte = 10;
      };

      asset fee;
      /// Must be nft_id->issuer
      account_id_type issuer;
      /// The account that will receive the new token instance
      account_id_type to;
      /// The non fungible token to issue
      non_fungible_token_id_type nft_id;
      /// The token instance data
      fc::variants data;
      /// User provided data encrypted to the memo key of the "to" account
      optional<memo_data> memo;
      /// Future operation extensions
      extensions_type extensions;

      /// Perform internal consistency checks.
      /// @throws fc::exception if any check fails
      void validate() const;

      account_id_type fee_payer() const { return issuer; }
      share_type calculate_fee( const fee_parameters_type& k ) const;
   };

   /**
    * @ingroup operations
    * Transfers the non fungible token instance.
    */
   struct non_fungible_token_transfer_operation : public base_operation<false>
   {
      struct fee_parameters_type {
         uint64_t fee = 5*GRAPHENE_BLOCKCHAIN_PRECISION/1000;
      };

      asset fee;
      /// The account to transfer the token instance from
      account_id_type from;
      /// The account that will receive the token instance
      account_id_type to;
      /// The non fungible token instance to transfer
      non_fungible_token_data_id_type nft_data_id;
      /// User provided data encrypted to the memo key of the "to" account
      optional<memo_data> memo;
      /// Future operation extensions
      extensions_type extensions;

      /// Perform internal consistency checks.
      /// @throws fc::exception if any check fails
      void validate() const;

      account_id_type fee_payer() const { return from; }

      bool is_partner_account_id(account_id_type acc_id) const;
   };

   /**
    * @ingroup operations
    * Changes data of the non fungible token instance.
    */
   struct non_fungible_token_data_operation : public base_operation<false>
   {
      struct fee_parameters_type {
         uint64_t fee = 5*GRAPHENE_BLOCKCHAIN_PRECISION/1000;
         uint32_t price_per_kbyte = 10;
      };

      asset fee;
      /// Must be nft_data_id->owner
      account_id_type owner;
      /// The non fungible token instance to transfer
      non_fungible_token_data_id_type nft_data_id;
      /// Name to value pairs to be updated
      std::unordered_map<std::string, fc::variant> data;
      /// Future operation extensions
      extensions_type extensions;

      /// Perform internal consistency checks.
      /// @throws fc::exception if any check fails
      void validate() const;

      account_id_type fee_payer() const { return owner; }
      share_type calculate_fee( const fee_parameters_type& k ) const;
   };

} } // graphene::chain

FC_REFLECT_TYPENAME( decltype( graphene::chain::non_fungible_token_data_type::string ) )

FC_REFLECT_ENUM( decltype( graphene::chain::non_fungible_token_data_type::string ), (string)(integer)(boolean) )

FC_REFLECT( graphene::chain::non_fungible_token_data_type,
            (unique)
            (modifiable)
            (type)
            (name)
          )

FC_REFLECT( graphene::chain::non_fungible_token_options,
            (issuer)
            (max_supply)
            (description)
          )

FC_REFLECT( graphene::chain::non_fungible_token_create_operation::fee_parameters_type,
            (basic_fee) 
            (price_per_kbyte)
          )

FC_REFLECT( graphene::chain::non_fungible_token_create_operation,
            (fee)
            (symbol)
            (options)
            (definitions)
            (fixed_max_supply)
            (transferable)
            (extensions)
          )

FC_REFLECT( graphene::chain::non_fungible_token_update_operation::fee_parameters_type,
            (fee) 
          )

FC_REFLECT( graphene::chain::non_fungible_token_update_operation,
            (fee)
            (nft_id)
            (options)
            (extensions)
          )

FC_REFLECT( graphene::chain::non_fungible_token_issue_operation::fee_parameters_type,
            (fee) 
            (price_per_kbyte)
          )

FC_REFLECT( graphene::chain::non_fungible_token_issue_operation,
            (fee)
            (issuer)
            (to)
            (nft_id)
            (data)
            (memo)
            (extensions)
          )

FC_REFLECT( graphene::chain::non_fungible_token_transfer_operation::fee_parameters_type,
            (fee) 
          )

FC_REFLECT( graphene::chain::non_fungible_token_transfer_operation,
            (fee)
            (from)
            (to)
            (nft_data_id)
            (memo)
            (extensions)
          )

FC_REFLECT( graphene::chain::non_fungible_token_data_operation::fee_parameters_type,
            (fee) 
            (price_per_kbyte)
          )

FC_REFLECT( graphene::chain::non_fungible_token_data_operation,
            (fee)
            (owner)
            (nft_data_id)
            (data)
            (extensions)
          )
