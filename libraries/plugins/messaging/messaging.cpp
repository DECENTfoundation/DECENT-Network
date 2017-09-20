#include <graphene/messaging/messaging.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/db/index.hpp>
#include <graphene/chain/message_object.hpp>


using namespace decent::messaging;
using namespace graphene::chain;


namespace bpo = boost::program_options;

void messaging_plugin::plugin_set_program_options(
   boost::program_options::options_description& command_line_options,
   boost::program_options::options_description& config_file_options)
{
   command_line_options.add_options()
      //("cmd_line_option", function), "cmd_line_option description")
      ;
   config_file_options.add(command_line_options);
}

std::string messaging_plugin::plugin_name()const
{
   return "messaging";
}

void messaging_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
   try {
      ilog("messaging plugin:  plugin_initialize() begin");
      database().add_index< graphene::db::primary_index < message_index > >();
      _options = &options;

      ilog("messaging plugin:  plugin_initialize() end");
   } FC_LOG_AND_RETHROW()
}

void messaging_plugin::plugin_startup()
{
   try {
      ilog("messaging plugin:  plugin_startup() begin");
      graphene::chain::database& d = database();
      
      graphene::chain::custom_evaluator::register_callback(graphene::chain::custom_operation_subtype_messaging, dynamic_cast<custom_operation_interpreter*>(this));
      
      ilog("messaging plugin:  plugin_startup() end");
   } FC_CAPTURE_AND_RETHROW()
}

void messaging_plugin::plugin_shutdown()
{
   return;
}

void_result messaging_plugin::do_evaluate(const custom_operation& o) 
{ 
   try {
      graphene::chain::message_payload pl;
      o.get_payload(pl);
      FC_ASSERT(pl.from == o.payer, "Invalid sender in custom operation payload.");
      return void_result(); 
   } FC_CAPTURE_AND_RETHROW((o))
};

void_result messaging_plugin::do_apply(const custom_operation& o) 
{ 
   auto & d = database();

   database().create<message_object>([&o, &d](message_object& obj)
   {
      message_payload pl;

      ((graphene::chain::custom_operation&)o).get_payload(pl);
      obj.sender  = pl.from;
      obj.receiver = pl.to;
      obj.created = d.head_block_time();
      obj.sender_pubkey = pl.pub_from;
      obj.receiver_pubkey = pl.pub_to;
      obj.nonce = pl.nonce;
      obj.data = pl.data;
      
   });
   return void_result(); 
};
