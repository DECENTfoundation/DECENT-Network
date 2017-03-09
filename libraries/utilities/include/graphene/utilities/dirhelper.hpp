
#pragma once

#include <fc/filesystem.hpp>

#include <cstdlib>


namespace graphene { namespace utilities {


    class decent_path_finder {
    private:
        // Constructor may throw exceptions. Which is bad in general, but if it fails program execution must be terminated
        decent_path_finder();
        decent_path_finder(const decent_path_finder&) {}

    public:
        static decent_path_finder& instance() {
            static decent_path_finder theChoosenOne;
            return theChoosenOne;
        }

    public:
        fc::path get_user_home()   const { return _user_home; }
        fc::path get_decent_home() const { return _decent_home; }
        fc::path get_decent_data() const { return _decent_data; }
        fc::path get_decent_logs() const { return _decent_logs; }
        fc::path get_decent_temp() const { return _decent_temp; }

    private:
        fc::path _user_home;
        fc::path _decent_home;
        fc::path _decent_data;
        fc::path _decent_logs;
        fc::path _decent_temp;
    };


} } // graphene::utilities
