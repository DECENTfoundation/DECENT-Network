/* (c) 2019 DECENT Services. For details refers to LICENSE.txt */
#include <decent/json.hpp>

namespace decent {

   std::string json_to_string(const json_t &doc)
   {
      return doc.dump();
   }

   json_t string_to_json(const std::string &doc)
   {
      return json_t::parse(doc);
   }

   std::vector<uint8_t> json_to_binary(const json_t &doc)
   {
      return json_t::to_ubjson(doc);
   }

   json_t binary_to_json(const std::vector<uint8_t> &doc)
   {
      return json_t::from_ubjson(doc);
   }

}

namespace fc {

   void to_variant(const decent::json_t& o, variant& v)
   {
      to_variant(decent::json_to_string(o), v);
   }

   void from_variant(const variant& v, decent::json_t& o)
   {
      std::string s;
      from_variant(v, s);

      try {
         o = decent::string_to_json(s);
      }
      catch(const decent::json_t::parse_error &e) {
         FC_THROW_EXCEPTION(parse_error_exception, e.what());
      }
   }

}
