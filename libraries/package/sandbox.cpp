#include <cstddef>

#include <graphene/package/package.hpp>

#include <boost/interprocess/sync/named_recursive_mutex.hpp>

#include <thread>
#include <iostream>


using namespace decent::package;


std::ostream& operator<< (std::ostream& os, const PackageInfo::DataState state) {
    switch (state) {
        case PackageInfo::DS_UNINITIALIZED:  os << "UNINITIALIZED";  break;
        case PackageInfo::INVALID:           os << "INVALID";        break;
        case PackageInfo::PARTIAL:           os << "PARTIAL";        break;
        case PackageInfo::UNCHECKED:         os << "UNCHECKED";      break;
        case PackageInfo::CHECKED:           os << "CHECKED";        break;
        default:                             os << "???";            break;
    }
    return os;
}

std::ostream& operator<< (std::ostream& os, const PackageInfo::TransferState state) {
    switch (state) {
        case PackageInfo::TS_IDLE:      os << "IDLE";         break;
        case PackageInfo::DOWNLOADING:  os << "DOWNLOADING";  break;
        case PackageInfo::SEEDING:      os << "SEEDING";      break;
        default:                        os << "???";          break;
    }
    return os;
}

std::ostream& operator<< (std::ostream& os, const PackageInfo::ManipulationState state) {
    switch (state) {
        case PackageInfo::MS_IDLE:     os << "IDLE";        break;
        case PackageInfo::PACKING:     os << "PACKING";     break;
        case PackageInfo::ENCRYPTING:  os << "ENCRYPTING";  break;
        case PackageInfo::STAGING:     os << "STAGING";     break;
        case PackageInfo::CHECKING:    os << "CHECKING";    break;
        case PackageInfo::DECRYPTING:  os << "DECRYPTING";  break;
        case PackageInfo::UNPACKING:   os << "UNPACKING";   break;
        case PackageInfo::DELETTING:   os << "DELETTING";   break;
        default:                       os << "???";         break;
    }
    return os;
}


class MyEventListener : public EventListenerInterface {
public:

    virtual void package_data_state_change(PackageInfo::DataState new_state)                  { std::clog << "Data state changed: " << new_state << std::endl; }
    virtual void package_transfer_state_change(PackageInfo::TransferState new_state)          { std::clog << "Transfer state changed: " << new_state << std::endl; }
    virtual void package_manipulation_state_change(PackageInfo::ManipulationState new_state)  { std::clog << "Manipulation state changed: " << new_state << std::endl; }

    virtual void package_creation_start()                          { std::clog << "Package creation started" << std::endl; }
    virtual void package_creation_progress()                       { std::clog << "Package creation progress" << std::endl; }
    virtual void package_creation_error(const std::string& error)  { std::clog << "Package creation error: " << error << std::endl; }
    virtual void package_creation_complete()                       { std::clog << "Package creation complete" << std::endl; }

    virtual void package_restoration_start()                          { std::clog << "Package restoration started" << std::endl; }
    virtual void package_restoration_progress()                       { std::clog << "Package restoration progress" << std::endl; }
    virtual void package_restoration_error(const std::string& error)  { std::clog << "Package restoration error: " << error << std::endl; }
    virtual void package_restoration_complete()                       { std::clog << "Package restoration complete" << std::endl; }

    virtual void package_extraction_start()                          { std::clog << "Package extraction started" << std::endl; }
    virtual void package_extraction_progress()                       { std::clog << "Package extraction progress" << std::endl; }
    virtual void package_extraction_error(const std::string& error)  { std::clog << "Package extraction error: " << error << std::endl; }
    virtual void package_extraction_complete()                       { std::clog << "Package extraction complete" << std::endl; }

    virtual void package_check_start()                          { std::clog << "Package check started" << std::endl; }
    virtual void package_check_progress()                       { std::clog << "Package check progress" << std::endl; }
    virtual void package_check_error(const std::string& error)  { std::clog << "Package check error: " << error << std::endl; }
    virtual void package_check_complete()                       { std::clog << "Package check complete" << std::endl; }

    virtual void package_seed_start() {}
    virtual void package_seed_progress() {}
    virtual void package_seed_error(const std::string& error) { std::clog << "Package seed error: " << error << std::endl; }
    virtual void package_seed_complete() {}

    virtual void package_download_start() {}
    virtual void package_download_progress() {}
    virtual void package_download_error(const std::string&) {}
    virtual void package_download_complete() {}
};



void pm_sandbox()
{

    auto& package_manager = decent::package::PackageManager::instance();

    package_manager.recover_all_packages();


    const boost::filesystem::path content_dir = "/tmp/test/test1_content";
    const boost::filesystem::path samples_dir = "/tmp/test/test1_samples";
    const boost::filesystem::path dest_dir = "/tmp/test/test1_unpacked";
    const fc::sha256 key = fc::sha256::hash(std::string("some_string_to_use_as_a_key"));
    const bool block = false;


    {
        auto package_handle = package_manager.get_package(content_dir, samples_dir, key);
        package_handle->add_event_listener(std::make_shared<MyEventListener>());

        {
            package_handle->create(block);
            package_handle->wait_for_current_task();
            auto ex_ptr = package_handle->get_task_last_error();
            if (ex_ptr) {
                std::rethrow_exception(ex_ptr);
            }
        }

        {
            std::cerr << "starting seeding"<<std::endl;
            package_handle->start_seeding("ipfs");
            package_handle->wait_for_current_task();
            auto ex_ptr = package_handle->get_task_last_error();
            std::cerr << "started seeding"<<std::endl;
            std::cerr << "Package url:" << package_handle->get_url() << std::endl;
            if (ex_ptr) {
                std::rethrow_exception(ex_ptr);
            }
        }

        // It is assumed that the package is now available for download from elsewhere.
        std::this_thread::sleep_for(std::chrono::seconds(20));

        {
            package_handle->stop_seeding();
            package_handle->wait_for_current_task();
            auto ex_ptr = package_handle->get_task_last_error();
            if (ex_ptr) {
                std::rethrow_exception(ex_ptr);
            }
        }

        const boost::filesystem::path package_dir = package_handle->get_package_dir();
        const std::string url = package_handle->get_url();


        package_manager.release_package(package_handle);


        package_handle = package_manager.get_package("/ipfs/QmWgZbg73wrgicmPradJcK51nY99o2fX8dt7pBJ8rUaurJ");
        package_handle->add_event_listener(std::make_shared<MyEventListener>());

        {
            package_handle->download();
            package_handle->wait_for_current_task();
            auto ex_ptr = package_handle->get_task_last_error();
            if (ex_ptr) {
                std::rethrow_exception(ex_ptr);
            }
        }

        {
            package_handle->check(block);
            package_handle->wait_for_current_task();
            auto ex_ptr = package_handle->get_task_last_error();
            if (ex_ptr) {
                std::rethrow_exception(ex_ptr);
            }
        }

        {
            package_handle->unpack(dest_dir, key, block);
            package_handle->wait_for_current_task();
            auto ex_ptr = package_handle->get_task_last_error();
            if (ex_ptr) {
                std::rethrow_exception(ex_ptr);
            }
        }
    }

    package_manager.release_all_packages();


/*
    const boost::filesystem::path src_content_path = "/tmp/src_content";
    const boost::filesystem::path package_path = "/tmp/dst_package";
    const boost::filesystem::path dst_content_path = "/tmp/dst_content";

    remove_all(package_path);
    remove_all(dst_content_path);

    boost::filesystem::create_directories(src_content_path);
    boost::filesystem::create_directories(package_path);
    boost::filesystem::create_directories(dst_content_path);

    const boost::filesystem::path package_content_zip = package_path / "content.zip";
    const boost::filesystem::path package_content_zip_aes = package_path / "content.zip.aes";

    const boost::filesystem::path dst_content_zip = dst_content_path / "content.zip";

    const std::string key = "A123456";

    {
        boost::iostreams::filtering_ostream out;
        out.push(boost::iostreams::gzip_compressor());
        out.push(boost::iostreams::file_sink(package_content_zip.string(), std::ios::out | std::ios::binary));

        archiver arc(out);

        std::vector<path> all_files;
        if (boost::filesystem::is_regular_file(src_content_path)) {
            arc.put(src_content_path.filename().string(), src_content_path);
        } else {
            get_files_recursive(src_content_path, all_files);

            for (int i = 0; i < all_files.size(); ++i) {
                arc.put(relative_path(all_files[i], src_content_path).string(), all_files[i]);
            }
        }

        arc.finalize();

        decent::encrypt::AesKey k;

        for (int i = 0; i < CryptoPP::AES::MAX_KEYLENGTH; i++)
            k.key_byte[i] = key.data()[i];

        AES_encrypt_file(package_content_zip.string(), package_content_zip_aes.string(), k);
        remove(package_content_zip);

        fc::ripemd160 hash = calculate_hash(package_content_zip_aes);

        boost::filesystem::create_directories(package_path / hash.str());
    }


    {
        decent::encrypt::AesKey k;

        for (int i = 0; i < CryptoPP::AES::MAX_KEYLENGTH; i++)
            k.key_byte[i] = key.data()[i];

        AES_decrypt_file(package_content_zip_aes.string(), package_content_zip.string(), k);

        boost::iostreams::filtering_istream istr;
        istr.push(boost::iostreams::gzip_decompressor());
        istr.push(boost::iostreams::file_source(package_content_zip.string(), std::ios::in | std::ios::binary));

        dearchiver dearc(istr);
        dearc.extract(dst_content_path.string());
    }
*/
}

int main(int argc, const char* argv[]) {
    try {

        pm_sandbox();

    }
    catch (const fc::exception& ex) {
        std::cerr << ex.to_detail_string() << std::endl;
    }
    catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown exception has been thrown" << std::endl;
    }

    return 0;
}
