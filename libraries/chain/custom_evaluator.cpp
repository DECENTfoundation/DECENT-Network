#include <graphene/chain/custom_evaluator.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/message_object.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {

operation_result custom_evaluator::do_evaluate(const operation_type& o)
{
   if (o.id != operation_type::custom_operation_subtype_messaging)
      return void_result();

   try {
      message_payload pl;
      o.get_messaging_payload(pl);
      const database& d = db();
      const auto& idx = d.get_index_type<account_index>().indices().get<graphene::db::by_id>();
      const auto itr = idx.find(pl.from);
      FC_ASSERT(itr != idx.end(), "Sender ${id} does not exist.", ("id", pl.from));
      for (size_t i = 0; i < pl.receivers_data.size(); i++) {
         const auto itr = idx.find(pl.receivers_data[i].to);
         FC_ASSERT(itr != idx.end(), "Receiver ${id} does not exist.", ("id", pl.receivers_data[i].to));
      }
      FC_ASSERT(pl.from == o.payer, "Sender must pay for the operation.");
      return void_result();
   } FC_CAPTURE_AND_RETHROW((o))
}

operation_result custom_evaluator::do_apply(const operation_type& o)
{
   if (o.id != operation_type::custom_operation_subtype_messaging)
      return void_result();

   message_payload pl;
   o.get_messaging_payload(pl);
   if(!is_account_tracked(pl.from) && std::all_of(pl.receivers_data.begin(), pl.receivers_data.end(), [](const auto& data) { return !is_account_tracked(data.to); } ))
      return void_result();

   database &d = db();
   return d.create<message_object>([&](message_object& obj)
   {
      obj.created = d.head_block_time();
      obj.sender_pubkey = pl.pub_from;
      obj.sender = pl.from;
      std::for_each(pl.receivers_data.begin(), pl.receivers_data.end(), [&obj](const message_payload_receivers_data& data) {
         obj.receivers_data.push_back({ data.to, data.pub_to, data.nonce, data.data });
      });
   }).id;
}

} } // graphene::chain
