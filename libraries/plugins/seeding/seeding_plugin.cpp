#include <graphene/plugin/seeding/seeding_plugin.hpp>
#include <graphene/chain/operation_history_object.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace seeding {

namespace detail {

class seeding_plugin_impl {
public:
   seeding_plugin_impl(seeding_plugin &_plugin) : _self(_plugin) {}

   virtual ~seeding_plugin_impl() {}

   graphene::chain::database &database() {
      return _self.database();
   }

   void on_operation(const operation_history_object &op_obj);

   seeding_plugin& _self;
};

seeding_plugin_impl::~seeding_plugin_impl()
{
   return;
}

void seeding_plugin_impl::on_operation(const operation_history_object &op_obj)
{
   graphene::chain::database &db = database();

   if (op_obj.op.which() == operation::tag<request_to_buy_operation>::value)
   {
      const request_to_buy_operation& rtb_op = op_obj.op.get< request_to_buy_operation >();

      deliver_keys_operation dk_op;

      db.create<my_seeding_object>([&](my_seeding_object &so))
      {
         so.URI        = rtb_op.URI ;
         so.consumer   = rtb_op.consumer ;
         so.seeder     = dk_op.seeder ;
         so.proof      = dk_op.proof ;
         so.key        = dk_op.key ;
      }
   }

}


} // end namespace detail


seeding_plugin::seeding_plugin() : my( new detail::seeding_plugin_impl( *this )){}

void seeding_plugin::plugin_initialize( const boost::program_options::variables_map& options )
{
   database().on_applies_operation.connect( [&]( const operation_history_object& b ){ my->on_operation(b); } );
   database().add_index< primary_index < my_seeding_index > >();
}

std::string seeding_plugin::plugin_name()const
{
   return "seeding";
}

void private_message_plugin::plugin_set_program_options(
        boost::program_options::options_description& cli,
        boost::program_options::options_description& cfg)
{
}

}}
