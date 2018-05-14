
#pragma once

#include <ostream>
#include <fc/variant.hpp>
#include <fc/optional.hpp>
#include <fc/filesystem.hpp>
#include <fc/log/logger_config.hpp>

namespace decent {

    void write_default_logging_config_to_stream(std::ostream &out, bool is_daemon);

    fc::optional<fc::logging_config> load_logging_config_from_ini_file(const fc::path& config_ini_filename, const fc::path& logs_dir);

}
