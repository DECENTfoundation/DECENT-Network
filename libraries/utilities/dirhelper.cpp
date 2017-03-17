
#include <graphene/utilities/dirhelper.hpp>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <cstdlib>
#include <boost/filesystem.hpp>

using namespace graphene;
using namespace utilities;


decent_path_finder::decent_path_finder() {
    struct passwd *pw = getpwuid(getuid());
    const char *home_dir = pw->pw_dir;
    _user_home = home_dir;

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
        _decent_logs = _decent_logs;
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

    create_directories(_decent_home);
    create_directories(_decent_data);
    create_directories(_decent_logs);
    create_directories(_decent_temp);
}
