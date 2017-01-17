
#pragma once

#include <fc/optional.hpp>
#include <fc/signals.hpp>
#include <fc/time.hpp>

#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/sha512.hpp>

#include <boost/filesystem.hpp>


namespace graphene { 
namespace package {

class package_object {
public:
	package_object(const boost::filesystem::path& package_path) : _package_path(package_path) {}

	boost::filesystem::path get_custody_file() const { return _package_path / "content.cus"; }
	boost::filesystem::path get_content_file() const { return _package_path / "content.zip.aes"; }
	boost::filesystem::path get_samples_path() const { return _package_path / "samples"; }
	const boost::filesystem::path& get_path() const { return _package_path; }


	bool verify_hash() const { return false; } //TODO: Implement this
	fc::ripemd160 get_hash() const { return fc::ripemd160(); } //TODO: Implement this

private:
	boost::filesystem::path   _package_path;
};



class package_transfer_interface {
public:
	typedef int   transfer_id;

public:

	struct transfer_progress {
		int total_bytes;
		int current_bytes;
		int current_speed; // Bytes per second
	};

	class transfer_listener {
		virtual void on_download_started(transfer_id id) = 0;
		virtual void on_download_finished(transfer_id id, package_object downloaded_package) = 0;
		virtual void on_download_progress(transfer_id id, transfer_progress progress) = 0;

		virtual void on_upload_started(transfer_id id) = 0;
		virtual void on_upload_finished(transfer_id id) = 0;
		virtual void on_upload_progress(transfer_id id, transfer_progress progress) = 0;
	};


public:
	virtual transfer_id upload_package(const package_object& package, transfer_listener& listener) = 0;
	virtual transfer_id download_package(const package_object& package, transfer_listener& listener) = 0;
};


class package_manager {

private:
	package_manager() {}
	package_manager(const package_manager&) {}

public:

	static package_manager& instance() {
		static package_manager pac_man;
		return pac_man;
	}

public:

	void initialize( const boost::filesystem::path& packages_directory);

	package_object create_package( const boost::filesystem::path& content_path, 
								   const boost::filesystem::path& samples, 
								   const fc::sha512& key);

	bool unpack_package( const boost::filesystem::path& destination_directory, 
						 const package_object& package,
						 const fc::sha512& key);


	package_transfer_interface::transfer_id upload_package( const package_object& package, 
															package_transfer_interface& protocol,
															package_transfer_interface::transfer_listener& listener );

	package_transfer_interface::transfer_id download_package( const package_object& package, 
															  package_transfer_interface& protocol,
															  package_transfer_interface::transfer_listener& listener );
	
	std::vector<std::string> get_packages();
private:
	boost::filesystem::path      _packages_directory;
};






} 
}
