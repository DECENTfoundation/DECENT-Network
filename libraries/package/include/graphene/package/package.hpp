
#pragma once

#include <decent/encrypt/custodyutils.hpp>
#include <graphene/utilities/dirhelper.hpp>

#include <fc/thread/thread.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/sha512.hpp>

#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>


namespace decent { namespace package {


    class package_manager;
    class package_info;
    class event_listener_interface;
    class transfer_engine_interface;

    typedef std::shared_ptr<package_info>               package_handle;
    typedef std::set<package_handle>                    package_handle_set;
    typedef std::shared_ptr<event_listener_interface>   event_listener_handle;
    typedef std::list<event_listener_handle>            event_listener_handle_list;
    typedef std::shared_ptr<transfer_engine_interface>  transfer_engine;
    typedef std::map<std::string, transfer_engine>      proto_to_transfer_engine_map;


    class package_info {
    public:
        enum State {
            UNINITIALIZED,
            INVALID,
            PARTIAL,
            UNCHECKED,
            CHECKED
        };

        enum Action {
            IDLE,
            PACKING,
            ENCRYPTING,
            STAGING,
            SEEDING,
            DOWNLOADING,
            CHECKING,
            DECRYPTING,
            UNPACKING,
            DELETTING
        };

    private:
        friend class package_manager;

        package_info(package_manager& manager,
                     const boost::filesystem::path& content_dir_path,
                     const boost::filesystem::path& samples_dir_path,
                     const fc::sha512& key,
                     const event_listener_handle& event_listener = event_listener_handle());

        package_info(package_manager& manager,
                     const fc::ripemd160& package_hash,
                     const event_listener_handle& event_listener = event_listener_handle());

        package_info(package_manager& manager,
                     const std::string& url,
                     const event_listener_handle& event_listener = event_listener_handle());

    public:
        package_info(const package_info&)             = delete;
        package_info(package_info&&)                  = delete;
        package_info& operator=(const package_info&)  = delete;
        package_info& operator=(package_info&&)       = delete;

        ~package_info();

    public:
        void add_event_listener(const event_listener_handle& event_listener);
        void remove_event_listener(const event_listener_handle& event_listener);

        State get_state() const;
        Action get_action() const;

    private:
        static boost::filesystem::path get_package_state_dir(const boost::filesystem::path& package_dir)  { return package_dir / ".state" ; }
        static boost::filesystem::path get_lock_file_path(const boost::filesystem::path& package_dir)     { return get_package_state_dir(package_dir) / "lock"; }

        boost::filesystem::path get_package_dir() const        { return _parent_dir / _hash.str(); }
        boost::filesystem::path get_package_state_dir() const  { return get_package_state_dir(get_package_dir()); }
        boost::filesystem::path get_lock_file_path() const     { return get_lock_file_path(get_package_dir()); }

        void lock_dir();
        void unlock_dir();

    private:
        mutable std::recursive_mutex  _mutex;
        std::shared_ptr<fc::thread>   _thread;
        State                         _state;
        Action                        _action;
        fc::ripemd160                 _hash;
        boost::filesystem::path       _parent_dir;
        std::shared_ptr<boost::interprocess::file_lock> _file_lock;
        std::shared_ptr<boost::interprocess::scoped_lock<boost::interprocess::file_lock>> _file_lock_guard;
        mutable std::recursive_mutex  _event_mutex;
        event_listener_handle_list    _event_listeners;
    };


    class event_listener_interface {
    public:
        virtual ~event_listener_interface() {}

        virtual void package_state_change(package_info::State) {};
        virtual void package_action_change(package_info::Action) {};

        virtual void package_creation_start() {};
        virtual void package_creation_progress(const std::string&) {};
        virtual void package_creation_error(const std::string&) {};
        virtual void package_creation_complete() {};

        virtual void package_restoration_start() {};
        virtual void package_restoration_progress() {};
        virtual void package_restoration_error(const std::string&) {};
        virtual void package_restoration_complete() {};

        virtual void package_transfer_start() {};
        virtual void package_transfer_progress() {};
        virtual void package_transfer_error(const std::string&) {};
        virtual void package_transfer_complete() {};
    };


    class transfer_engine_interface {
    public:
        virtual ~transfer_engine_interface() {}

    };


    class package_manager {
    private:
        explicit package_manager(const boost::filesystem::path& packages_path);

    public:
        package_manager()                                   = delete;
        package_manager(const package_manager&)             = delete;
        package_manager(package_manager&&)                  = delete;
        package_manager& operator=(const package_manager&)  = delete;
        package_manager& operator=(package_manager&&)       = delete;

        ~package_manager();

        static package_manager& instance() {
            static package_manager the_package_manager(graphene::utilities::decent_path_finder::instance().get_decent_data() / "packages");
            return the_package_manager;
        }

    public:
        package_handle get_package(const boost::filesystem::path& content_dir_path,
                                   const boost::filesystem::path& samples_dir_path,
                                   const fc::sha512& key,
                                   const event_listener_handle& event_listener = event_listener_handle());

        package_handle get_package(const std::string& url,
                                   const event_listener_handle& event_listener = event_listener_handle());

        package_handle get_package(const fc::ripemd160& hash); // TODO: do we need this?

        void release_package(const fc::ripemd160& hash);
        void release_package(const package_handle& package);

        package_handle_set get_all_known_packages() const;
        boost::filesystem::path get_packages_path() const;
        std::shared_ptr<fc::thread> get_thread() const;
        void set_libtorrent_config(const boost::filesystem::path& libtorrent_config_file);

    private:
        mutable std::recursive_mutex  _mutex;
        std::shared_ptr<fc::thread>   _thread;
        boost::filesystem::path       _packages_path;
        package_handle_set            _packages;
        proto_to_transfer_engine_map  _proto_transfer_engines;
    };


} } // decent::package







#pragma once

#include <decent/encrypt/crypto_types.hpp>
#include <decent/encrypt/custodyutils.hpp>

#include <fc/optional.hpp>
#include <fc/signals.hpp>
#include <fc/time.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/sha512.hpp>
#include <fc/network/url.hpp>

#include <boost/filesystem.hpp>

#include <vector>
#include <map>


namespace graphene { namespace package {


class report_stats_listener_base {
public:
    std::map<std::string, std::vector<std::string>> ipfs_IDs;

    virtual void report_stats( std::map<std::string,uint64_t> stats )=0;
};


class empty_report_stats_listener : public report_stats_listener_base {
public:
    static empty_report_stats_listener& instance() {
        static empty_report_stats_listener the_empty_report_stats_listener;
        return the_empty_report_stats_listener;
    }

    virtual void report_stats( std::map<std::string,uint64_t> stats ){};
};


class package_object {
public:
    package_object() = delete;
    explicit package_object(const boost::filesystem::path& package_path);

    boost::filesystem::path get_custody_file() const { return _package_path / "content.cus"; }
    boost::filesystem::path get_content_file() const { return _package_path / "content.zip.aes"; }
    boost::filesystem::path get_samples_path() const { return _package_path / "samples"; }
    const boost::filesystem::path& get_path() const { return _package_path; }

    void get_all_files(std::vector<boost::filesystem::path>& all_files) const;
    bool verify_hash() const;
    fc::ripemd160 get_hash() const { return _hash; }
    int get_size() const;
    bool is_valid() const { return _hash != fc::ripemd160(); }
    uint32_t create_proof_of_custody(const decent::crypto::custody_data& cd, decent::crypto::custody_proof& proof) const;

private:
    boost::filesystem::path  _package_path;
    fc::ripemd160            _hash;
};


    class package_transfer_interface : public decent::package::transfer_engine_interface {
public:
    typedef int transfer_id;

    struct transfer_progress {
        transfer_progress() : total_bytes(0), current_bytes(0), current_speed(0), str_status("No Status") { }
        transfer_progress(int tb, int cb, int cs, const std::string& status) : total_bytes(tb), current_bytes(cb), current_speed(cs), str_status(status) { }

        int total_bytes;
        int current_bytes;
        int current_speed; // Bytes per second
        std::string str_status;
    };

    class transfer_listener {
    public:
        virtual ~transfer_listener() = default;
        virtual void on_download_started(transfer_id id) = 0;
        virtual void on_download_finished(transfer_id id, package_object downloaded_package) = 0;
        virtual void on_download_progress(transfer_id id, transfer_progress progress) = 0;
        virtual void on_upload_started(transfer_id id, const std::string& url) = 0;
        virtual void on_error(transfer_id id, std::string error) = 0;
    };

    virtual ~package_transfer_interface() = default;
    virtual void upload_package(transfer_id id, const package_object& package, transfer_listener* listener) = 0;
    virtual void download_package(transfer_id id, const std::string& url, transfer_listener* listener, report_stats_listener_base& stats_listener) = 0;
    virtual void print_status() = 0;
    virtual transfer_progress get_progress() = 0;
    virtual std::string get_transfer_url() = 0;
    virtual std::shared_ptr<package_transfer_interface> clone() = 0;
};


class empty_transfer_listener : public package_transfer_interface::transfer_listener {
public:
    static empty_transfer_listener& instance() {
        static empty_transfer_listener the_empty_transfer_listener;
        return the_empty_transfer_listener;
    }

    virtual void on_download_started(package_transfer_interface::transfer_id id) { }
    virtual void on_download_finished(package_transfer_interface::transfer_id id, package_object downloaded_package) { }
    virtual void on_download_progress(package_transfer_interface::transfer_id id, package_transfer_interface::transfer_progress progress) { }
    virtual void on_upload_started(package_transfer_interface::transfer_id id, const std::string& url) { }
    virtual void on_error(package_transfer_interface::transfer_id id, std::string error) { }
};


class package_manager {
private:
    struct transfer_job {
        enum transfer_type {
            DOWNLOAD,
            UPLOAD
        };

        std::shared_ptr<package_transfer_interface>     transport;
        package_transfer_interface::transfer_listener*  listener;
        package_transfer_interface::transfer_id         job_id;
        transfer_type                                   job_type;
    };

private:
    typedef std::map<std::string, std::shared_ptr<package_transfer_interface>>  protocol_handler_map;
    typedef std::map<package_transfer_interface::transfer_id, transfer_job>     transfer_map;
    typedef std::set<package_object>                                            package_set;

private:
    package_manager();

public:
    package_manager(const package_manager&)             = delete;
    package_manager(package_manager&&)                  = delete;
    package_manager& operator=(const package_manager&)  = delete;
    package_manager& operator=(package_manager&&)       = delete;

    ~package_manager();

    static package_manager& instance() {
        static package_manager the_package_manager;
        return the_package_manager;
    }

public:
    package_object create_package( const boost::filesystem::path& content_path,
                                   const boost::filesystem::path& samples, 
                                   const fc::sha512& key,
                                   decent::crypto::custody_data& cd);

    bool unpack_package( const boost::filesystem::path& destination_directory, 
                         const package_object& package,
                         const fc::sha512& key);

    void delete_package(fc::ripemd160 hash);

    package_transfer_interface::transfer_id upload_package( const package_object& package, 
                                                            const std::string& protocol_name,
                                                            package_transfer_interface::transfer_listener& listener );

    package_transfer_interface::transfer_id download_package( const std::string& url,
                                                              package_transfer_interface::transfer_listener& listener,
                                                              report_stats_listener_base& stats_listener );


    std::vector<package_object> get_packages();
    package_object              get_package_object(fc::ripemd160 hash);

    std::string                                   get_transfer_url(package_transfer_interface::transfer_id id);
    package_transfer_interface::transfer_progress get_progress(std::string URI) const;

    void set_packages_path(const boost::filesystem::path& packages_path);
    boost::filesystem::path get_packages_path() const;

    void set_libtorrent_config(const boost::filesystem::path& libtorrent_config_file);
    boost::filesystem::path get_libtorrent_config() const;

    uint32_t create_proof_of_custody(const boost::filesystem::path& content_file, const decent::crypto::custody_data& cd, decent::crypto::custody_proof& proof);
    void print_all_transfers();

private:
    transfer_job& create_transfer_object();

    void save_state();
    void restore_state();

private:
    mutable fc::mutex                  _mutex;
    boost::filesystem::path            _packages_path;
    boost::filesystem::path            _libtorrent_config_file;
    decent::crypto::custody_utils      _custody_utils;
    protocol_handler_map               _protocol_handlers;
    transfer_map                       _transfers;
    int                                _next_transfer_id;
    package_set                        _packages;
};


} } // graphene::package
