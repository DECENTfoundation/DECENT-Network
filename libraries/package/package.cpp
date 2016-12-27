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

#include <graphene/package/package.hpp>

#include <fc/exception/exception.hpp>
#include <fc/network/ntp.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>

#include <atomic>

using namespace graphene::package;
using namespace boost::filesystem;


PackageManager::PackageManager(const Path& contentPath, const Path& samples, const fc::sha512& key) 
	: _contentPath(contentPath), _samples(samples), _packageKey(key) {

}

PackageManager::PackageManager(const Path& packagePath) : _packagePath(packagePath) {

}

	
bool PackageManager::unpackPackage(const Path& destinationDirectory, const fc::sha512& key, std::string* error) {
	return false;
}

bool PackageManager::createPackage(const Path& destinationDirectory, std::string* error) {
	if (!is_directory(destinationDirectory)) {
		if (error)
			*error = "Destination directory not found";
		
		return false;
	}
	if (!is_directory(_contentPath) && !is_regular_file(_contentPath)) {
		if (error)
			*error = "Content path is not directory or file";
		
		return false;
	}
	if (!is_directory(_samples) || _samples.size() == 0) {
		if (error)
			*error = "Samples path is not directory";
		
		return false;
	}

	path tempPath = temp_directory_path();


	return false;
}
	
const PackageManager::Path& PackageManager::getPackagePath() const {
	return _packagePath;
}

const PackageManager::Path& PackageManager::getCustodyFile() {
	return _custodyFile;
}

const PackageManager::Path& PackageManager::getContentFile() {
	return _contentFile;
}

const PackageManager::Path& PackageManager::getSamplesPath() {
    return PackageManager::Path();
}


bool PackageManager::verifyHash() const {
	return true;
}

fc::ripemd160 
PackageManager::getHash() const {
	return fc::ripemd160();
}


