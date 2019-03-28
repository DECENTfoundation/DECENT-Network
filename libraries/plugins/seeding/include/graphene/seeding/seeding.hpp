/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#include <graphene/app/plugin.hpp>

namespace decent { namespace seeding {

struct seeding_plugin_impl;

class seeding_plugin : public graphene::app::plugin
{
   public:
      seeding_plugin(graphene::app::application* app);
      virtual ~seeding_plugin();

      static std::string plugin_name();

      /**
       * Extend program options with our option list
       * @param cli CLI parameters
       * @param cfg Config file parameters
       */
      static void plugin_set_program_options(
              boost::program_options::options_description& cli,
              boost::program_options::options_description& cfg);

      /**
       * Initialize plugin based on config parameters
       * @param options
       */
      void plugin_initialize(const boost::program_options::variables_map& options) override;
      /**
       * Start the plugin and begin work.
       */
      void plugin_startup() override;

      std::unique_ptr<seeding_plugin_impl> my;
};

}}
