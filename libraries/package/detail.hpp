/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#pragma once

#include <fc/crypto/ripemd160.hpp>
#include <fc/thread/thread.hpp>
#include <fc/network/url.hpp>

#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <atomic>
#include <memory>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <stdlib.h>

namespace decent { namespace package {


    class PackageInfo;


namespace detail {


    bool is_nested(boost::filesystem::path nested, boost::filesystem::path base);
    boost::filesystem::path get_relative(boost::filesystem::path from, boost::filesystem::path to);
    void get_files_recursive(const boost::filesystem::path& dir, std::vector<boost::filesystem::path>& all_files);
    void get_files_recursive_except(const boost::filesystem::path& dir, std::vector<boost::filesystem::path>& all_files, const std::set<boost::filesystem::path>& paths_to_skip);
    void remove_all_except(boost::filesystem::path dir, const std::set<boost::filesystem::path>& paths_to_skip);
    void move_all_except(boost::filesystem::path from_dir, boost::filesystem::path to_dir, const std::set<boost::filesystem::path>& paths_to_skip);
    void touch(const boost::filesystem::path& file_path);
    std::string get_proto(const std::string& url);
    bool is_correct_hash_str(const std::string& hash_str);
    fc::ripemd160 calculate_hash(const boost::filesystem::path& file_path);


    class PackageTask {
    public:

         PackageTask() = delete;
         PackageTask(PackageTask& package) = delete;
         PackageTask(const PackageTask& package) = delete;
         PackageTask operator =(PackageTask& task) = delete;
         PackageTask operator =(const PackageTask& task) = delete;
         PackageTask(PackageTask&& package) = delete;
        virtual ~PackageTask();

        virtual void start(const bool block = false);
        bool is_running() const;
        void stop(const bool block = false);
        bool is_stop_requested() const;
        void wait();
        std::exception_ptr consume_last_error();

    protected:
        explicit PackageTask(PackageInfo& package);
        class StopRequestedException {};

        virtual void task() {elog("This should never happened!"); std::abort();};

    private:
        std::atomic<bool>   _running;
        std::atomic<bool>   _stop_requested;
        std::exception_ptr  _last_exception;
        virtual bool is_base_class(){return true;};

    protected:
        std::shared_ptr<fc::thread> _thread;
        PackageInfo&                _package;
    };


#define PACKAGE_INFO_GENERATE_EVENT(event_name, event_params)            \
{                                                                        \
    std::lock_guard<std::recursive_mutex> guard(_package._event_mutex);  \
    for (auto& event_listener_ : _package._event_listeners)              \
        if (event_listener_)                                             \
            event_listener_-> event_name event_params ;                  \
}                                                                        \


#define PACKAGE_INFO_CHANGE_DATA_STATE(state)                                    \
{                                                                                \
    ilog("Package ${p} changed state ${s}", ("p", _package._url)("s", #state));       \
    PackageInfo::DataState new_state = PackageInfo:: state;                      \
    PackageInfo::DataState old_state = new_state;                                \
    {                                                                            \
        std::lock_guard<std::recursive_mutex> guard(_package._mutex);            \
        old_state = _package._data_state;                                        \
        _package._data_state = new_state;                                        \
    }                                                                            \
    if (old_state != new_state) {                                                \
        PACKAGE_INFO_GENERATE_EVENT(package_data_state_change, ( new_state ) );  \
    }     \
}                                                                                \


#define PACKAGE_INFO_CHANGE_TRANSFER_STATE(state)                                    \
{                                                                                    \
    ilog("Package ${p} changed state ${s}", ("p", _package._url)("s", #state));           \
    PackageInfo::TransferState new_state = PackageInfo:: state;                      \
    PackageInfo::TransferState old_state = new_state;                                \
    {                                                                                \
        std::lock_guard<std::recursive_mutex> guard(_package._mutex);                \
        old_state = _package._transfer_state;                                        \
        _package._transfer_state = new_state;                                        \
    }                                                                                \
    if (old_state != new_state) {                                                    \
        PACKAGE_INFO_GENERATE_EVENT(package_transfer_state_change, ( new_state ) );  \
    }                                                                                \
}                                                                                    \


#define PACKAGE_INFO_CHANGE_MANIPULATION_STATE(state)                                    \
{                                                                                        \
    ilog("Package ${p} changed state ${s}", ("p", _package._url)("s", #state));              \
    PackageInfo::ManipulationState new_state = PackageInfo:: state;                      \
    PackageInfo::ManipulationState old_state = new_state;                                \
    {                                                                                    \
        std::lock_guard<std::recursive_mutex> guard(_package._mutex);                    \
        old_state = _package._manipulation_state;                                        \
        _package._manipulation_state = new_state;                                        \
    }                                                                                    \
    if (old_state != new_state) {                                                        \
        PACKAGE_INFO_GENERATE_EVENT(package_manipulation_state_change, ( new_state ) );  \
    }                                                                                    \
}                                                                                        \


#define PACKAGE_TASK_EXIT_IF_REQUESTED { if (is_stop_requested()) throw StopRequestedException(); }



}



} } // namespace decent::package::detail
