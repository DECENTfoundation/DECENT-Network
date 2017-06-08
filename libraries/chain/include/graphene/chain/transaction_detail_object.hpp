#pragma once
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/chain/protocol/memo.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

namespace graphene { namespace chain {

   class transaction_detail_object : public abstract_object<transaction_detail_object>
   {
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id  = impl_transaction_detail_object_type;

      enum eOperationType
      {
         transfer,
         account_create,
         content_submit,
         content_buy,
         content_rate
      };
      account_id_type m_from_account;
      account_id_type m_to_account;
      uint8_t m_operation_type;
      asset m_transaction_amount;
      asset m_transaction_fee;
      std::string m_str_description;
      optional<memo_data> m_transaction_encrypted_memo;
      fc::time_point_sec m_timestamp;

      share_type get_transaction_amount() const;
      share_type get_transaction_fee() const;
   };

   struct by_id;
   struct by_from_account;
   struct by_to_account;
   struct by_operation_type;
   struct by_transaction_amount;
   struct by_transaction_fee;
   struct by_description;
   struct by_time;

   template <typename TAG, typename _t_object>
   struct key_extractor;

   template <>
   struct key_extractor<by_from_account, transaction_detail_object>
   {
      static account_id_type get(transaction_detail_object const& ob)
      {
         return ob.m_from_account;
      }
   };

   template <>
   struct key_extractor<by_to_account, transaction_detail_object>
   {
      static account_id_type get(transaction_detail_object const& ob)
      {
         return ob.m_to_account;
      }
   };

   template <>
   struct key_extractor<by_operation_type, transaction_detail_object>
   {
      static uint8_t get(transaction_detail_object const& ob)
      {
         return ob.m_operation_type;
      }
   };

   template <>
   struct key_extractor<by_transaction_amount, transaction_detail_object>
   {
      static share_type get(transaction_detail_object const& ob)
      {
         return ob.get_transaction_amount();
      }
   };

   template <>
   struct key_extractor<by_transaction_fee, transaction_detail_object>
   {
      static share_type get(transaction_detail_object const& ob)
      {
         return ob.get_transaction_fee();
      }
   };

   template <>
   struct key_extractor<by_description, transaction_detail_object>
   {
      static string get(transaction_detail_object const& ob)
      {
         return ob.m_str_description;
      }
   };

   template <>
   struct key_extractor<by_time, transaction_detail_object>
   {
      static fc::time_point_sec get(transaction_detail_object const& ob)
      {
         return ob.m_timestamp;
      }
   };


   typedef multi_index_container<
      transaction_detail_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_non_unique< tag<by_from_account>, member<transaction_detail_object, account_id_type, &transaction_detail_object::m_from_account> >,
         ordered_non_unique< tag<by_to_account>, member<transaction_detail_object, account_id_type, &transaction_detail_object::m_to_account> >,
         ordered_non_unique< tag<by_operation_type>, member<transaction_detail_object, uint8_t, &transaction_detail_object::m_operation_type> >,
         ordered_non_unique< tag<by_transaction_amount>, const_mem_fun<transaction_detail_object, share_type, &transaction_detail_object::get_transaction_amount> >,
         ordered_non_unique< tag<by_transaction_fee>, const_mem_fun<transaction_detail_object, share_type, &transaction_detail_object::get_transaction_fee> >,
         ordered_non_unique< tag<by_description>, member<transaction_detail_object, std::string, &transaction_detail_object::m_str_description> >,
         ordered_non_unique< tag<by_time>, member<transaction_detail_object, fc::time_point_sec, &transaction_detail_object::m_timestamp> >
      >
   > transaction_detail_multi_index_type;

   typedef generic_index<transaction_detail_object, transaction_detail_multi_index_type> transaction_detail_index;
} }

FC_REFLECT_DERIVED( graphene::chain::transaction_detail_object, (graphene::db::object), (m_from_account)(m_to_account)(m_operation_type)(m_transaction_amount)(m_transaction_fee)(m_str_description)(m_transaction_encrypted_memo)(m_timestamp) )
