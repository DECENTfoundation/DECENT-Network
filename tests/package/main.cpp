/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
/*
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

#include <cstdlib>
#include <iostream>
#include <boost/test/included/unit_test.hpp>
#include <fc/filesystem.hpp>


#include <decent/package/package.hpp>

using namespace decent::package;

uint32_t GRAPHENE_TESTING_GENESIS_TIMESTAMP;


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

///////////////////////////////////////////////////////////////////////////////////////

void create_fake_content(boost::filesystem::path& content_path, boost::filesystem::path& samples_path)
{
   fc::path temp_dir = fc::temp_directory_path() / "decent_fake_content";

   boost::filesystem::create_directories(boost::filesystem::path(temp_dir.string()) );

   content_path = temp_dir / "content";
   samples_path = temp_dir / "samples";

   boost::filesystem::create_directories(content_path);
   boost::filesystem::create_directories(samples_path);

   boost::filesystem::ofstream content_file(content_path / "fake_content.txt" );
   content_file << "Heloo world of DECENT.";
   content_file.close();

   boost::filesystem::ofstream sample_file(samples_path / "fake_sample.txt" );
   sample_file << "Sample file...";
   sample_file.close();
}

bool check_fake_content(const boost::filesystem::path& base_dir)
{
   boost::filesystem::path filename = base_dir / "fake_content.txt";
   return boost::filesystem::exists(filename);
}

void create_unpack_folder(boost::filesystem::path& content_path)
{
   fc::path temp_dir = fc::temp_directory_path() / "decent_fake_content";

   boost::filesystem::create_directories(boost::filesystem::path(temp_dir.string()) );

   content_path = temp_dir / "unpack";
   boost::filesystem::create_directories(content_path);
}

///////////////////////////////////////////////////////////////////////////////////////
// run IPFS daemon before this test...

std::string g_test_packagename;
std::string g_test_string_as_key = "some_string_to_use_as_a_key";

boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {

   std::srand(time(NULL));
   std::cout << "Random number generator seeded to " << time(NULL) << std::endl;
   const char* genesis_timestamp_str = getenv("GRAPHENE_TESTING_GENESIS_TIMESTAMP");
   if( genesis_timestamp_str != nullptr )
   {
      GRAPHENE_TESTING_GENESIS_TIMESTAMP = std::stoul( genesis_timestamp_str );
   }
   std::cout << "GRAPHENE_TESTING_GENESIS_TIMESTAMP is " << GRAPHENE_TESTING_GENESIS_TIMESTAMP << std::endl;
   return nullptr;
}

BOOST_AUTO_TEST_CASE( package_create_test )
{

   auto& package_manager = decent::package::PackageManager::instance();

   package_manager.recover_all_packages();

   const fc::sha256 key = fc::sha256::hash(g_test_string_as_key);
   const bool block = false;

   boost::filesystem::path content_dir;   // =  "/tmp/test/test1_content";
   boost::filesystem::path samples_dir;   // = "/tmp/test/test1_samples";

   //create some fake content...
   create_fake_content(content_dir, samples_dir);


   try {
      auto package_handle = package_manager.get_package(content_dir, samples_dir, key, DECENT_SECTORS);
      BOOST_CHECK(package_handle.get() != nullptr);

      package_handle->add_event_listener(std::make_shared<MyEventListener>());

      {
         package_handle->create(block);
         package_handle->wait_for_current_task();
         BOOST_CHECK(package_handle->get_task_last_error() == nullptr);
      }

      std::cerr << "starting seeding" << std::endl;
      package_handle->start_seeding("ipfs");
      package_handle->wait_for_current_task();
      BOOST_CHECK(package_handle->get_task_last_error() == nullptr);

//         std::cerr << "started seeding" << std::endl;
//         std::cerr << "Package url:" << package_handle->get_url() << std::endl;

      // It is assumed that the package is now available for download from elsewhere.
      std::this_thread::sleep_for(std::chrono::seconds(10));

      {
         package_handle->stop_seeding("ipfs");
         package_handle->wait_for_current_task();
         BOOST_CHECK(package_handle->get_task_last_error() == nullptr);

      }

      const boost::filesystem::path package_dir = package_handle->get_package_dir();

      g_test_packagename = package_handle->get_url();

      package_manager.release_package(package_handle);

      boost::filesystem::remove_all(content_dir);
      boost::filesystem::remove_all(samples_dir);


   } FC_LOG_AND_RETHROW()

   package_manager.release_all_packages();

}

BOOST_AUTO_TEST_CASE( package_download_and_unpack_test )
{
   const bool block = false;

   if (g_test_packagename.empty()) {
      return;
   }

   boost::filesystem::path dest_dir;
   create_unpack_folder(dest_dir);

   const fc::sha256 key = fc::sha256::hash(g_test_string_as_key);

   auto& package_manager = decent::package::PackageManager::instance();

   try {
      //    "/ipfs/QmWgZbg73wrgicmPradJcK51nY99o2fX8dt7pBJ8rUaurJ"
      auto package_handle = package_manager.get_package(g_test_packagename, fc::ripemd160());
      BOOST_CHECK(package_handle.get() != nullptr);
      package_handle->add_event_listener(std::make_shared<MyEventListener>());

      {
         package_handle->download();
         package_handle->wait_for_current_task();
         BOOST_CHECK(package_handle->get_task_last_error() == nullptr);
      }

      {
         package_handle->check(block);
         package_handle->wait_for_current_task();
         BOOST_CHECK(package_handle->get_task_last_error() == nullptr);
      }

      {
         package_handle->unpack(dest_dir, key, block);
         package_handle->wait_for_current_task();
         BOOST_CHECK(package_handle->get_task_last_error() == nullptr);
      }

      //check if file exists...
      BOOST_CHECK(check_fake_content(dest_dir));

      boost::filesystem::remove_all(dest_dir);

   } FC_LOG_AND_RETHROW()

   package_manager.release_all_packages();
}
