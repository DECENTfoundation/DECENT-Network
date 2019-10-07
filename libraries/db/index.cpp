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
#include <graphene/db/index.hpp>
#include <graphene/db/object_database.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/filesystem.hpp>
#include <fc/filesystem.hpp>
#include <fc/io/raw.hpp>
#include <fc/io/json.hpp>
#include <fstream>

namespace graphene { namespace db {

   void index::open( const boost::filesystem::path& db )
   { try{
      if( !exists( db ) )
         return;
      boost::interprocess::file_mapping fm( db.generic_string().c_str(), boost::interprocess::read_only );
      boost::interprocess::mapped_region mr( fm, boost::interprocess::read_only, 0, file_size(db) );
      fc::datastream<const char*> ds( (const char*)mr.get_address(), mr.get_size() );
      fc::sha256 open_ver;

      object_id_type next_id;
      fc::raw::unpack(ds, next_id);
      set_next_id(next_id);

      fc::raw::unpack(ds, open_ver);
      FC_ASSERT( open_ver == get_object_version(), "Incompatible Version, the serialization of objects in this index has changed" );
      try {
         vector<char> tmp;
         while( true )
         {
            fc::raw::unpack( ds, tmp );
            load( tmp );
         }
      } catch ( const fc::exception& ){}
   }FC_CAPTURE_AND_RETHROW((db))}

   void index::save( const boost::filesystem::path& db )
   {
      std::ofstream out( db.generic_string(), std::ofstream::binary | std::ofstream::out | std::ofstream::trunc );
      FC_ASSERT( out );
      fc::raw::pack( out, get_next_id() );
      fc::raw::pack( out, get_object_version() );
      inspect_all_objects( [&]( const object& o ) {
          auto packed_vec = fc::raw::pack( store( o ) );
          out.write( packed_vec.data(), packed_vec.size() );
      });
   }

   void base_primary_index::save_undo( const object& obj )
   { _db.save_undo( obj ); }

   void base_primary_index::on_add( const object& obj )
   {
      _db.save_undo_add( obj );
      for( auto ob : _observers ) ob->on_add( obj );
   }

   void base_primary_index::on_remove( const object& obj )
   { _db.save_undo_remove( obj ); for( auto ob : _observers ) ob->on_remove( obj ); }

   void base_primary_index::on_modify( const object& obj )
   {for( auto ob : _observers ) ob->on_modify(  obj ); }

} } // graphene::chain
