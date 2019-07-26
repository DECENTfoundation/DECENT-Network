/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include <graphene/app/application.hpp>

namespace graphene { namespace app {

class abstract_plugin
{
   public:
      virtual ~abstract_plugin() = default;

      /**
       * @brief Perform early startup routines and register plugin indexes, callbacks, etc.
       *
       * Plugins MUST supply a method initialize() which will be called early in the application startup. This method
       * should contain early setup code such as initializing variables, adding indexes to the database, registering
       * callback methods from the database, adding APIs, etc., as well as applying any options in the \p options map
       *
       * This method is called BEFORE the database is open, therefore any routines which require any chain state MUST
       * NOT be called by this method. These routines should be performed in startup() instead.
       *
       * @param options The options passed to the application, via configuration files or command line
       */
      virtual void plugin_initialize( const boost::program_options::variables_map& options ) = 0;

      /**
       * @brief Begin normal runtime operations
       *
       * Plugins MUST supply a method startup() which will be called at the end of application startup. This method
       * should contain code which schedules any tasks, or requires chain state.
       */
      virtual void plugin_startup() = 0;

      /**
       * @brief Cleanly shut down the plugin.
       *
       * This is called to request a clean shutdown (e.g. due to SIGINT or SIGTERM).
       */
      virtual void plugin_shutdown() = 0;

      /**
       * @brief Get the plugin name.
       *
       * @return plugin name
       */
      static std::string plugin_name();

      /**
       * @brief Fill in command line parameters used by the plugin.
       *
       * @param command_line_options All options this plugin supports taking on the command-line
       * @param config_file_options All options this plugin supports storing in a configuration file
       *
       * This method populates its arguments with any command-line and configuration file options the plugin supports.
       */
      static void plugin_set_program_options(
         boost::program_options::options_description& command_line_options,
         boost::program_options::options_description& config_file_options);
};

/**
 * Provides basic default implementations of abstract_plugin functions.
 */

class plugin : public abstract_plugin
{
   public:
      plugin(application* app);
      virtual ~plugin() override;

      virtual void plugin_initialize( const boost::program_options::variables_map& options ) override {}
      virtual void plugin_startup() override {}
      virtual void plugin_shutdown() override {}

      chain::database& database() { return *app().chain_database(); }
      application& app()const { assert(_app); return *_app; }

   private:
      application* _app = nullptr;
};

template<typename Plugin>
static void set_plugin_program_options(boost::program_options::options_description& command_line_options,
                                       boost::program_options::options_description& configuration_file_options)
{
   boost::program_options::options_description plugin_cli_options("Options for plugin " + Plugin::plugin_name()), plugin_cfg_options;
   Plugin::plugin_set_program_options(plugin_cli_options, plugin_cfg_options);
   if( !plugin_cli_options.options().empty() )
      command_line_options.add(plugin_cli_options);
   if( !plugin_cfg_options.options().empty() )
      configuration_file_options.add(plugin_cfg_options);
}

template<typename ...Plugins>
struct plugin_set
{
   using types = std::tuple<std::shared_ptr<Plugins>...>;
   template<std::size_t N>
   using type = typename std::tuple_element<N, types>::type::element_type;

   static void set_program_options(boost::program_options::options_description& command_line_options,
                                   boost::program_options::options_description& configuration_file_options)
   {
      set_plugins_program_options(command_line_options, configuration_file_options,
                                  std::make_index_sequence<std::tuple_size<types>::value>());
   }

   static types create(application &app)
   {
      return create_plugins(app, std::make_index_sequence<std::tuple_size<types>::value>());
   }

private:
   template<std::size_t ...Idx>
   static void set_plugins_program_options(boost::program_options::options_description& command_line_options,
                                           boost::program_options::options_description& configuration_file_options,
                                           std::index_sequence<Idx...>)
   {
      auto x = {(set_plugin_program_options<type<Idx>>(command_line_options, configuration_file_options), 0)...};
      (void)x;
   }

   template<std::size_t ...Idx>
   static types create_plugins(application &app, std::index_sequence<Idx...>)
   {
      return std::make_tuple((app.create_plugin<type<Idx>>())...);
   }
};

} } //graphene::app
