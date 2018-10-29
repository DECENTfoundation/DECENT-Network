/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#include <graphene/app/plugin.hpp>
#include <graphene/chain/custom_evaluator.hpp>
#include <graphene/chain/database.hpp>


namespace decent {
   namespace messaging {


      class messaging_plugin : public graphene::app::plugin, graphene::chain::custom_operation_interpreter {
      public:
         messaging_plugin(graphene::app::application* app) : graphene::app::plugin(app) {}
         virtual ~messaging_plugin() {}

         static std::string plugin_name();

         static void plugin_set_program_options(
            boost::program_options::options_description &command_line_options,
            boost::program_options::options_description &config_file_options
         );

         virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
         virtual void plugin_startup() override;
         virtual void plugin_shutdown() override;

         graphene::chain::void_result do_evaluate(const graphene::chain::custom_operation& o) override;
         graphene::chain::void_result do_apply(const graphene::chain::custom_operation& o) override;

      private:
         boost::program_options::variables_map _options;
      };

   }
} //graphene::messaging
