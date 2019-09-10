#include <graphene/chain/custom_evaluator.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {

void custom_evaluator::message_payload::get_messaging_payload(const custom_operation& o)
{
   FC_ASSERT(o.data.size());
   fc::variant tmp = fc::json::from_string(std::string(o.data.begin(), o.data.end()));
   fc::from_variant(tmp, *this);
}

void custom_evaluator::message_payload::set_messaging_payload(custom_operation& o) const
{
   fc::variant tmp;
   fc::to_variant(*this, tmp);
   std::string s = fc::json::to_string(tmp);
   o.data = std::vector<char>(s.begin(), s.end());
}

operation_result custom_evaluator::do_evaluate(const operation_type& o)
{
   if (o.id != custom_operation_subtype_messaging)
      return void_result();

   try {
      message_payload pl;
      pl.get_messaging_payload(o);
      const database& d = db();
      const auto& idx = d.get_index_type<account_index>().indices().get<graphene::db::by_id>();
      const auto itr = idx.find(pl.from);
      FC_ASSERT(itr != idx.end(), "Sender ${id} does not exist.", ("id", pl.from));
      for (size_t i = 0; i < pl.receivers_data.size(); i++) {
         const auto itr = idx.find(pl.receivers_data[i].receiver);
         FC_ASSERT(itr != idx.end(), "Receiver ${id} does not exist.", ("id", pl.receivers_data[i].receiver));
      }
      FC_ASSERT(pl.from == o.payer, "Sender must pay for the operation.");
      return void_result();
   } FC_CAPTURE_AND_RETHROW((o))
}

operation_result custom_evaluator::do_apply(const operation_type& o)
{
   if (o.id != custom_operation_subtype_messaging)
      return void_result();

   database &d = db();
   return d.create<message_object>([&o, &d](message_object& obj)
   {
      message_payload pl;
      pl.get_messaging_payload(o);
      obj.created = d.head_block_time();
      obj.sender_pubkey = pl.pub_from;
      obj.sender = pl.from;
      obj.receivers_data = pl.receivers_data;
   }).id;
}

} } // graphene::chain

