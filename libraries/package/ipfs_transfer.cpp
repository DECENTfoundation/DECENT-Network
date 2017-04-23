
#include "ipfs_transfer.hpp"

#include <graphene/package/package.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <vector>


namespace decent { namespace package {


    namespace detail {


        bool parse_ipfs_url(const std::string& url, std::string& obj_id) {
            const std::string ipfs = "ipfs:";
            if (url.substr(0, ipfs.size()) == ipfs) {
                obj_id = url.substr(ipfs.size());
                boost::algorithm::trim_left_if(obj_id, [](char ch) { return ch == '/'; });
                boost::algorithm::trim_right_if(obj_id, [](char ch) { return ch == '/'; });
                return true;
            }
            return false;
        }

    } // namespace detail



    IPFSDownloadPackageTask::IPFSDownloadPackageTask(PackageInfo& package)
        : detail::PackageTask(package)
        , _client("localhost", 5001)
    {
    }

    uint64_t IPFSDownloadPackageTask::ipfs_recursive_get_size(const std::string &url)
    {
        uint64_t size = 0;
        ipfs::Json objects;
        _client.Ls(url, &objects);

        for( auto nested_object : objects) {
            ipfs::Json links = nested_object.at("Links");

            for (auto link : links) {
               
            
                if((int) link.at("Type") == 1 ) //directory
                {
                    size += ipfs_recursive_get_size(link.at("Hash"));
                }

                if((int) link.at("Type") == 2 ) //file
                {
                    size += (uint64_t) link.at("Size");
                }

            }
        }
        return size;
    }

    void IPFSDownloadPackageTask::ipfs_recursive_get(const std::string &url,
                                                         const boost::filesystem::path &dest_path)
    {
        ilog("ipfs_recursive_get called for url ${u}",("u", url));
        FC_ASSERT( exists(dest_path) && is_directory(dest_path) );

        ipfs::Json objects;
        _client.Ls(url, &objects);

        for( auto nested_object : objects) {
            ilog("ipfs_recursive_get inside loop");
            ipfs::Json links = nested_object.at("Links");

            for( auto &link : links ) {
                if((int) link.at("Type") == 1 ) //directory
                {
                    const auto dir_name = dest_path / link.at("Name");
                    create_directories(dir_name);
                     ipfs_recursive_get(link.at("Hash"), dir_name);
                }
                if((int) link.at("Type") == 2 ) //file
                {
                    uint64_t size = (uint64_t) link.at("Size");
                    const std::string file_name = link.at("Name");
                    const std::string file_obj_id = link.at("Hash");
                    std::fstream myfile((dest_path / file_name).string(), std::ios::out | std::ios::binary);
                    _client.FilesGet(file_obj_id, &myfile);

                    _package._downloaded_size += size;
                }
                PACKAGE_TASK_EXIT_IF_REQUESTED;
            }
        }
    }

    void IPFSDownloadPackageTask::task() {
        PACKAGE_INFO_GENERATE_EVENT(package_download_start, ( ) );

        using namespace boost::filesystem;

        const auto temp_dir_path = unique_path(graphene::utilities::decent_path_finder::instance().get_decent_temp() / "%%%%-%%%%-%%%%-%%%%");

        try {
            PACKAGE_TASK_EXIT_IF_REQUESTED;

            std::string obj_id;

            if (!detail::parse_ipfs_url(_package._url, obj_id)) {
                FC_THROW("'${url}' is not an ipfs NURI", ("url", _package._url));
            }

            create_directories(temp_dir_path);
            remove_all(temp_dir_path);
            create_directories(temp_dir_path);

            PACKAGE_INFO_CHANGE_TRANSFER_STATE(DOWNLOADING);

            _package._size = ipfs_recursive_get_size( obj_id );
            _package._downloaded_size = 0;
            
            ipfs_recursive_get( obj_id, temp_dir_path );

            const auto content_file = temp_dir_path / "content.zip.aes";

            _package._hash = detail::calculate_hash(content_file);
            const auto package_dir = _package.get_package_dir();

            PACKAGE_TASK_EXIT_IF_REQUESTED;

            _package.lock_dir();

            PACKAGE_INFO_CHANGE_DATA_STATE(PARTIAL);

            std::set<boost::filesystem::path> paths_to_skip;

            paths_to_skip.clear();
            paths_to_skip.insert(_package.get_lock_file_path());
            detail::remove_all_except(package_dir, paths_to_skip);

            PACKAGE_TASK_EXIT_IF_REQUESTED;

            paths_to_skip.clear();
            paths_to_skip.insert(_package.get_package_state_dir(temp_dir_path));
            paths_to_skip.insert(_package.get_lock_file_path(temp_dir_path));
            detail::move_all_except(temp_dir_path, package_dir, paths_to_skip);

            remove_all(temp_dir_path);

            PACKAGE_INFO_CHANGE_DATA_STATE(CHECKED);
            PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_download_complete, ( ) );
        }
        catch ( const fc::exception& ex ) {
            remove_all(temp_dir_path);
            _package.unlock_dir();
            PACKAGE_INFO_CHANGE_DATA_STATE(INVALID);
            PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_download_error, ( ex.to_detail_string() ) );
            throw;
        }
        catch ( const std::exception& ex ) {
            remove_all(temp_dir_path);
            _package.unlock_dir();
            PACKAGE_INFO_CHANGE_DATA_STATE(INVALID);
            PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_download_error, ( ex.what() ) );
            throw;
        }
        catch ( ... ) {
            remove_all(temp_dir_path);
            _package.unlock_dir();
            PACKAGE_INFO_CHANGE_DATA_STATE(INVALID);
            PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_download_error, ( "unknown" ) );
            throw;
        }
    }

    IPFSStartSeedingPackageTask::IPFSStartSeedingPackageTask(PackageInfo& package)
        : detail::PackageTask(package)
        , _client("localhost", 5001)
    {
    }

    void IPFSStartSeedingPackageTask::task() {
        PACKAGE_INFO_GENERATE_EVENT(package_seed_start, ( ) );

        try {
            PACKAGE_TASK_EXIT_IF_REQUESTED;

            std::set<boost::filesystem::path> paths_to_skip;
            paths_to_skip.insert(_package.get_package_state_dir());
            paths_to_skip.insert(_package.get_lock_file_path());

            std::vector<boost::filesystem::path> files;
            detail::get_files_recursive_except(_package.get_package_dir(), files, paths_to_skip);

            std::vector<ipfs::http::FileUpload> files_to_add;

            const auto package_base_path = _package._parent_dir.lexically_normal();

            for (auto& file : files) {
                const auto file_rel_path = detail::get_relative(package_base_path, file.lexically_normal());
                files_to_add.push_back({ file_rel_path.string(), ipfs::http::FileUpload::Type::kFileName, file.string() });
            }

            PACKAGE_INFO_CHANGE_TRANSFER_STATE(SEEDING);

            ipfs::Json added_files;
            _client.FilesAdd(files_to_add, &added_files);

//          PACKAGE_INFO_GENERATE_EVENT(package_seed_progress, ( ) );

            std::string root_hash;
            for (auto& added_file : added_files) {
                if (added_file.at("path") == _package._hash.str()) {
                    root_hash = added_file.at("hash");
                    break;
                }
            }

            if (root_hash.empty()) {
                FC_THROW("Unable to find root hash in 'ipfs add' results");
            }

            _package._url = "ipfs:" + root_hash;

            _client.PinAdd(root_hash); // just in case

            // TODO: remove the actual files?

            PACKAGE_INFO_CHANGE_TRANSFER_STATE(SEEDING);
            PACKAGE_INFO_GENERATE_EVENT(package_seed_complete, ( ) );
        }
        catch ( const fc::exception& ex ) {
            PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_seed_error, ( ex.to_detail_string() ) );
            throw;
        }
        catch ( const std::exception& ex ) {
            PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_seed_error, ( ex.what() ) );
            throw;
        }
        catch ( ... ) {
            PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_seed_error, ( "unknown" ) );
            throw;
        }
    }

    IPFSStopSeedingPackageTask::IPFSStopSeedingPackageTask(PackageInfo& package)
        : detail::PackageTask(package)
        , _client("localhost", 5001)
    {
    }

    void IPFSStopSeedingPackageTask::task() {
        try {
            PACKAGE_TASK_EXIT_IF_REQUESTED;

            std::string obj_id;

            if (!detail::parse_ipfs_url(_package._url, obj_id)) {
                FC_THROW("'${url}' is not an ipfs NURI", ("url", _package._url));
            }

            _client.PinRm(obj_id, ipfs::Client::PinRmOptions::RECURSIVE);

            PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
        }
        catch ( const fc::exception& ex ) {
            PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_seed_error, ( ex.to_detail_string() ) );
            throw;
        }
        catch ( const std::exception& ex ) {
            PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_seed_error, ( ex.what() ) );
            throw;
        }
        catch ( ... ) {
            PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_seed_error, ( "unknown" ) );
            throw;
        }
    }

    std::shared_ptr<detail::PackageTask> IPFSTransferEngine::create_download_task(PackageInfo& package) {
        return std::make_shared<IPFSDownloadPackageTask>(package);
    }

    std::shared_ptr<detail::PackageTask> IPFSTransferEngine::create_start_seeding_task(PackageInfo& package) {
        return std::make_shared<IPFSStartSeedingPackageTask>(package);
    }

    std::shared_ptr<detail::PackageTask> IPFSTransferEngine::create_stop_seeding_task(PackageInfo& package) {
        return std::make_shared<IPFSStopSeedingPackageTask>(package);
    }


} } // namespace decent::package








// ====================================================












#include <decent/encrypt/encryptionutils.hpp>

#include <fc/exception/exception.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>
#include <fc/network/ntp.hpp>
#include <fc/thread/scoped_lock.hpp>
#include <fc/variant.hpp>

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <atomic>
#include <fstream>
#include <iostream>

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::iostreams;
using namespace nlohmann;
using namespace graphene::package;


ipfs_transfer::ipfs_transfer(const ipfs_transfer& orig)
    : _thread(orig._thread)
    , _mutex(orig._mutex)
    , _client(orig._client)
    , _transfer_logger(orig._transfer_logger)
{
    if (!_thread)  FC_THROW("Thread instance is not available");
    if (!_mutex)   FC_THROW("Mutex instance is not available");
    if (!_client)  FC_THROW("Client instance is not available");
}

ipfs_transfer::ipfs_transfer()
    : _thread(std::make_shared<fc::thread>("ipfs_transfer"))
    , _mutex(std::make_shared<fc::mutex>())
    , _client(std::make_shared<ipfs::Client>("localhost", 5001))
    , _transfer_logger(fc::logger::get("transfer"))
{
}

ipfs_transfer::~ipfs_transfer() {
    // TODO
}

void ipfs_transfer::print_status() {
}

transfer_progress ipfs_transfer::get_progress() {
    fc::scoped_lock<fc::mutex> guard(*_mutex);
	return _last_progress;
}

package_object ipfs_transfer::check_and_install_package() {
    // package::package_object obj("");

    // if (!obj.verify_hash()) {
    // 	FC_THROW("Unable to verify downloaded package");
    // }

    // rename(path(st.save_path) / st.name, package_manager::instance().get_packages_path() / st.name);
    return package::package_object(package_manager::instance().get_packages_path());
}

std::string ipfs_transfer::get_transfer_url() {
    return _url;
}

fc::ripemd160 ipfs_transfer::hash_from_url(const std::string& url) {
   const size_t last_char_i = url.size() - 1;
   
   const size_t first = url.find_first_of(":");
   if (first >= last_char_i) {
      FC_THROW("Invalid download URL");
   }
   
   const string proto = url.substr(0, first);
   if (proto != "ipfs") {
      FC_THROW("Invalid download URL");
   }
   
   const string body = url.substr(first + 1);
   const size_t sep = body.find_first_of(":");
   if (sep >= last_char_i) {
      FC_THROW("Invalid download URL");
   }
   
   const string hash = body.substr(0, sep);
   const string package_name = body.substr(sep + 1);
   return fc::ripemd160(package_name);
}

void ipfs_transfer::upload_package(transfer_id id, const package_object& package, transfer_listener* listener) {
	_id = id;
	_listener = listener;
	_is_upload = true;

    const string packages_path = package_manager::instance().get_packages_path().string();

    vector<boost::filesystem::path> all_files;
    package.get_all_files(all_files);

    std::vector<ipfs::http::FileUpload> files_to_add;
    for (auto file : all_files) {
        const string file_full_path = file.string();
        string file_relative_path = file.string();
    	file_relative_path.erase(0, packages_path.size());

        files_to_add.push_back({ file_relative_path, ipfs::http::FileUpload::Type::kFileName, file_full_path });
    }

	ipfs::Json added_files;
	_client->FilesAdd(files_to_add, &added_files);

	json::iterator it = added_files.begin();
	json::iterator itEnd = added_files.end();

	for (; it != itEnd; ++it) {
		const string path_str = it->at("path");
		if (path_str == package.get_hash().str()) {
			break;
		}
	}

	if (it == itEnd) {
		FC_THROW("Unable to find root hash");
	}

	const string hash = it->at("hash");
	_url = "ipfs:" + hash + ":" + package.get_hash().str();
	
	_listener->on_upload_started(_id, _url);

}

void ipfs_transfer::download_package(transfer_id id, const std::string& url, transfer_listener* listener, report_stats_listener_base& stats_listener) {
	_id = id;
	_listener = listener;
	_url = url;
	_is_upload = false;
	ilog("ipfs_transfer::download_package called for ${u}",("u", url));
    if (_url.empty()) {
        FC_THROW("Invalid download URL");
    }

    const size_t last_char_i = _url.size() - 1;

	const size_t first = _url.find_first_of(":");
	if (first >= last_char_i) {
		FC_THROW("Invalid download URL");
	}

    const string proto = _url.substr(0, first);
    if (proto != "ipfs") {
        FC_THROW("Invalid download URL");
    }

	const string body = _url.substr(first + 1);
	const size_t sep = body.find_first_of(":");
	if (sep >= last_char_i) {
		FC_THROW("Invalid download URL");
	}

	const string hash = body.substr(0, sep);
	const string package_name = body.substr(sep + 1);

	create_directories(package_manager::instance().get_packages_path() / package_name);

    _thread->async([this, package_name, hash, &stats_listener] () {

		_listener->on_download_started(_id);

        ipfs::Json object;
        _client->ObjectGet(hash, &object);


	    ipfs::Json links = object.at("Links");

        size_t total_size = 0;
        for (auto link : links) {
			total_size += (size_t)link.at("Size");
		}

        size_t downloaded_size = 0;
		for (auto link : links) {
			const string file_name = link.at("Name");
			const string ipfs_hash = link.at("Hash");
			const size_t file_size = link.at("Size");

            std::fstream myfile((package_manager::instance().get_packages_path() / package_name / file_name).string(), ios_base::out);
            _client->FilesGet(ipfs_hash, &myfile);
	    	myfile.close();

            downloaded_size += file_size;
	    	_last_progress = transfer_progress(total_size, downloaded_size, 0, "Downloading...");
	    	_listener->on_download_progress(_id, _last_progress);
		}

        _last_progress = transfer_progress(total_size, total_size, 0, "Download Finished");
		_listener->on_download_finished(_id, package_object((package_manager::instance().get_packages_path() / package_name).string()));


		std::map<std::string, uint64_t> stats;
		uint64_t sum;
		for(const auto& item : stats_listener.ipfs_IDs)
		{
			sum = 0;
			for( const auto& id : item.second)
			{
				ipfs::Json json;
				_client->BitswapLedger( id, &json);
				sum += json["Recv"].get<uint64_t>();
			}
			stats[item.first] = sum;
		}

		stats_listener.report_stats( stats );

    });
}
