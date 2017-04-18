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
#include <graphene/utilities/string_escape.hpp>
#include <sstream>
#include <numeric>
#if defined( _MSC_VER )
#include <iso646.h>
#endif
namespace graphene { namespace utilities {

  std::string escape_string_for_c_source_code(const std::string& input)
  {
    std::ostringstream escaped_string;
    escaped_string << "\"";
    for (unsigned i = 0; i < input.size(); ++i)
    {
      switch (input[i])
      {
      case '\a': 
        escaped_string << "\\a";
        break;
      case '\b': 
        escaped_string << "\\b";
        break;
      case '\t': 
        escaped_string << "\\t";
        break;
      case '\n': 
        escaped_string << "\\n";
        break;
      case '\v': 
        escaped_string << "\\v";
        break;
      case '\f': 
        escaped_string << "\\f";
        break;
      case '\r': 
        escaped_string << "\\r";
        break;
      case '\\': 
        escaped_string << "\\\\";
        break;
      case '\"': 
        escaped_string << "\\\"";
        break;
      default:
        escaped_string << input[i];
      }
    }
    escaped_string << "\"";
    return escaped_string.str();
  }
    std::size_t extra_space(const std::string& s) noexcept
    {
        return std::accumulate(s.begin(), s.end(), size_t{},
                               [](size_t res, typename std::string::value_type c)
                               {
                                   switch (c)
                                   {
                                       case '"':
                                       case '\\':
                                       case '\b':
                                       case '\f':
                                       case '\n':
                                       case '\r':
                                       case '\t':
                                       {
                                           // from c (1 byte) to \x (2 bytes)
                                           return res + 1;
                                       }
                                           
                                       default:
                                       {
                                           if (c >= 0x00 and c <= 0x1f)
                                           {
                                               // from c (1 byte) to \uxxxx (6 bytes)
                                               return res + 5;
                                           }
                                           
                                           return res;
                                       }
                                   }
                               });
    }
    
    std::string json_unescape_string(const std::string& s)
    {
        std::string result;
        bool is_escape = false;
        
        for (const auto& c : s)
        {
            if (!is_escape && (c != '\\')) {
                result += c;
                continue;
            }
            
            if (!is_escape && (c == '\\')) {
                is_escape = true;
                continue;
            }
            
            is_escape = false;
            
            switch (c)
            {
                case '"':
                {
                    result += '"';
                    break;
                }
                    
                case '\\':
                {
                    result += '\\';
                    break;
                }
                    
                // backspace (0x08)
                case 'b':
                {
                    result += '\b';
                    break;
                }
                    
                // formfeed (0x0c)
                case 'f':
                {
                    result += '\f';
                    break;
                }
                    
                // newline (0x0a)
                case 'n':
                {
                    result += '\n';
                    break;
                }
                    
                // carriage return (0x0d)
                case 'r':
                {
                    result += '\r';
                    break;
                }
                    
                // horizontal tab (0x09)
                case 't':
                {
                    //result += '\t'; JSON does not like tabs
                    result += ' ';
                    break;
                }
                    
            }
        }
        
        return result;
        
    }
    
    
    inline std::string json_escape_string(const std::string& s)
    {
        const auto space = extra_space(s);
        if (space == 0)
        {
            return s;
        }
        
        // create a result string of necessary size
        std::string result(s.size() + space, '\\');
        std::size_t pos = 0;
        
        for (const auto& c : s)
        {
            switch (c)
            {
                    // quotation mark (0x22)
                case '"':
                {
                    result[pos + 1] = '"';
                    pos += 2;
                    break;
                }
                    
                    // reverse solidus (0x5c)
                case '\\':
                {
                    // nothing to change
                    pos += 2;
                    break;
                }
                    
                    // backspace (0x08)
                case '\b':
                {
                    result[pos + 1] = 'b';
                    pos += 2;
                    break;
                }
                    
                    // formfeed (0x0c)
                case '\f':
                {
                    result[pos + 1] = 'f';
                    pos += 2;
                    break;
                }
                    
                    // newline (0x0a)
                case '\n':
                {
                    result[pos + 1] = 'n';
                    pos += 2;
                    break;
                }
                    
                    // carriage return (0x0d)
                case '\r':
                {
                    result[pos + 1] = 'r';
                    pos += 2;
                    break;
                }
                    
                    // horizontal tab (0x09)
                case '\t':
                {
                    result[pos + 1] = 't';
                    pos += 2;
                    break;
                }
                    
                default:
                {
                    if (c >= 0x00 and c <= 0x1f)
                    {
                        // convert a number 0..15 to its hex representation
                        // (0..f)
                        static const char hexify[16] =
                        {
                            '0', '1', '2', '3', '4', '5', '6', '7',
                            '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
                        };
                        
                        // print character c as \uxxxx
                        for (const char m :
                             { 'u', '0', '0', hexify[c >> 4], hexify[c & 0x0f]
                             })
                        {
                            result[++pos] = m;
                        }
                        
                        ++pos;
                    }
                    else
                    {
                        // all other characters are added as-is
                        result[pos++] = c;
                    }
                    break;
                }
            }
        }
        
        return result;
    }

} } // end namespace graphene::utilities

