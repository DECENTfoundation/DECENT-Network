
#pragma once

#include <fc/optional.hpp>
#include <fc/signals.hpp>
#include <fc/time.hpp>

#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/sha512.hpp>

#include <fc/network/url.hpp>

#include <boost/filesystem.hpp>
#include <decent/encrypt/crypto_types.hpp>
#include <decent/encrypt/custodyutils.hpp>


namespace graphene { 
namespace package {

class package_object {
public:
	package_object(const boost::filesystem::path& package_path);

	boost::filesystem::path get_custody_file() const { return _package_path / "content.cus"; }
	boost::filesystem::path get_content_file() const { return _package_path / "content.zip.aes"; }
	boost::filesystem::path get_samples_path() const { return _package_path / "samples"; }
	const boost::filesystem::path& get_path() const { return _package_path; }

	void get_all_files(std::vector<boost::filesystem::path>& all_files) const;

	bool verify_hash() const;

	fc::ripemd160 get_hash() const { return _hash; }

	bool is_valid() const { return _hash != fc::ripemd160(); }

	uint32_t create_proof_of_custody(decent::crypto::custody_data cd, decent::crypto::custody_proof& proof) const;

private:
	boost::filesystem::path   _package_path;
	fc::ripemd160			  _hash;
};



class package_transfer_interface {
public:
	typedef int   transfer_id;

public:

	struct transfer_progress {
		transfer_progress() {}
		transfer_progress(int tb, int cb, int cs) : total_bytes(tb), current_bytes(cb), current_speed(cs) {}

		int total_bytes;
		int current_bytes;
		int current_speed; // Bytes per second
	};

	class transfer_listener {
	public:
		virtual void on_download_started(transfer_id id) = 0;
		virtual void on_download_finished(transfer_id id, package_object downloaded_package) = 0;
		virtual void on_download_progress(transfer_id id, transfer_progress progress) = 0;

		virtual void on_upload_started(transfer_id id, const std::string& url) = 0;

		virtual void on_error(transfer_id id, std::string error) = 0;
	};


public:
	virtual void upload_package(transfer_id id, const package_object& package, transfer_listener* listener) = 0;
	virtual void download_package(transfer_id id, const std::string& url, transfer_listener* listener) = 0;
	virtual void print_status() = 0;

	virtual std::string       get_transfer_url() = 0;

	virtual package_transfer_interface* clone() = 0;
};


class package_manager {
private:

	struct transfer_job {
		enum transfer_type {
			DOWNLOAD,
			UPLOAD
		};

		package_transfer_interface* 					transport;
		package_transfer_interface::transfer_listener*  listener;	
		package_transfer_interface::transfer_id			job_id;	
		transfer_type									job_type;
	};

private:

	typedef std::map<std::string, package_transfer_interface*> 		protocol_handler_map;
	typedef std::vector<transfer_job> 								transfer_jobs;
private:
	package_manager();
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
								   const fc::sha512& key,
                          		   decent::crypto::custody_data& cd);

	bool unpack_package( const boost::filesystem::path& destination_directory, 
						 const package_object& package,
						 const fc::sha512& key);



	package_transfer_interface::transfer_id upload_package( const package_object& package, 
															const std::string& protocol_name,
															package_transfer_interface::transfer_listener& listener );

	package_transfer_interface::transfer_id download_package( const std::string& url,
															  package_transfer_interface::transfer_listener& listener );
	
	std::vector<package_object> get_packages();
	package_object				get_package_object(fc::ripemd160 hash);
	void                        delete_package(fc::ripemd160 hash);

	std::string					get_transfer_url(package_transfer_interface::transfer_id id);


	decent::crypto::custody_utils& get_custody_utils() { return _custody_utils; }
	boost::filesystem::path get_packages_path() const { return _packages_directory; }
	
	void print_all_transfers();

private:
	boost::filesystem::path            _packages_directory;
	decent::crypto::custody_utils      _custody_utils;
	protocol_handler_map               _protocol_handlers;
	transfer_jobs					   _all_transfers;
};






} 
}
