#include <graphene/chain/custom_evaluator.hpp>
#include <graphene/chain/database.hpp>

namespace graphene { namespace chain {

custom_evaluator_register& custom_evaluator_register::instance()
{
   static custom_evaluator_register register_instance;
   return register_instance;
}

void custom_evaluator_register::register_callback(custom_operation_subtype s, custom_operation_interpreter* i)
{
  // lockit
  m_operation_subtypes.insert(std::make_pair(s, i));
}

void custom_evaluator_register::unregister_callback(custom_operation_subtype s)
{
  // lockit
  m_operation_subtypes.erase(s);
}

void custom_evaluator_register::unregister_all()
{
   m_operation_subtypes.clear();
}

custom_operation_interpreter* custom_evaluator_register::find(custom_operation_subtype subtype)
{
   auto iter = m_operation_subtypes.find(subtype);
   if (iter == m_operation_subtypes.end()) {
      // leave it unprocessed
      return nullptr;
   }

   return iter->second;
}

//////////////////////////////////////////////////////////////////////////////

void_result custom_evaluator::do_evaluate(const operation_type& o)
{
   try
   {
      const database& _db = db();
      for( const auto acc : o.required_auths )
         FC_ASSERT( _db.find_object( acc ), "Account does not exist." );

      custom_evaluator_register& instance = custom_evaluator_register::instance();

      custom_operation_interpreter* evaluator = instance.find(static_cast<custom_operation_subtype>(o.id));
      if (!evaluator) {
         // leave it unprocessed
         return void_result();
      }

      return evaluator->do_evaluate(o);

   } FC_CAPTURE_AND_RETHROW((o))
}

graphene::db::object_id_type custom_evaluator::do_apply(const operation_type& o)
{ 
   try
   {
      custom_evaluator_register& instance = custom_evaluator_register::instance();

      custom_operation_interpreter* evaluator = instance.find(static_cast<custom_operation_subtype>(o.id));
      if (!evaluator) {
         // leave it unprocessed
         return graphene::db::object_id_type();
      }

      return evaluator->do_apply(o);

   } FC_CAPTURE_AND_RETHROW((o))
}

} } // graphene::chain
