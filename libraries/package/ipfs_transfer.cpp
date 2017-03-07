
#include "ipfs_transfer.hpp"

#include <decent/encrypt/encryptionutils.hpp>
#include <graphene/package/package.hpp>

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
{
    if (!_thread)  FC_THROW("Thread instance is not available");
    if (!_mutex)   FC_THROW("Mutex instance is not available");
    if (!_client)  FC_THROW("Client instance is not available");
}

ipfs_transfer::ipfs_transfer()
    : _thread(std::make_shared<fc::thread>("ipfs_transfer"))
    , _mutex(std::make_shared<fc::mutex>())
    , _client(std::make_shared<ipfs::Client>("localhost", 5001))
{
}

ipfs_transfer::~ipfs_transfer() {
    // TODO
}

void ipfs_transfer::print_status() {
}

package_transfer_interface::transfer_progress ipfs_transfer::get_progress() {
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
	    	_last_progress = transfer_progress(total_size, downloaded_size, 0);
	    	_listener->on_download_progress(_id, _last_progress);
		}

        _last_progress = transfer_progress(total_size, total_size, 0);
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
