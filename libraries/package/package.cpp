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

#include "torrent_transfer.hpp"
#include "ipfs_transfer.hpp"

#include <decent/encrypt/encryptionutils.hpp>
#include <graphene/package/package.hpp>

#include "json.hpp"

#include <fc/exception/exception.hpp>
#include <fc/network/ntp.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/iostreams/copy.hpp>

#include <iostream>
#include <atomic>

using namespace std;
using namespace nlohmann;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::iostreams;
using namespace graphene::package;


namespace {

const int ARC_BUFFER_SIZE  = 1024 * 1024; // 4kb
const int RIPEMD160_BUFFER_SIZE  = 1024 * 1024; // 4kb

struct arc_header {
    char type; // 0 = EOF, 1 = REGULAR FILE
	char name[255];
	char size[8];
};

class archiver {
	filtering_ostream&   _out;
public:
	archiver(filtering_ostream& out): _out(out) {

	}

	bool put(const std::string& file_name, file_source& in, int file_size) {
		arc_header header;

		std::memset((void*)&header, 0, sizeof(arc_header));
        
	    std::snprintf(header.name, 255, "%s", file_name.c_str());
        
        header.type = 1;
        *(int*)header.size = file_size;


		_out.write((const char*)&header,sizeof(arc_header));
        
        char buffer[ARC_BUFFER_SIZE];
        int bytes_read = in.read(buffer, ARC_BUFFER_SIZE);
        
        while (bytes_read > 0) {
            _out.write(buffer, bytes_read);
            bytes_read = in.read(buffer, ARC_BUFFER_SIZE);
        }
        
        return true;
	}

	void finalize() {
		arc_header header;

		std::memset((void*)&header, 0, sizeof(arc_header));
		_out.write((const char*)&header,sizeof(arc_header));
		_out.flush();
        _out.reset();      
	}

};


class dearchiver {
    filtering_istream& _in;

public:
    dearchiver(filtering_istream& in) : _in(in) { }

    bool extract(const std::string& output_dir) {
        arc_header header;

        while (true) {
            std::memset((void*)&header, 0, sizeof(arc_header));
            _in.read((char*)&header, sizeof(arc_header));
            if (header.type == 0) {
                break;
            }

            const path file_path = output_dir / header.name;
            const path file_dir = file_path.parent_path();

            if (!exists(file_dir) || !is_directory(file_dir)) {
                try {
                    if (!create_directories(file_dir) && !is_directory(file_dir)) {
                        FC_THROW("Unable to create ${dir} directory", ("dir", file_dir.string()) );
                    }
                }
                catch (const boost::filesystem::filesystem_error& ex) {
                    if (!is_directory(file_dir)) {
                        FC_THROW("Unable to create ${dir} directory: ${error}", ("dir", file_dir.string()) ("error", ex.what()) );
                    }
                }
            }

            std::fstream sink(file_path.string(), ios::out | ios::binary);
            if (!sink.is_open()) {
                FC_THROW("Unable to open file ${file} for writing", ("file", file_path.string()) );
            }

            char buffer[ARC_BUFFER_SIZE];
            int bytes_to_read = *(int*)header.size;

            if (bytes_to_read < 0) {
                FC_THROW("Unexpected size in header");
            }

            while (bytes_to_read > 0) {
                const int bytes_read = boost::iostreams::read(_in, buffer, std::min(ARC_BUFFER_SIZE, bytes_to_read));
                if (bytes_read < 0) {
                    break;
                }

                sink.write(buffer, bytes_read);
                if (sink.bad()) {
                    FC_THROW("Unable to write to file ${file}", ("file", file_path.string()) );
                }

                bytes_to_read -= bytes_read;
            }

            if (bytes_to_read != 0) {
                FC_THROW("Unexpected end of file");
            }
        }
        
        return true;
    }
};


string make_uuid() {
    boost::uuids::random_generator generator;
    return boost::uuids::to_string(generator());
}

void get_files_recursive(boost::filesystem::path path, std::vector<boost::filesystem::path>& all_files) {
 
    boost::filesystem::recursive_directory_iterator it = recursive_directory_iterator(path);
    boost::filesystem::recursive_directory_iterator end;
 
    while(it != end) // 2.
    {
    	if (is_regular_file(*it)) {
    		all_files.push_back(*it);
    	}

        if(is_directory(*it) && is_symlink(*it))
            it.no_push();
 
        try
        {
            ++it;
        }
        catch(std::exception& ex)
        {
            std::cout << ex.what() << std::endl;
            it.no_push();
            try { ++it; } catch(...) { std::cout << "!!" << std::endl; return; }
        }
    }
}


boost::filesystem::path relative_path( const boost::filesystem::path &path, const boost::filesystem::path &relative_to )
{
    // create absolute paths
    boost::filesystem::path p = absolute(path);
    boost::filesystem::path r = absolute(relative_to);

    // if root paths are different, return absolute path
    if( p.root_path() != r.root_path() )
        return p;

    // initialize relative path
    boost::filesystem::path result;

    // find out where the two paths diverge
    boost::filesystem::path::const_iterator itr_path = p.begin();
    boost::filesystem::path::const_iterator itr_relative_to = r.begin();
    while( *itr_path == *itr_relative_to && itr_path != p.end() && itr_relative_to != r.end() ) {
        ++itr_path;
        ++itr_relative_to;
    }

    // add "../" for each remaining token in relative_to
    if( itr_relative_to != r.end() ) {
        ++itr_relative_to;
        while( itr_relative_to != r.end() ) {
            result /= "..";
            ++itr_relative_to;
        }
    }

    // add remaining path
    while( itr_path != p.end() ) {
        result /= *itr_path;
        ++itr_path;
    }

    return result;
}


fc::ripemd160 calculate_hash(path file_path) {
    file_source source(file_path.string(), std::ifstream::binary);

    char buffer[RIPEMD160_BUFFER_SIZE];
    int bytes_read = source.read(buffer, RIPEMD160_BUFFER_SIZE);
    
    fc::ripemd160::encoder ripe_calc;

    while (bytes_read > 0) {
        ripe_calc.write(buffer, bytes_read);
        bytes_read = source.read(buffer, RIPEMD160_BUFFER_SIZE);
    }

    return ripe_calc.result();
}

} // Unnamed namespace




package_object::package_object(const boost::filesystem::path& package_path) {
    _package_path = package_path;

    if (!is_directory(_package_path)) {
        _package_path = path();
        _hash = fc::ripemd160();
        return;
    }

    try {
        if (_package_path.filename() == ".") {
            _package_path = _package_path.parent_path();
        }
        _hash = fc::ripemd160(_package_path.filename().string());
    } catch (fc::exception& er) {
        _package_path = path();
        _hash = fc::ripemd160();
    }
}

void package_object::get_all_files(std::vector<boost::filesystem::path>& all_files) const {
    get_files_recursive(get_path(), all_files);
}

bool package_object::verify_hash() const {
    if (!is_valid()) {
        return false;
    }

    return _hash == calculate_hash(get_content_file());
}

int package_object::get_size() const {
    return file_size(get_content_file());
}


void package_manager::restore_json_state() {
}


void package_manager::save_json_state() {
}

package_manager::package_manager() {
    _protocol_handlers.insert(std::make_pair("magnet", std::make_shared<torrent_transfer>()));
    _protocol_handlers.insert(std::make_pair("ipfs", std::make_shared<ipfs_transfer>()));
}

void package_manager::set_packages_path(const boost::filesystem::path& packages_path) {
    if (!exists(packages_path) || !is_directory(packages_path)) {
        try {
            if (!create_directories(packages_path) && !is_directory(packages_path)) {
                FC_THROW("Unable to create packages directory");
            }
        }
        catch (const boost::filesystem::filesystem_error& ex) {
            if (!is_directory(packages_path)) {
                FC_THROW("Unable to create packages directory: ${error}", ("error", ex.what()) );
            }
        }
    }

    fc::scoped_lock<fc::mutex> guard(_mutex);
    _packages_path = packages_path;

    restore_json_state();
}


package_manager::~package_manager() {
    std::cout << "Saving package manager state..." << std::endl;
    save_json_state();
}

boost::filesystem::path package_manager::get_packages_path() const {
    fc::scoped_lock<fc::mutex> guard(_mutex);
    return _packages_path;
}

void package_manager::set_libtorrent_config(const boost::filesystem::path& libtorrent_config_file) {
    fc::scoped_lock<fc::mutex> guard(_mutex);

    _libtorrent_config_file = libtorrent_config_file;

    protocol_handler_map::iterator it = _protocol_handlers.find("magnet");
    if (it != _protocol_handlers.end()) {
        torrent_transfer* handler = dynamic_cast<torrent_transfer*>(it->second.get());
        if (handler) {
            if (_libtorrent_config_file.empty() || boost::filesystem::exists(_libtorrent_config_file)) {
                handler->reconfigure(_libtorrent_config_file);
            }
            else {
                handler->dump_config(_libtorrent_config_file);
            }
        }
    }
}

boost::filesystem::path package_manager::get_libtorrent_config() const {
    fc::scoped_lock<fc::mutex> guard(_mutex);
    return _libtorrent_config_file;
}

bool package_manager::unpack_package(const path& destination_directory, const package_object& package, const fc::sha512& key) {
    if (!package.is_valid()) {
        FC_THROW("Invalid package");
    }

    if (!is_directory(package.get_path())) {
        FC_THROW("Package path is not directory");
    }

    if (CryptoPP::AES::MAX_KEYLENGTH > key.data_size()) {
        FC_THROW("CryptoPP::AES::MAX_KEYLENGTH is bigger than key size");
    }

    if (!exists(destination_directory) || !is_directory(destination_directory)) {
        try {
            if (!create_directories(destination_directory) && !is_directory(destination_directory)) {
                FC_THROW("Unable to create destination directory");
            }
        }
        catch (const boost::filesystem::filesystem_error& ex) {
            if (!is_directory(destination_directory)) {
                FC_THROW("Unable to create destination directory: ${error}", ("error", ex.what()) );
            }
        }
    }

    fc::scoped_lock<fc::mutex> guard(_mutex);

    path archive_file = package.get_content_file();
    path temp_dir = temp_directory_path();
    path zip_file = temp_dir / "content.zip";

    decent::crypto::aes_key k;
    for (int i = 0; i < CryptoPP::AES::MAX_KEYLENGTH; i++)
      k.key_byte[i] = key.data()[i];

    if (space(temp_dir).available < file_size(archive_file) * 1.5) { // Safety margin
        FC_THROW("Not enough storage space to create package");
    }

    AES_decrypt_file(archive_file.string(), zip_file.string(), k);

    filtering_istream istr;
    istr.push(gzip_decompressor());
    istr.push(file_source(zip_file.string(), std::ifstream::binary));

    dearchiver dearc(istr);
    dearc.extract(destination_directory.string());

	return true;
}

package_object package_manager::create_package( const boost::filesystem::path& content_path, const boost::filesystem::path& samples, const fc::sha512& key, decent::crypto::custody_data& cd) {
	if (!is_directory(content_path) && !is_regular_file(content_path)) {
        FC_THROW("Content path is not directory or file");
	}

	//if (!is_directory(samples) || samples.size() == 0) {
    //    FC_THROW("Samples path is not directory");
	//}

    const path packages_path = get_packages_path();

	path temp_path = packages_path / make_uuid();
	if (!create_directory(temp_path)) {
        FC_THROW("Failed to create temporary directory");
	}

    if (CryptoPP::AES::MAX_KEYLENGTH > key.data_size()) {
        FC_THROW("CryptoPP::AES::MAX_KEYLENGTH is bigger than key size");
    }


	path content_zip = temp_path / "content.zip";

	filtering_ostream out;
    out.push(gzip_compressor());
    out.push(file_sink(content_zip.string(), std::ofstream::binary));
	archiver arc(out);

	vector<path> all_files;
    if (is_regular_file(content_path)) {
       all_files.push_back(content_path);
    } else {
	   get_files_recursive(content_path, all_files); 
    }
    
	for (int i = 0; i < all_files.size(); ++i) {
        file_source source(all_files[i].string(), std::ifstream::binary);
        
		arc.put(relative_path(all_files[i], content_path).string(), source, file_size(all_files[i]));
	}

	arc.finalize();


    path aes_file_path = temp_path / "content.zip.aes";

    decent::crypto::aes_key k;
    for (int i = 0; i < CryptoPP::AES::MAX_KEYLENGTH; i++)
      k.key_byte[i] = key.data()[i];


    if (space(temp_path).available < file_size(content_zip) * 1.5) { // Safety margin
        FC_THROW("Not enough storage space to create package");
    }

    AES_encrypt_file(content_zip.string(), aes_file_path.string(), k);
    remove(content_zip);

    {
        fc::scoped_lock<fc::mutex> guard(_mutex);
        _custody_utils.create_custody_data(aes_file_path, cd);
    }

    fc::ripemd160 hash = calculate_hash(aes_file_path);
    rename(temp_path, packages_path / hash.str());

	return package_object(packages_path / hash.str());
}
	
package_transfer_interface::transfer_id
package_manager::upload_package( const package_object& package, 
                                 const string& protocol_name,
                                 package_transfer_interface::transfer_listener& listener ) {

    fc::scoped_lock<fc::mutex> guard(_mutex);

    protocol_handler_map::iterator it = _protocol_handlers.find(protocol_name);
    if (it == _protocol_handlers.end()) {
        FC_THROW("Can not find protocol handler for : ${proto}", ("proto", protocol_name) );
    }

    _all_transfers.push_back(transfer_job());
    transfer_job& t = _all_transfers.back();

    t.job_id = _all_transfers.size() - 1;
    t.transport = it->second->clone();
    t.listener = &listener;
    t.job_type = transfer_job::UPLOAD;

    try {
        t.transport->upload_package(t.job_id, package, &listener);
    } catch(std::exception& ex) {
        std::cout << "Upload error: " << ex.what() << std::endl;
    }

    return t.job_id;
}

package_transfer_interface::transfer_id 
package_manager::download_package( const string& url,
                                   package_transfer_interface::transfer_listener& listener ) {

    fc::scoped_lock<fc::mutex> guard(_mutex);
    fc::url download_url(url);

    protocol_handler_map::iterator it = _protocol_handlers.find(download_url.proto());
    if (it == _protocol_handlers.end()) {
        FC_THROW("Can not find protocol handler for : ${proto}", ("proto", download_url.proto()) );
    }

    _all_transfers.push_back(transfer_job());
    transfer_job& t = _all_transfers.back();

    t.job_id = _all_transfers.size() - 1;
    t.transport = it->second->clone();
    t.listener = &listener;
    t.job_type = transfer_job::DOWNLOAD;

    try {
        t.transport->download_package(t.job_id, url, &listener);
    } catch(std::exception& ex) {
        std::cout << "Download error: " << ex.what() << std::endl;
    }

    return t.job_id;
}

void package_manager::print_all_transfers() {
    fc::scoped_lock<fc::mutex> guard(_mutex);
    for (int i = 0; i < _all_transfers.size(); ++i) {
        const transfer_job& job = _all_transfers[i];
        cout << "~~~ Status for job #" << job.job_id << " [" << ((job.job_type == transfer_job::UPLOAD) ? "Upload" : "Download") << "]\n";
        job.transport->print_status();
        cout << "~~~ End of status for #" << job.job_id << endl;
    }
}

package_transfer_interface::transfer_progress 
package_manager::get_progress(std::string URI) const {
    fc::scoped_lock<fc::mutex> guard(_mutex);
    for (int i = 0; i < _all_transfers.size(); ++i) {
        const transfer_job& job = _all_transfers[i];
        string transfer_url = job.transport->get_transfer_url();
        if (transfer_url == URI) {
            return job.transport->get_progress();
        }
    }

    return package_transfer_interface::transfer_progress(0, 0, 0);
}

std::string package_manager::get_transfer_url(package_transfer_interface::transfer_id id) {
    fc::scoped_lock<fc::mutex> guard(_mutex);
    if (id >= _all_transfers.size()) {
        FC_THROW("Invalid transfer id: ${id}", ("id", id) );
    }

    transfer_job& job = _all_transfers[id];
    return job.transport->get_transfer_url();
}


std::vector<package_object> package_manager::get_packages() {
    fc::scoped_lock<fc::mutex> guard(_mutex);

    std::vector<package_object> all_packages;
    directory_iterator it(_packages_path), it_end;
    for (; it != it_end; ++it) {
        if (is_directory(*it)) {
            all_packages.push_back(package_object(it->path().string()));
        }
    }
    return all_packages;
}

package_object package_manager::get_package_object(fc::ripemd160 hash) {
    const path packages_path = get_packages_path();
    return package_object(packages_path / hash.str());
}


void package_manager::delete_package(fc::ripemd160 hash) {
    const path packages_path = get_packages_path();
    package_object po(packages_path / hash.str());
    if (!po.is_valid()) {
        FC_THROW("Invalid package: ${hash}", ("hash", hash.str()) );
    }

    remove_all(po.get_path());
}

uint32_t package_manager::create_proof_of_custody(const boost::filesystem::path& content_file, const decent::crypto::custody_data& cd, decent::crypto::custody_proof&proof) {
    fc::scoped_lock<fc::mutex> guard(_mutex);
    return _custody_utils.create_proof_of_custody(content_file, cd, proof);
}

uint32_t package_object::create_proof_of_custody(const decent::crypto::custody_data& cd, decent::crypto::custody_proof& proof) const {
   return package_manager::instance().create_proof_of_custody(get_content_file(), cd, proof);
}
