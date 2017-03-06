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
#pragma once

#include <cstdlib>

#include <fc/filesystem.hpp>

namespace graphene { namespace utilities {


    
    
    class decent_path_finder {
        
        // Constructor may throw exceptions. Which is bad in general, but if it fails program execution must be terminated
        
        decent_path_finder();
        decent_path_finder(const decent_path_finder&) {}
    public:
        
        static decent_path_finder& instance() {
            static decent_path_finder theChoosenOne;
            return theChoosenOne;
        }
        
    public:
        fc::path get_decent_temp() const { return _decent_temp; }
        fc::path get_user_home() const { return _user_home; }
        fc::path get_decent_home() const { return _decent_home; }
        fc::path get_decent_logs() const { return _decent_temp; }
        fc::path get_decent_data() const { return _decent_data; }
        
        
    private:
        fc::path  _user_home;
        fc::path  _decent_data;
        fc::path  _decent_home;
        fc::path  _decent_logs;
        fc::path  _decent_temp;
        
    };

} } // graphene::utilities
