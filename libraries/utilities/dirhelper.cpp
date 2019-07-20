/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */

#include <graphene/utilities/dirhelper.hpp>
#include <boost/filesystem.hpp>

#if defined( _MSC_VER )
#include <ShlObj.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif
#include <cstdlib>

namespace graphene { namespace utilities {

decent_path_finder::decent_path_finder()
{
#if defined( _MSC_VER )
   PWSTR path = NULL;
   HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &path);
   if (SUCCEEDED(hr)) {
      _user_home = std::wstring(path);
      CoTaskMemFree(path);
   }
#else
   passwd *pw = getpwuid(getuid());
   if (pw && pw->pw_dir) {
      _user_home = pw->pw_dir;
   }
#endif

   const char* decent_home = getenv("DECENT_HOME");
   if (decent_home == NULL) {
      _decent_home = _user_home / ".decent";
   } else {
      _decent_home = decent_home;
   }

   const char* decent_logs = getenv("DECENT_LOGS");
   if (decent_logs == NULL) {
      _decent_logs = _decent_home / "logs";
   } else {
      _decent_logs = decent_logs;
   }

   const char* decent_temp = getenv("DECENT_TEMP");
   if (decent_temp == NULL) {
      _decent_temp = _decent_home / "temp";
   } else {
      _decent_temp = decent_temp;
   }

   const char* decent_data = getenv("DECENT_DATA");
   if (decent_data == NULL) {
      _decent_data = _decent_home / "data";
   } else {
      _decent_data = decent_data;
   }

   const char* ipfs_bin_dir = getenv("IPFS_BIN");
   if (ipfs_bin_dir != NULL) {
      _ipfs_bin = ipfs_bin_dir;
   }

   const char* ipfs_path_dir = getenv("IPFS_PATH");
   if (ipfs_path_dir != NULL) {
      _ipfs_path = ipfs_path_dir;
   }

   create_directories(_decent_home);
   create_directories(_decent_data);
   create_directories(_decent_logs);
   create_directories(_decent_temp);
}

} } // graphene::utilities
