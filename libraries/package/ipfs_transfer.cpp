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




ipfs_transfer::ipfs_transfer() {
	_client = new ipfs::Client("localhost", 5001);

}

ipfs_transfer::~ipfs_transfer() {
	delete _client;
	//_transfer_log.close();

}

void ipfs_transfer::print_status() {
	
}


void ipfs_transfer::upload_package(transfer_id id, const package_object& package, transfer_listener* listener) {
	_id = id;
	_listener = listener;
	_is_upload = true;
    

    vector<boost::filesystem::path> all_files;
    package.get_all_files(all_files);

    std::vector<ipfs::http::FileUpload> files_to_add;
    for (int i = 0; i < all_files.size(); ++i) {
    	files_to_add.push_back({ all_files[i].string(), ipfs::http::FileUpload::Type::kFileName, all_files[i].string() });
    }

	ipfs::Json added_files;
	_client->FilesAdd(files_to_add, &added_files);


//	_my_thread->async([this, package] () {
//	
//		path log_path = package_manager::instance().get_packages_path() / "transfer.log";
//		this->_transfer_log.open(log_path.string(), std::ios::out | std::ios::app);
//		this->_transfer_log << "***** Torrent upload started for package: " << package.get_hash().str() << endl;  
//    });



}

void ipfs_transfer::download_package(transfer_id id, const std::string& url, transfer_listener* listener) {
	_id = id;
	_listener = listener;
	_url = url;
	_is_upload = false;

	// _my_thread->async([this, url] () {
	
	// 	path log_path = package_manager::instance().get_packages_path() / "transfer.log";
	// 	this->_transfer_log.open(log_path.string(), std::ios::out | std::ios::app);
	// 	this->_transfer_log << "***** Torrent download started from url: " << url << endl;
    
 //    });

}




package_object ipfs_transfer::check_and_install_package() {
    // package::package_object obj("");

    // if (!obj.verify_hash()) {
    // 	FC_THROW("Unable to verify downloaded package");    
    // }

    // rename(path(st.save_path) / st.name, package_manager::instance().get_packages_path() / st.name);
    return package::package_object(package_manager::instance().get_packages_path());
}




std::string ipfs_transfer::get_transfer_url(transfer_id id) {
	return _url;
}




