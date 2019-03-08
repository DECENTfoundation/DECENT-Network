/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#include "detail.hpp"

#include <fc/network/url.hpp>
#include <fc/thread/thread.hpp>

#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <atomic>
#include <memory>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>


namespace decent { namespace package { namespace detail {


    bool is_nested(boost::filesystem::path nested, boost::filesystem::path base)
    {
        nested = nested.lexically_normal();
        base = base.lexically_normal();

        boost::filesystem::path::const_iterator base_it = base.begin();
        boost::filesystem::path::const_iterator nested_it = nested.begin();

        while (base_it != base.end() && nested_it != nested.end() && (*base_it) == (*nested_it))
        {
            ++base_it;
            ++nested_it;
        }

        return base_it == base.end()/* && nested_it != nested.end()*/;
    }

    boost::filesystem::path get_relative(boost::filesystem::path from, boost::filesystem::path to)
    {
        from = from.lexically_normal();
        to = to.lexically_normal();

        boost::filesystem::path::const_iterator from_it = from.begin();
        boost::filesystem::path::const_iterator to_it = to.begin();

        while (from_it != from.end() && to_it != to.end() && (*from_it) == (*to_it))
        {
            ++from_it;
            ++to_it;
        }

        boost::filesystem::path relative;

        while (from_it != from.end())
        {
            relative /= "..";
            ++from_it;
        }

        while (to_it != to.end())
        {
            relative /= *to_it;
            ++to_it;
        }

        return relative;
    }

    void get_files_recursive(const boost::filesystem::path& dir, std::vector<boost::filesystem::path>& all_files) {
        using namespace boost::filesystem;

        if (!is_directory(dir)) {
            FC_THROW("${path} does not point to an existing diectory", ("path", dir.string()) );
        }

        for (recursive_directory_iterator it(dir); it != recursive_directory_iterator(); ++it) {
            if (is_regular_file(*it)) {
                all_files.push_back(*it);
            }
            else if (is_directory(*it) && is_symlink(*it)) {
                it.no_push();
            }
        }
    }

    void get_files_recursive_except(const boost::filesystem::path& dir, std::vector<boost::filesystem::path>& some_files,
                                    const std::set<boost::filesystem::path>& paths_to_skip) {
        using namespace boost::filesystem;

        if (!is_directory(dir)) {
            FC_THROW("${path} does not point to an existing diectory", ("path", dir.string()) );
        }

        for (recursive_directory_iterator it(dir); it != recursive_directory_iterator(); ++it) {
            if (is_regular_file(*it) && paths_to_skip.find(*it) == paths_to_skip.end()) {
                some_files.push_back(*it);
            }
            else if ((is_directory(*it) && is_symlink(*it)) || paths_to_skip.find(*it) != paths_to_skip.end()) {
                it.no_push();
            }
        }
    }

    void remove_all_except(boost::filesystem::path dir, const std::set<boost::filesystem::path>& paths_to_skip) {
        using namespace boost::filesystem;

        if (!is_directory(dir)) {
            FC_THROW("${path} does not point to an existing diectory", ("path", dir.string()) );
        }

        dir = dir.lexically_normal();
        std::set<path> paths_to_remove;

        for (recursive_directory_iterator it(dir); it != recursive_directory_iterator(); ++it) {
            if (is_directory(*it) && is_symlink(*it)) {
                it.no_push();
            }

            bool skip_this = false;

            for (auto path_to_skip : paths_to_skip) {
                if (path_to_skip.is_relative()) {
                    path_to_skip = dir / path_to_skip;
                }

                if (is_nested(path_to_skip, it->path())) {
                    skip_this = true;
                    break;
                }
            }

            if (!skip_this) {
                paths_to_remove.insert(it->path());
                it.no_push();
            }
        }

        for (const auto& path_to_remove : paths_to_remove) {
            remove_all(path_to_remove);
        }
    }

    void move_all_except(boost::filesystem::path from_dir, boost::filesystem::path to_dir, const std::set<boost::filesystem::path>& paths_to_skip) {
        using namespace boost::filesystem;

        if (!is_directory(from_dir)) {
            FC_THROW("${path} does not point to an existing diectory", ("path", from_dir.string()) );
        }

        if (!is_directory(to_dir)) {
            FC_THROW("${path} does not point to an existing diectory", ("path", to_dir.string()) );
        }

        from_dir = from_dir.lexically_normal();
        to_dir = to_dir.lexically_normal();
        std::map<path, path> paths_to_rename;

        for (recursive_directory_iterator it(from_dir); it != recursive_directory_iterator(); ++it) {
            if (is_directory(*it) && is_symlink(*it)) {
                it.no_push();
            }

            bool skip_this = false;

            for (auto path_to_skip : paths_to_skip) {
                if (path_to_skip.is_relative()) {
                    path_to_skip = from_dir / path_to_skip;
                }

                path_to_skip = path_to_skip.lexically_normal();

                if (it->path().lexically_normal() == path_to_skip) {
                    skip_this = true;
                    break;
                }
            }

            if( is_directory(*it) && !skip_this ) {
                create_directory( to_dir / get_relative(from_dir, it->path() ) );
                skip_this = true;
            }

            if (!skip_this) {
                paths_to_rename[it->path()] = to_dir / get_relative(from_dir, it->path());
            }
        }

        for (const auto& path_to_rename : paths_to_rename) {
            rename(path_to_rename.first, path_to_rename.second);
        }
    }

    void touch(const boost::filesystem::path& file_path) {
        using namespace boost::filesystem;
        using namespace boost::interprocess;

        const auto file_dir = file_path.parent_path();
        if (!create_directories(file_dir) && !is_directory(file_dir)) {
            FC_THROW("Unable to create directory ${path}", ("path", file_dir.string()) );
        }

        std::ofstream file(file_path.string());
    }

    std::string get_proto(const std::string& url) {
        const std::string ipfs = "ipfs:";
        const std::string magnet = "magnet:";
        if (url.substr(0, ipfs.size()) == ipfs)           { return "ipfs"; }
        else if (url.substr(0, magnet.size()) == magnet)  { return "magnet"; }
        else                                              { return fc::url(url).proto(); }
    }

    bool is_correct_hash_str(const std::string& hash_str) {
        if (hash_str.size() != 40) {
            return false;
        }

        for(auto ch : hash_str) {
            if ( !(ch >= '0' && ch <= '9') &&
                 !(ch >= 'a' && ch <= 'f') &&
                 !(ch >= 'A' && ch <= 'F') ) {
                return false;
            }
        }

        return true;
    }

    fc::ripemd160 calculate_hash(const boost::filesystem::path& file_path) {
        std::ifstream fin(file_path.string().c_str(), std::ios::binary | std::ios::in);

        if (!fin.is_open()) {
            FC_THROW("Unable to open file ${fn} for reading", ("fn", file_path.string()) );
        }

        fc::ripemd160::encoder ripe_calc;

        const size_t RIPEMD160_BUFFER_SIZE  = 1024 * 1024; // 1Mb
        char* buffer = new char[RIPEMD160_BUFFER_SIZE];

        const size_t source_size = boost::filesystem::file_size(file_path);
        size_t total_read = 0;

        while (true) {
            fin.read(buffer, RIPEMD160_BUFFER_SIZE);
            auto bytes_read = static_cast<size_t>(fin.gcount());

            if (bytes_read > 0) {
                ripe_calc.write(buffer, bytes_read);
                total_read += bytes_read;
            }

            if (bytes_read < (int)RIPEMD160_BUFFER_SIZE) {
                break;
            }
        }

        delete[] buffer;

        if (total_read != source_size) {
            FC_THROW("Failed to read entire ${fn} file", ("fn", file_path.string()) );
        }

        return ripe_calc.result();
    }

    PackageTask::PackageTask(PackageInfo& package)
        : _running(false)
        , _stop_requested(false)
        , _last_exception(nullptr)
        , _package(package)
    {
    }

    PackageTask::~PackageTask() {
        stop(true);
    }

    void PackageTask::start(const bool block) {
        stop(true);
        if(is_base_class()){
            elog("calling packagetask::start from base class!!!");
            std::abort();
        }
        _running = true;
        _stop_requested = false;
        _last_exception = nullptr;

        auto run_task = [this] () {
            try {
                task();
            }
            catch (StopRequestedException&) {
            }
            catch ( ... ) {
                _last_exception = std::current_exception();
            }

            _running = false;
        };

        if (block) {
            run_task();
        }
        else {
            _thread.reset(new fc::thread());
            _thread->async(run_task);
        }
    }

    bool PackageTask::is_running() const {
        return _running;
    }

    void PackageTask::stop(const bool block) {
        _stop_requested = true;

        if (block) {
            wait();
        }
    }

    bool PackageTask::is_stop_requested() const {
        return _stop_requested;
    }

    void PackageTask::wait() {
        while (is_running()) {
            // TODO: improve
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    std::exception_ptr PackageTask::consume_last_error() {
        auto last_exception = _last_exception;
        _last_exception = nullptr;
        return last_exception;
    }


}//namespace detail



} } // namespace decent::package
