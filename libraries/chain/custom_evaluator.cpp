#include <graphene/chain/custom_evaluator.hpp>


namespace graphene { namespace chain {

   std::map<custom_operation_subtype, std::unique_ptr<custom_operation_interpreter> > custom_evaluator::operation_subtypes = custom_evaluator::create_callbacks_map();

void_result custom_evaluator::do_evaluate(const custom_operation& o)
{ 
   return void_result(); 
}

void_result custom_evaluator::do_apply(const custom_operation& o)
{ 
   return void_result(); 
}

void_result custom_operation_interpreter_messaging::do_evaluate(const custom_operation& o) 
{ 
   return void_result(); 
};

void_result custom_operation_interpreter_messaging::do_apply(const custom_operation& o) 
{ 
   return void_result(); 
};


std::map<custom_operation_subtype, std::unique_ptr<custom_operation_interpreter>> create_callbacks_map()
{
   std::map<custom_operation_subtype, std::unique_ptr<custom_operation_interpreter>> m;
   m[custom_operation_subtype::custom_operation_subtype_messaging] = std::unique_ptr<custom_operation_interpreter>(new custom_operation_interpreter_messaging);

   return m;
}

} } // graphene::chain