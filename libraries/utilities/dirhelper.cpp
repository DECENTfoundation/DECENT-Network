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
        create_directories(_decent_home);
    } else {
        _decent_home = decent_home;
    }
    
    const char* decent_logs = getenv("DECENT_LOGS");
    if (decent_logs == NULL) {
        _decent_logs = _decent_home / "logs";
        create_directories(_decent_logs);
    } else {
        _decent_logs = _decent_logs;
    }
    
    const char* decent_temp = getenv("DECENT_TEMP");
    if (decent_temp == NULL) {
        _decent_temp = _decent_home / "temp";
        create_directories(_decent_temp);
    } else {
        _decent_temp = decent_temp;
    }
    
    const char* decent_data = getenv("DECENT_DATA");
    if (decent_data == NULL) {
        _decent_data = _decent_home / "data";
        create_directories(_decent_data);
    } else {
        _decent_data = decent_data;
    }
    
}



