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



#include <graphene/package/package.hpp>

#include <fc/exception/exception.hpp>
#include <fc/network/ntp.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>
#include <iostream>
#include <atomic>

#include <decent/encrypt/encryptionutils.hpp>

#include "aes_filter.hpp"

using namespace graphene::package;
using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::iostreams;


namespace {

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
        
        char buffer[4096];
        int bytes_read = in.read(buffer, 4096);
        
        while (bytes_read > 0) {
            _out.write(buffer, bytes_read);
            bytes_read = in.read(buffer, 4096);
        }
        
        return true;
	}

	void finalize() {
		arc_header header;

		std::memset((void*)&header, 0, sizeof(arc_header));
		_out.write((const char*)&header,sizeof(arc_header));
		_out.flush();        
	}

};




class dearchiver {
	filtering_istream&       _in;

public:
	dearchiver(filtering_istream& in): _in(in) {

	}

	bool extract(const std::string& output_path, string* error) {
		arc_header header;

        while (true) {
            std::memset((void*)&header, 0, sizeof(arc_header));
            _in.read((char*)&header, sizeof(arc_header));
            if (header.type == 0) {
                break;
            }
            
            path file_path(header.name);
            path file_location = output_path / file_path.parent_path();

            cout << "Location: " << file_location << endl;
            
            cout << "File name: " << header.name << endl;
            cout << "File size: " << *(int*)header.size << endl;
            
            if (!is_directory(file_location) && !create_directories(file_location)) {
                if (error) *error = "Unable to create directory";
                return false;
            }
            
            std::fstream sink((output_path / file_path).string(), ios::out | ios::binary);
            
            int bytes_to_read = *(int*)header.size;
            char buffer[4096];
            
            int bytes_read = boost::iostreams::read(_in, buffer, std::min(4096, bytes_to_read));
            
            while (bytes_read > 0 && bytes_to_read > 0) {

                sink.write(buffer, bytes_read);
                bytes_to_read -= bytes_read;
                if (bytes_to_read == 0) {
                    break;
                }
                bytes_read = boost::iostreams::read(_in, buffer, std::min(4096, bytes_to_read));
            }
            
            if (bytes_read < 0 && bytes_to_read > 0) {
                if (error) *error = "Unexpected end of file";
                return false;
            }
            
            sink.close();
        }
        
        return true;
        
	}


};




string make_uuid() {
	const int length = 32;

    static string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
    string result;
    result.resize(length);

    srand(time(NULL));
    for (int i = 0; i < length; i++)
        result[i] = charset[rand() % charset.length()];

    return result;
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



}



package_manager::package_manager(const path& content_path, const path& samples, const fc::sha512& key) 
	: _content_path(content_path), _samples(samples), _package_key(key) {

}

package_manager::package_manager(const path& package_path) : _package_path(package_path) {

}

	
bool package_manager::unpack_package(const path& destination_directory, const fc::sha512& key, std::string* error) {
	if (!is_directory(destination_directory)) {
		if (error) 
			*error = "Destination directory not found";
		
		return false;
	}
	if (!is_directory(_package_path)) {
		if (error)
			*error = "Package path is not directory";
		
		return false;
	}
	
	path archive_file = _package_path / "content.zip.aes";
    
    
    filtering_istream istr;
    istr.push(gzip_decompressor());
    //istr.push(aes_decrypter(key));
    
    istr.push(file_source(archive_file.string(), std::ifstream::binary));
    
    dearchiver dearc(istr);
    if (!dearc.extract(destination_directory.string(), error)) {
        return false;
    }


	return true;
}

bool package_manager::create_package(const path& destination_directory, std::string* error) {
	if (!is_directory(destination_directory)) {
		if (error)
			*error = "Destination directory not found";
		
		return false;
	}
	if (!is_directory(_content_path) && !is_regular_file(_content_path)) {
		if (error)
			*error = "Content path is not directory or file";
		
		return false;
	}
	if (!is_directory(_samples) || _samples.size() == 0) {
		if (error)
			*error = "Samples path is not directory";
		
		return false;
	}

	path tempPath = destination_directory / make_uuid();
	if (!create_directory(tempPath)) {
		if (error)
			*error = "Failed to create temporary directory";
		
		return false;
	}


	path content_zip = tempPath / "content.zip";
	cout << tempPath << "\n";

	filtering_ostream out;
    out.push(gzip_compressor());
    //out.push(aes_encryptor(_package_key));
    out.push(file_sink(content_zip.string(), std::ofstream::binary));
	archiver arc(out);

	vector<path> all_files;
	get_files_recursive(_content_path, all_files);
	for (int i = 0; i < all_files.size(); ++i) {
        file_source source(all_files[i].string(), std::ifstream::binary);
        
		arc.put(relative_path(all_files[i], _content_path).string(), source, file_size(all_files[i]));
	}

	arc.finalize();


    path aes_file_path = tempPath / "content.zip.aes";

    decent::crypto::aes_key k;
    for (int i = 0; i < CryptoPP::AES::MAX_KEYLENGTH; i++)
      k.key_byte[i] = i;


    AES_encrypt_file(content_zip.string(), aes_file_path.string(), k);
    //remove(content_zip);


	return true;
}
	
const package_manager::path& package_manager::get_package_path() const {
	return _package_path;
}

const package_manager::path& package_manager::get_custody_file() {
	return _custody_file;
}

const package_manager::path& package_manager::get_content_file() {
	return _content_file;
}

const package_manager::path& package_manager::get_samples_path() {
    return package_manager::path();
}


bool package_manager::verify_hash() const {
	return true;
}

fc::ripemd160 
package_manager::get_hash() const {
	return fc::ripemd160();
}


