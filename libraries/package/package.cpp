#include <cstddef>
#include "torrent_transfer.hpp"
#include "ipfs_transfer.hpp"
#include "local.hpp"

#include <decent/encrypt/encryptionutils.hpp>
#include <decent/package/package.hpp>

#include <fc/log/logger.hpp>
#include <fc/thread/scoped_lock.hpp>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <atomic>
#include <thread>
#include <vector>


using namespace boost;
using namespace boost::filesystem;


namespace decent { namespace package {


    namespace detail {


        struct ArchiveHeader {
            char type;       // 0 = EOF, 1 = REGULAR FILE
            char name[255];
            char size[8];
        };


        class Archiver {
        public:
            explicit Archiver(boost::iostreams::filtering_ostream& out)
                : _out(out)
            {
            }

            bool put(const std::string& file_name, const boost::filesystem::path& source_file_path) {
                boost::iostreams::file_source in(source_file_path.string(), std::ios_base::in | std::ios_base::binary);

                if (!in.is_open()) {
                    FC_THROW("Unable to open file ${file} for reading", ("file", source_file_path.string()) );
                }

                const int file_size = boost::filesystem::file_size(source_file_path);

                ArchiveHeader header;

                std::memset((void*)&header, 0, sizeof(header));

                std::snprintf(header.name, sizeof(header.name), "%s", file_name.c_str());

                header.type = 1;
                *(int*)header.size = file_size;

                _out.write((const char*)&header, sizeof(header));

                boost::iostreams::stream<boost::iostreams::file_source> is(in);
                _out << is.rdbuf();

                return true;
            }

            ~Archiver() {
                ArchiveHeader header;

                std::memset((void*)&header, 0, sizeof(header));
                _out.write((const char*)&header, sizeof(header));
            }

        private:
            boost::iostreams::filtering_ostream& _out;
        };


        class Dearchiver {
        public:
            explicit Dearchiver(boost::iostreams::filtering_istream& in)
                : _in(in)
            {
            }

            bool extract(const boost::filesystem::path& output_dir) {
                while (true) {
                    ArchiveHeader header;

                    std::memset((void*)&header, 0, sizeof(header));

                    _in.read((char*)&header, sizeof(header));

                    // FIXME: this masks the bug when file is corrupted and header type is a random byte.
                    if (header.type != 1 || strlen(header.name) == 0) { // if (header.type == 0) {
                        break;
                    }

                    using namespace boost::filesystem;

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

                    std::fstream sink(file_path.string(), std::ios::out | std::ios::binary);

                    if (!sink.is_open()) {
                        FC_THROW("Unable to open file ${file} for writing", ("file", file_path.string()) );
                    }

                    const int bytes_to_read = *(int*)header.size;

                    if (bytes_to_read < 0) {
                        FC_THROW("Unexpected size in header");
                    }

                    std::copy_n(std::istreambuf_iterator<char>(_in),
                                bytes_to_read,
                                std::ostreambuf_iterator<char>(sink)
                    );
                }
                
                return true;
            }

        private:
            boost::iostreams::filtering_istream& _in;
        };


    } // namespace detail


    namespace detail {


        class CreatePackageTask : public PackageTask {
        public:
            CreatePackageTask(PackageInfo& package,
                              PackageManager& manager,
                              const boost::filesystem::path& content_dir_path,
                              const boost::filesystem::path& samples_dir_path,
                              const fc::sha256& key)
                : PackageTask(package)
                , _content_dir_path(content_dir_path)
                , _samples_dir_path(samples_dir_path)
                , _key(key)
            {
            }

        protected:
            virtual void task() override {
                PACKAGE_INFO_GENERATE_EVENT(package_creation_start, ( ) );

                using namespace boost::filesystem;

                const auto temp_dir_path = unique_path(graphene::utilities::decent_path_finder::instance().get_decent_temp() / "%%%%-%%%%-%%%%-%%%%");
                bool samples = false;

                try {
                    PACKAGE_TASK_EXIT_IF_REQUESTED;

                    if (CryptoPP::AES::MAX_KEYLENGTH > _key.data_size()) {
                        FC_THROW("CryptoPP::AES::MAX_KEYLENGTH is bigger than key size (${size})", ("size", _key.data_size()) );
                    }

                    if (!is_directory(_content_dir_path) && !is_regular_file(_content_dir_path)) {
                        FC_THROW("Content path ${path} must point to either directory or file", ("path", _content_dir_path.string()) );
                    }

                    if (exists(_samples_dir_path) && !is_directory(_samples_dir_path)) {
                        FC_THROW("Samples path ${path} must point to directory", ("path", _samples_dir_path.string()));
                    }else{
                        samples = !_samples_dir_path.empty();
                    }

                    if (exists(temp_dir_path) || !create_directory(temp_dir_path)) {
                        FC_THROW("Failed to create unique temporary directory ${path}", ("path", temp_dir_path.string()) );
                    }


//                  PACKAGE_INFO_GENERATE_EVENT(package_creation_progress, ( ) );


                    create_directories(temp_dir_path);
                    remove_all(temp_dir_path);
                    create_directories(temp_dir_path);

                    PACKAGE_INFO_CHANGE_MANIPULATION_STATE(PACKING);

                    const auto zip_file_path = temp_dir_path / "content.zip";

                    {
                        using namespace boost::iostreams;

                        filtering_ostream out;
                        out.push(gzip_compressor());
                        out.push(file_sink(zip_file_path.string(), std::ios::out | std::ios::binary));

                        detail::Archiver archiver(out);

                        if (is_regular_file(_content_dir_path)) {
                            PACKAGE_TASK_EXIT_IF_REQUESTED;
                            archiver.put(_content_dir_path.filename().string(), _content_dir_path);
                        } else {
                            std::vector<path> all_files;
                            detail::get_files_recursive(_content_dir_path, all_files);
                            for (auto& file : all_files) {
                                PACKAGE_TASK_EXIT_IF_REQUESTED;
                                archiver.put(detail::get_relative(_content_dir_path, file).string(), file);
                            }
                        }
                    }

                    PACKAGE_TASK_EXIT_IF_REQUESTED;

                    if (space(temp_dir_path).available < file_size(zip_file_path) * 1.5) { // Safety margin.
                        FC_THROW("Not enough storage space in ${path} to create package", ("path", temp_dir_path.string()) );
                    }

                   uint64_t size = 0;

                    {
                        PACKAGE_INFO_CHANGE_MANIPULATION_STATE(ENCRYPTING);

                        const auto aes_file_path = temp_dir_path / "content.zip.aes";

                        decent::encrypt::AesKey k;
                        for (int i = 0; i < CryptoPP::AES::MAX_KEYLENGTH; i++) {
                            k.key_byte[i] = _key.data()[i];
                        }

                        PACKAGE_TASK_EXIT_IF_REQUESTED;
                        AES_encrypt_file(zip_file_path.string(), aes_file_path.string(), k);
                        PACKAGE_TASK_EXIT_IF_REQUESTED;
                        _package._hash = detail::calculate_hash(aes_file_path);
                        PACKAGE_TASK_EXIT_IF_REQUESTED;
                        //calculate custody...
                        decent::encrypt::CustodyUtils::instance().create_custody_data(aes_file_path, _package._custody_data);
                        size += file_size( aes_file_path );
                        const auto cus_file_path = temp_dir_path / "content.cus";
                        size += file_size( cus_file_path );
                    }

                    if( samples ){
                        const auto temp_samples_dir_path = temp_dir_path / "Samples";

                        create_directories(temp_samples_dir_path);
                        remove_all(temp_samples_dir_path);
                        create_directories(temp_samples_dir_path);

                        for( directory_iterator file(_samples_dir_path); file != directory_iterator(); ++file ){
                            path current (file->path());
                            if (is_regular_file(current)){
                                copy_file(current, temp_samples_dir_path / current.filename() );
                                size += file_size( temp_samples_dir_path / current.filename() );
                            }
                        }
                    }

                    PACKAGE_INFO_CHANGE_MANIPULATION_STATE(STAGING);

                    const auto package_dir = _package.get_package_dir();

                    if (exists(package_dir)) {
                        wlog("overwriting existing path ${path}", ("path", package_dir.string()) );

                        if (!is_directory(package_dir)) {
                            remove_all(package_dir);
                        }
                    }

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
                    paths_to_skip.insert(zip_file_path);
                    detail::move_all_except(temp_dir_path, package_dir, paths_to_skip);
                    _package._size = size;

                    remove_all(temp_dir_path);

                    PACKAGE_INFO_CHANGE_DATA_STATE(CHECKED);
                    PACKAGE_INFO_CHANGE_MANIPULATION_STATE(MS_IDLE);
                    PACKAGE_INFO_GENERATE_EVENT(package_creation_complete, ( ) );
                }
                catch ( const fc::exception& ex ) {
                    remove_all(temp_dir_path);
                    _package.unlock_dir();
                    PACKAGE_INFO_CHANGE_DATA_STATE(INVALID);
                    PACKAGE_INFO_CHANGE_MANIPULATION_STATE(MS_IDLE);
                    PACKAGE_INFO_GENERATE_EVENT(package_creation_error, ( ex.to_detail_string() ) );
                    throw;
                }
                catch ( const std::exception& ex ) {
                    remove_all(temp_dir_path);
                    _package.unlock_dir();
                    PACKAGE_INFO_CHANGE_DATA_STATE(INVALID);
                    PACKAGE_INFO_CHANGE_MANIPULATION_STATE(MS_IDLE);
                    PACKAGE_INFO_GENERATE_EVENT(package_creation_error, ( ex.what() ) );
                    throw;
                }
                catch ( ... ) {
                    remove_all(temp_dir_path);
                    _package.unlock_dir();
                    PACKAGE_INFO_CHANGE_DATA_STATE(INVALID);
                    PACKAGE_INFO_CHANGE_MANIPULATION_STATE(MS_IDLE);
                    PACKAGE_INFO_GENERATE_EVENT(package_creation_error, ( "unknown" ) );
                    throw;
                }
            }

        private:
            const boost::filesystem::path  _content_dir_path;
            const boost::filesystem::path  _samples_dir_path;
            const fc::sha256               _key;
        };


        class RemovePackageTask : public PackageTask {
        public:
            explicit RemovePackageTask(PackageInfo& package)
                : PackageTask(package)
            {
            }

        protected:
            virtual void task() override {
                PACKAGE_TASK_EXIT_IF_REQUESTED;
                PACKAGE_INFO_CHANGE_MANIPULATION_STATE(DELETTING);

                boost::filesystem::remove_all(_package.get_package_dir());

                PACKAGE_INFO_CHANGE_DATA_STATE(DS_UNINITIALIZED);
                PACKAGE_INFO_CHANGE_MANIPULATION_STATE(MS_IDLE);
            }
        };


        class UnpackPackageTask : public PackageTask {
        public:
            explicit UnpackPackageTask(PackageInfo& package,
                                       const boost::filesystem::path& dir_path,
                                       const fc::sha256& key)
                : PackageTask(package)
                , _target_dir(dir_path)
                , _key(key)
            {
            }

        protected:
            virtual void task() override {
                PACKAGE_INFO_GENERATE_EVENT(package_extraction_start, ( ) );

                using namespace boost::filesystem;

                const auto temp_dir_path = unique_path(graphene::utilities::decent_path_finder::instance().get_decent_temp() / "%%%%-%%%%-%%%%-%%%%");

                try {
                    PACKAGE_TASK_EXIT_IF_REQUESTED;

                    if (CryptoPP::AES::MAX_KEYLENGTH > _key.data_size()) {
                        FC_THROW("CryptoPP::AES::MAX_KEYLENGTH is bigger than key size (${size})", ("size", _key.data_size()) );
                    }

                    if (exists(_target_dir) && !is_directory(_target_dir)) {
                        FC_THROW("Target path ${path} must point to directory", ("path", _target_dir.string()));
                    }


//                  PACKAGE_INFO_GENERATE_EVENT(package_extraction_progress, ( ) );


                    create_directories(temp_dir_path);
                    remove_all(temp_dir_path);
                    create_directories(temp_dir_path);

                    if (!exists(_target_dir) || !is_directory(_target_dir)) {
                        try {
                            if (!create_directories(_target_dir) && !is_directory(_target_dir)) {
                                FC_THROW("Unable to create destination directory");
                            }
                        }
                        catch (const boost::filesystem::filesystem_error& ex) {
                            if (!is_directory(_target_dir)) {
                                FC_THROW("Unable to create destination directory: ${error}", ("error", ex.what()) );
                            }
                        }
                    }

                    const auto aes_file_path = _package.get_content_file();
                    const auto archive_file_path = temp_dir_path / "content.zip";

                    {
                        PACKAGE_INFO_CHANGE_MANIPULATION_STATE(DECRYPTING);

                        decent::encrypt::AesKey k;

                        for (int i = 0; i < CryptoPP::AES::MAX_KEYLENGTH; ++i) {
                            k.key_byte[i] = _key.data()[i];
                        }

                        if (space(temp_dir_path).available < file_size(aes_file_path) * 1.5) { // Safety margin
                            FC_THROW("Not enough storage space to create package in ${tmp_dir}", ("tmp_dir", temp_dir_path.string()) );
                        }

                        AES_decrypt_file(aes_file_path.string(), archive_file_path.string(), k);

                        PACKAGE_TASK_EXIT_IF_REQUESTED;
                        PACKAGE_INFO_CHANGE_MANIPULATION_STATE(UNPACKING);

                        using namespace boost::iostreams;

                        boost::iostreams::filtering_istream istr;
                        istr.push(gzip_decompressor());
                        istr.push(file_source(archive_file_path.string(), std::ios::in | std::ios::binary));

                        detail::Dearchiver dearchiver(istr);
                        dearchiver.extract(_target_dir);
                    }

                    remove_all(temp_dir_path);

                    PACKAGE_INFO_CHANGE_DATA_STATE(CHECKED);
                    PACKAGE_INFO_CHANGE_MANIPULATION_STATE(MS_IDLE);
                    PACKAGE_INFO_GENERATE_EVENT(package_extraction_complete, ( ) );
                }
                catch ( const fc::exception& ex ) {
                    remove_all(temp_dir_path);
//                  PACKAGE_INFO_CHANGE_DATA_STATE(INVALID);
                    PACKAGE_INFO_CHANGE_MANIPULATION_STATE(MS_IDLE);
                    PACKAGE_INFO_GENERATE_EVENT(package_extraction_error, ( ex.to_detail_string() ) );
                    throw;
                }
                catch ( const std::exception& ex ) {
                    remove_all(temp_dir_path);
//                  PACKAGE_INFO_CHANGE_DATA_STATE(INVALID);
                    PACKAGE_INFO_CHANGE_MANIPULATION_STATE(MS_IDLE);
                    PACKAGE_INFO_GENERATE_EVENT(package_extraction_error, ( ex.what() ) );
                    throw;
                }
                catch ( ... ) {
                    remove_all(temp_dir_path);
//                  PACKAGE_INFO_CHANGE_DATA_STATE(INVALID);
                    PACKAGE_INFO_CHANGE_MANIPULATION_STATE(MS_IDLE);
                    PACKAGE_INFO_GENERATE_EVENT(package_extraction_error, ( "unknown" ) );
                    throw;
                }
            }

        private:
            const boost::filesystem::path _target_dir;
            const fc::sha256 _key;
        };


        class CheckPackageTask : public PackageTask {
        public:
            explicit CheckPackageTask(PackageInfo& package)
                : PackageTask(package)
            {
            }

        protected:
            virtual void task() override {
                PACKAGE_INFO_GENERATE_EVENT(package_check_start, ( ) );

                try {
                    PACKAGE_TASK_EXIT_IF_REQUESTED;
                    PACKAGE_INFO_CHANGE_MANIPULATION_STATE(CHECKING);


//                  PACKAGE_INFO_GENERATE_EVENT(package_check_progress, ( ) );


                    const auto aes_file_path = _package.get_content_file();
                    const auto file_hash = detail::calculate_hash(aes_file_path);

                    if (_package._hash != file_hash) {
                        FC_THROW("Package hash (${phash}) does not match ${fn} content file hash (${fhash})",
                                  ("phash", _package._hash.str()) ("fn", aes_file_path.string()) ("fhash", file_hash.str()) );
                    }
                    //TODO_DECENT - we should check the size here...

                    PACKAGE_INFO_CHANGE_DATA_STATE(CHECKED);
                    PACKAGE_INFO_CHANGE_MANIPULATION_STATE(MS_IDLE);
                    PACKAGE_INFO_GENERATE_EVENT(package_check_complete, ( ) );
                }
                catch ( const fc::exception& ex ) {
                    PACKAGE_INFO_CHANGE_DATA_STATE(INVALID);
                    PACKAGE_INFO_CHANGE_MANIPULATION_STATE(MS_IDLE);
                    PACKAGE_INFO_GENERATE_EVENT(package_check_error, ( ex.to_detail_string() ) );
                    throw;
                }
                catch ( const std::exception& ex ) {
                    PACKAGE_INFO_CHANGE_DATA_STATE(INVALID);
                    PACKAGE_INFO_CHANGE_MANIPULATION_STATE(MS_IDLE);
                    PACKAGE_INFO_GENERATE_EVENT(package_check_error, ( ex.what() ) );
                    throw;
                }
                catch ( ... ) {
                    PACKAGE_INFO_CHANGE_DATA_STATE(INVALID);
                    PACKAGE_INFO_CHANGE_MANIPULATION_STATE(MS_IDLE);
                    PACKAGE_INFO_GENERATE_EVENT(package_check_error, ( "unknown" ) );
                    throw;
                }
            }
        };


    } // namespace detail


    PackageInfo::PackageInfo(PackageManager& manager,
                             const boost::filesystem::path& content_dir_path,
                             const boost::filesystem::path& samples_dir_path,
                             const fc::sha256& key)
        : _data_state(DS_UNINITIALIZED)
        , _transfer_state(TS_IDLE)
        , _manipulation_state(MS_IDLE)
        , _parent_dir(manager.get_packages_path())
        , _create_task(std::make_shared<detail::CreatePackageTask>(*this, manager, content_dir_path, samples_dir_path, key))
    {
    }

    PackageInfo::PackageInfo(PackageManager& manager, const fc::ripemd160& package_hash)
        : _data_state(DS_UNINITIALIZED)
        , _transfer_state(TS_IDLE)
        , _manipulation_state(MS_IDLE)
        , _hash(package_hash)
        , _parent_dir(manager.get_packages_path())
    {
        auto& _package = *this; // For macros to work.

        PACKAGE_INFO_CHANGE_DATA_STATE(PARTIAL);
        PACKAGE_INFO_GENERATE_EVENT(package_restoration_start, ( ) );

        try {
            using namespace boost::filesystem;

            if (!exists(get_package_dir()) || !is_directory(get_package_dir())) {
                FC_THROW("Package directory ${path} does not exist", ("path", get_package_dir().string()) );
            }

//          PACKAGE_INFO_GENERATE_EVENT(package_restoration_progress, ( ) );

            lock_dir();

            PACKAGE_INFO_CHANGE_DATA_STATE(UNCHECKED);
            PACKAGE_INFO_CHANGE_MANIPULATION_STATE(MS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_restoration_complete, ( ) );
        }
        catch ( const fc::exception& ex ) {
            unlock_dir();
            PACKAGE_INFO_CHANGE_DATA_STATE(INVALID);
            PACKAGE_INFO_CHANGE_MANIPULATION_STATE(MS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_restoration_error, ( ex.to_detail_string() ) );
            throw;
        }
        catch ( const std::exception& ex ) {
            unlock_dir();
            PACKAGE_INFO_CHANGE_DATA_STATE(INVALID);
            PACKAGE_INFO_CHANGE_MANIPULATION_STATE(MS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_restoration_error, ( ex.what() ) );
            throw;
        }
        catch ( ... ) {
            unlock_dir();
            PACKAGE_INFO_CHANGE_DATA_STATE(INVALID);
            PACKAGE_INFO_CHANGE_MANIPULATION_STATE(MS_IDLE);
            PACKAGE_INFO_GENERATE_EVENT(package_restoration_error, ( "unknown" ) );
            throw;
        }
    }

    PackageInfo::PackageInfo(PackageManager& manager, const std::string& url)
        : _data_state(DS_UNINITIALIZED)
        , _transfer_state(TS_IDLE)
        , _manipulation_state(MS_IDLE)
        , _url(url)
        , _parent_dir(manager.get_packages_path())
        , _download_task(manager.get_proto_transfer_engine(detail::get_proto(url)).create_download_task(*this))
    {
    }

    PackageInfo::~PackageInfo() {
        cancel_current_task(true);
        unlock_dir();
    }

    void PackageInfo::create(bool block) {
        std::lock_guard<std::recursive_mutex> guard(_task_mutex);

        if (!_create_task) {
            FC_THROW("package handle was not prepared for creation");
        }

        _create_task->stop(true);

        _current_task = _create_task;
        _current_task->start(block);
    }

    void PackageInfo::unpack(const boost::filesystem::path& dir_path, const fc::sha256& key, bool block) {
        std::lock_guard<std::recursive_mutex> guard(_task_mutex);

        _current_task.reset(new detail::UnpackPackageTask(*this, dir_path, key));
        _current_task->start(block);
    }

    void PackageInfo::download(bool block) {
        std::lock_guard<std::recursive_mutex> guard(_task_mutex);

        if (!_download_task) {
            if( _data_state == CHECKED ) {
                _download_task = decent::package::PackageManager::instance().get_proto_transfer_engine(
                      "local").create_download_task(*this);
            }else {
                FC_THROW("package handle was not prepared for download");
            }
        }

        _download_task->stop(true);

        _current_task = _download_task;
        _current_task->start(block);
    }

    void PackageInfo::start_seeding(std::string proto, bool block) {
        std::lock_guard<std::recursive_mutex> guard(_task_mutex);

        const std::string url_proto = detail::get_proto(_url);

        if (proto.empty()) {
            proto = url_proto;
        }

        if (proto.empty()) {
            FC_THROW("seeding protocol must be specified");
        }

        if (!url_proto.empty() && proto != url_proto) {
            FC_THROW("seeding protocol of the package must be '${proto}'", ("proto", proto) );
        }
        
        _current_task = PackageManager::instance().get_proto_transfer_engine(proto).create_start_seeding_task(*this);
        _current_task->start(block);
    }

    void PackageInfo::stop_seeding(std::string proto, bool block) {
        std::lock_guard<std::recursive_mutex> guard(_task_mutex);

        const std::string url_proto = detail::get_proto(_url);

        if (proto.empty()) {
            proto = url_proto;
        }

        if (proto.empty()) {
            FC_THROW("seeding protocol must be specified");
        }

        if (!url_proto.empty() && proto != url_proto) {
            FC_THROW("seeding protocol of the package must be '${proto}'", ("proto", proto) );
        }

        _current_task = PackageManager::instance().get_proto_transfer_engine(proto).create_stop_seeding_task(*this);
        _current_task->start(block);
    }

    void PackageInfo::check(bool block) {
        std::lock_guard<std::recursive_mutex> guard(_task_mutex);

        _current_task.reset(new detail::CheckPackageTask(*this));
        _current_task->start(block);
    }

    void PackageInfo::remove(bool block) {
        std::lock_guard<std::recursive_mutex> guard(_task_mutex);

        _current_task.reset(new detail::RemovePackageTask(*this));
        _current_task->start(block);
    }

    void PackageInfo::create_proof_of_custody(const decent::encrypt::CustodyData& cd, decent::encrypt::CustodyProof& proof)const {
        //assume the data are downloaded and available
       FC_ASSERT(cd.n < 10000000 );
       int ret = decent::encrypt::CustodyUtils::instance().create_proof_of_custody(get_content_file(), cd, proof);
       if( ret != 0 )
          FC_THROW("Failed to create custody data");
       return;
    }

    void PackageInfo::wait_for_current_task() {
        decltype(_current_task) current_task;
        {
            std::lock_guard<std::recursive_mutex> guard(_task_mutex);
            current_task = _current_task;
        }

        if (current_task) {
            current_task->wait();
        }
    }

    void PackageInfo::cancel_current_task(bool block) {
        std::lock_guard<std::recursive_mutex> guard(_task_mutex);

        if (_current_task) {
            _current_task->stop(true);
        }
    }

    std::exception_ptr PackageInfo::get_task_last_error() const {
        std::lock_guard<std::recursive_mutex> guard(_task_mutex);

        if (_current_task) {
            return _current_task->consume_last_error();
        }

        return nullptr;
    }

    void PackageInfo::add_event_listener(const event_listener_handle_t& event_listener) {
        std::lock_guard<std::recursive_mutex> guard(_event_mutex);

        if (event_listener) {
            if (std::find(_event_listeners.begin(), _event_listeners.end(), event_listener) == _event_listeners.end()) {
                _event_listeners.push_back(event_listener);
            }
        }
    }

    void PackageInfo::remove_event_listener(const event_listener_handle_t& event_listener) {
        std::lock_guard<std::recursive_mutex> guard(_event_mutex);
        _event_listeners.remove(event_listener);
    }

    void PackageInfo::remove_all_event_listeners() {
        std::lock_guard<std::recursive_mutex> guard(_event_mutex);
        _event_listeners.clear();
    }


    PackageInfo::DataState PackageInfo::get_data_state() const {
        std::lock_guard<std::recursive_mutex> guard(_mutex);
        return _data_state;
    }

    PackageInfo::TransferState PackageInfo::get_transfer_state() const {
        std::lock_guard<std::recursive_mutex> guard(_mutex);
        return _transfer_state;
    }

    PackageInfo::ManipulationState PackageInfo::get_manipulation_state() const {
        std::lock_guard<std::recursive_mutex> guard(_mutex);
        return _manipulation_state;
    }
   
   uint64_t PackageInfo::get_size() const {
      size_t size=0;
      for(recursive_directory_iterator it( get_package_dir() );
          it != recursive_directory_iterator();
          ++it)
      {
         if(!is_directory(*it))
            size+=file_size(*it);
      }
      return size;
   }
   
   

    void PackageInfo::lock_dir() {
        std::lock_guard<std::recursive_mutex> guard(_mutex);

        using namespace boost::interprocess;

        detail::touch(get_lock_file_path());

        _file_lock_guard.reset();
        _file_lock.reset(new file_lock(get_lock_file_path().string().c_str()));

        if (!_file_lock->try_lock()) {
            _file_lock.reset();
            FC_THROW("Unable to lock package directory ${path} (lock file ${file})", ("path", get_package_dir().string()) ("file", get_lock_file_path().string()) );
        }

        _file_lock_guard.reset(new scoped_lock<file_lock>(*_file_lock, accept_ownership_type()));
    }

    void PackageInfo::unlock_dir() {
        std::lock_guard<std::recursive_mutex> guard(_mutex);
        _file_lock_guard.reset();
        _file_lock.reset();
    }

/*
    bool PackageInfo::is_dir_locked() const {
        std::lock_guard<std::recursive_mutex> guard(_mutex);
        return _file_lock;
    }
*/

    PackageManager::PackageManager(const boost::filesystem::path& packages_path)
        : _packages_path(packages_path)
    {
        if (!exists(_packages_path) || !is_directory(_packages_path)) {
            try {
                if (!create_directories(_packages_path) && !is_directory(_packages_path)) {
                    FC_THROW("Unable to create packages directory ${path}", ("path", _packages_path.string()) );
                }
            }
            catch (const boost::filesystem::filesystem_error& ex) {
                if (!is_directory(_packages_path)) {
                    FC_THROW("Unable to create packages directory ${path}: ${error}", ("path", _packages_path.string()) ("error", ex.what()) );
                }
            }
        }

//      _proto_transfer_engines["magnet"] = std::make_shared<TorrentTransferEngine>();
        _proto_transfer_engines["ipfs"] = std::make_shared<IPFSTransferEngine>();
        _proto_transfer_engines["local"] = std::make_shared<LocalTransferEngine>();

        set_libtorrent_config(graphene::utilities::decent_path_finder::instance().get_decent_home() / "libtorrent.json");

        // TODO: restore anything?
    }

    PackageManager::~PackageManager() {
        if (release_all_packages()) {
            elog("some of the packages are used elsewhere, while the package manager instance is shutting down");
        }

        // TODO: save anything?
    }

    package_handle_t PackageManager::get_package(const boost::filesystem::path& content_dir_path,
                                                 const boost::filesystem::path& samples_dir_path,
                                                 const fc::sha256& key)
    {
        std::lock_guard<std::recursive_mutex> guard(_mutex);
        package_handle_t package(new PackageInfo(*this, content_dir_path, samples_dir_path, key));
        return *_packages.insert(package).first;
    }

    package_handle_t PackageManager::get_package(const std::string& url, const fc::ripemd160&  hash)
    {
        std::lock_guard<std::recursive_mutex> guard(_mutex);
        for (auto& package : _packages) {
            if (package) {
                if (package->_hash == hash) {
                    package->_url = url;
                    return package;
                }
            }
        }
        package_handle_t package(new PackageInfo(*this, url));
        return *_packages.insert(package).first;
    }

    package_handle_t PackageManager::get_package(const fc::ripemd160& hash)
    {
        std::lock_guard<std::recursive_mutex> guard(_mutex);

        for (auto& package : _packages) {
            if (package) {
                if (package->_hash == hash) {
                    return package;
                }
            }
        }

        package_handle_t package(new PackageInfo(*this, hash));
        return *_packages.insert(package).first;
    }


    package_handle_t PackageManager::find_package(const std::string& url)
    {
        std::lock_guard<std::recursive_mutex> guard(_mutex);
        
        for (auto& pack: _packages) {
            if (pack->get_url() == url) {
                return pack;
            }
        }
        return nullptr;
    }

    package_handle_t PackageManager::find_package(const fc::ripemd160& hash)
    {
        std::lock_guard<std::recursive_mutex> guard(_mutex);

        for (auto& package : _packages) {
            if (package) {
                if (package->_hash == hash) {
                    return package;
                }
            }
        }

        return nullptr;
    }


    package_handle_set_t PackageManager::get_all_known_packages() const {
        std::lock_guard<std::recursive_mutex> guard(_mutex);
        return _packages;
    }

    void PackageManager::recover_all_packages(const event_listener_handle_t& event_listener) {
        std::lock_guard<std::recursive_mutex> guard(_mutex);

        ilog("reading packages from directory ${path}", ("path", _packages_path.string()) );

        using namespace boost::filesystem;

        for (directory_iterator entry(_packages_path); entry != directory_iterator(); ++entry) {
            try {
                const std::string hash_str = entry->path().filename().string();

                if (!detail::is_correct_hash_str(hash_str)) {
                    FC_THROW("Package directory ${path} does not look like RIPEMD-160 hash", ("path", hash_str) );
                }

                get_package(fc::ripemd160(hash_str))->add_event_listener(event_listener);
            }
            catch (const fc::exception& ex)
            {
                elog("unable to read package at ${path}: ${error}", ("path", entry->path().string()) ("error", ex.to_detail_string()) );
            }
        }

        ilog("read ${size} packages", ("size", _packages.size()) );
    }

    bool PackageManager::release_all_packages() {
        std::lock_guard<std::recursive_mutex> guard(_mutex);

        bool other_uses = false;

        if (!_packages_path.empty()) {
            ilog("releasing ${size} packages", ("size", _packages.size()) );

            for (auto it = _packages.begin(); it != _packages.end(); ) {
                other_uses = other_uses || it->use_count() > 1;
                it = _packages.erase(it);
            }
        }

        return other_uses;
    }

    bool PackageManager::release_package(const fc::ripemd160& hash) {
        std::lock_guard<std::recursive_mutex> guard(_mutex);

        bool other_uses = false;

        for (auto it = _packages.begin(); it != _packages.end(); ) {
            if (!(*it) || (*it)->_hash == hash) {
                other_uses = other_uses || it->use_count() > 1;
                it = _packages.erase(it);
            }
            else {
                ++it;
            }
        }

        return other_uses;
    }

    bool PackageManager::release_package(package_handle_t& package) {
        std::lock_guard<std::recursive_mutex> guard(_mutex);
        _packages.erase(package);

        const bool other_uses = (package.use_count() > 1);
        package.reset();

        return other_uses;
    }

    boost::filesystem::path PackageManager::get_packages_path() const {
        std::lock_guard<std::recursive_mutex> guard(_mutex);
        return _packages_path;
    }

    void PackageManager::set_libtorrent_config(const boost::filesystem::path& libtorrent_config_file) {
        std::lock_guard<std::recursive_mutex> guard(_mutex);

        for(auto& proto_transfer_engine : _proto_transfer_engines) {
            TorrentTransferEngine* torrent_engine = dynamic_cast<TorrentTransferEngine*>(proto_transfer_engine.second.get());
            if (torrent_engine) {
                if (libtorrent_config_file.empty() || boost::filesystem::exists(libtorrent_config_file)) {
                    torrent_engine->reconfigure(libtorrent_config_file);
                }
                else {
                    torrent_engine->dump_config(libtorrent_config_file);
                    break; // dump the config of first one found only
                }
            }
        }
    }

    TransferEngineInterface& PackageManager::get_proto_transfer_engine(const std::string& proto) const {
        std::lock_guard<std::recursive_mutex> guard(_mutex);

        auto it = _proto_transfer_engines.find(proto);
        if (it == _proto_transfer_engines.end()) {
            FC_THROW("Cannot find protocol handler for '${proto}'", ("proto", proto) );
        }

        return *it->second;
    }


} } // namespace decent::package



