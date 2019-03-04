/* (c) 2019 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#include <nlohmann/json.hpp>

#include <fc/io/raw.hpp>

namespace decent {

   typedef nlohmann::json json_t;

   std::string json_to_string(const json_t &doc);
   json_t string_to_json(const std::string &doc);

   std::vector<uint8_t> json_to_binary(const json_t &doc);
   json_t binary_to_json(const std::vector<uint8_t> &doc);

}

FC_REFLECT_TYPENAME( decent::json_t )

namespace fc {

   void to_variant(const decent::json_t& o, variant& v);
   void from_variant(const variant& v, decent::json_t& o);

   template<> struct reflector<decent::json_t> {
      typedef decent::json_t type;
      typedef fc::true_type is_defined;
      typedef fc::false_type is_enum;
      enum member_count_enum {
         local_member_count = 1,
         total_member_count = 1
      };

      template<typename S, typename C>
      static void visit(const fc::raw::detail::pack_object_visitor<S, C>& v) {
         std::function<std::vector<uint8_t>(const decent::json_t&)> f = &decent::json_to_binary;
         v.TEMPLATE operator()("doc", f);
      }

      template<typename S, typename C>
      static void visit(const fc::raw::detail::unpack_object_visitor<S, C>& v) {
         try {
            std::function<decent::json_t(const std::vector<uint8_t>&)> f = &decent::binary_to_json;
            v.TEMPLATE operator()("doc", f);
         }
         catch(const decent::json_t::parse_error& e) {
            FC_THROW_EXCEPTION(parse_error_exception, e.what());
         }
      }
   };
}
