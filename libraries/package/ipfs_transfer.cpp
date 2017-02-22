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

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/make_shared.hpp>



#include <graphene/package/package.hpp>

#include <fc/exception/exception.hpp>
#include <fc/network/ntp.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>

#include <iostream>
#include <fstream>
#include <atomic>

#include <decent/encrypt/encryptionutils.hpp>


#include "ipfs_transfer.hpp"


using namespace graphene::package;
using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::iostreams;
using namespace nlohmann;



ipfs_transfer::ipfs_transfer() {
	_client = new ipfs::Client("localhost", 5001);
	_my_thread = new fc::thread("ipfs_thread");
	_last_progress = transfer_progress(0, 1, 0); 
}

ipfs_transfer::~ipfs_transfer() {

	_my_thread->quit();
	
	delete _my_thread;
	_my_thread = NULL;
	
	delete _client;
	//_transfer_log.close();

}

void ipfs_transfer::print_status() {
	
}

package_transfer_interface::transfer_progress ipfs_transfer::get_progress() {
	return _last_progress;
}


void ipfs_transfer::upload_package(transfer_id id, const package_object& package, transfer_listener* listener) {
	_id = id;
	_listener = listener;
	_is_upload = true;
    

    vector<boost::filesystem::path> all_files;
    package.get_all_files(all_files);

    string packages_path = package_manager::instance().get_packages_path().string();


    std::vector<ipfs::http::FileUpload> files_to_add;

    for (int i = 0; i < all_files.size(); ++i) {
    	string fname = all_files[i].string();
    	fname.erase(0, packages_path.size());

    	files_to_add.push_back({ fname, ipfs::http::FileUpload::Type::kFileName, all_files[i].string() });
    }

	ipfs::Json added_files;
	_client->FilesAdd(files_to_add, &added_files);

	json::iterator it = added_files.begin();
	json::iterator itEnd = added_files.end();

	for (; it != itEnd; ++it) {
		string path_str = it->at("path");
		if (path_str == package.get_hash().str()) {
			break;
		}
	}

	if (it == itEnd) {
		FC_THROW("Unable to find root hash");    
	}

	string hash = it->at("hash");
	_url = "ipfs:" + hash + ":" + package.get_hash().str();
	
	_listener->on_upload_started(_id, _url);

}

void ipfs_transfer::download_package(transfer_id id, const std::string& url, transfer_listener* listener) {
	_id = id;
	_listener = listener;
	_url = url;
	_is_upload = false;

	size_t first = _url.find_first_of(":");
	if (first == string::npos) {
		FC_THROW("Invalid download URL");
	}

	string body = _url.substr(first + 1);

	size_t sep = body.find_first_of(":");
	if (sep == string::npos) {
		FC_THROW("Invalid download URL");
	}

	string hash = body.substr(0, sep);
	string package_name = body.substr(sep + 1);

	create_directories(package_manager::instance().get_packages_path() / package_name);

    ipfs::Json object;
    _client->ObjectGet(hash, &object);

    _listener->on_download_started(_id);

	_my_thread->async([this, package_name, object] () {
		
	    ipfs::Json links = object.at("Links");

	    json::iterator it = links.begin();
		json::iterator itEnd = links.end();

		int total_size = 0, downloaded_size = 0;

		for (; it != itEnd; ++it) {
			int file_size = it->at("Size");
			total_size += file_size;
		}

		it = links.begin();

		for (; it != itEnd; ++it) {
			string file_name = it->at("Name");
			string ipfs_hash = it->at("Hash");
			int file_size = it->at("Size");

			downloaded_size += file_size;
			
			std::stringstream contents;
	    	_client->FilesGet(ipfs_hash, &contents);

	    	std::ofstream myfile((package_manager::instance().get_packages_path() / package_name / file_name).string(), ios_base::out);
	    	myfile << contents.rdbuf();
	    	myfile.close();

	    	_last_progress = transfer_progress(total_size, downloaded_size, 0);
	    	
	    	_listener->on_download_progress(_id, _last_progress);
		}
	    
	    _last_progress = transfer_progress(total_size, total_size, 0);
		_listener->on_download_finished(_id, package_object((package_manager::instance().get_packages_path() / package_name).string()));
    
    });


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




