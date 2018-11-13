/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
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
#include <decent/monitoring/monitoring_fc.hpp>
#include <graphene/utilities/dirhelper.hpp>

#include <fc/reflect/variant.hpp>
#include <fc/io/json.hpp>

#include <algorithm>
#include <fstream>

namespace monitoring {
   static fc::path monitoring_path;

   void set_data_dir(fc::path decentd_dir)
   {
      monitoring_path = decentd_dir / "monitoring";
      fc::create_directories(monitoring_path);
      monitoring_path = monitoring_path / "counters.json";
   }
}

void monitoring::monitoring_counters_base::save_to_disk(const std::vector<counter_item>& counters)
{
   graphene::utilities::decent_path_finder &dpf = graphene::utilities::decent_path_finder::instance();
   fc::path data_dir = dpf.get_decent_data();
   std::fstream fs;
   fs.open(monitoring_path.string().c_str(), std::fstream::out);
   if (fs.is_open()) {

      fc::variant tmp;
      fc::to_variant(counters, tmp);
      std::string s = fc::json::to_string(tmp);

      fs.write(s.c_str(), s.size());
   }
}

void monitoring::monitoring_counters_base::read_from_disk(std::vector<counter_item>& counters)
{
   graphene::utilities::decent_path_finder &dpf = graphene::utilities::decent_path_finder::instance();
   fc::path data_dir = dpf.get_decent_data();
   std::fstream fs;
   fs.open(monitoring_path.string().c_str(), std::fstream::in);
   if (fs.is_open()) {

      std::string s;
      fs.seekg(0, std::ios::end);
      s.reserve(fs.tellg());
      fs.seekg(0, std::ios::beg);

      s.assign((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());

      fc::variant tmp = fc::json::from_string(s);
      fc::from_variant(tmp, counters);
   }
}