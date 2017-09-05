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
      //LOAD_VALUE_SET(options, "miner-id", _miners, chain::miner_id_type)

      ilog("messaging plugin:  plugin_initialize() end");
   } FC_LOG_AND_RETHROW()
}

void messaging_plugin::plugin_startup()
{
   try {
      ilog("messaging plugin:  plugin_startup() begin");
      graphene::chain::database& d = database();

      //graphene::chain::custom_operation o;
      //graphene::chain::operation op = o;
      
      //const std::vector< std::unique_ptr<graphene::chain::op_evaluator> >& evals = d.get_operation_evaluators();
      //int which = op.which();
      //const std::unique_ptr<graphene::chain::op_evaluator>& eval = evals[which];
      
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
   return void_result(); 
};

void_result messaging_plugin::do_apply(const custom_operation& o) 
{ 
   auto & d = database();

   database().create<message_object>([&o, &d](message_object& obj)
   {
      obj.account  = o.from;
      obj.created = d.head_block_time();
   });
   return void_result(); 
};
