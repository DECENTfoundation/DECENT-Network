
#pragma once

#include <fc/variant.hpp>
#include <fc/optional.hpp>
#include <fc/filesystem.hpp>
#include <fc/log/logger_config.hpp>

namespace boost { namespace program_options { class options_description; } }
namespace decent {

    void write_default_config_file(const fc::path& config_ini_filename, const boost::program_options::options_description &cfg_options, bool is_daemon);

    fc::optional<fc::logging_config> load_logging_config_from_ini_file(const fc::path& config_ini_filename, const fc::path& logs_dir);

}
