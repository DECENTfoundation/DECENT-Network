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
#include <graphene/db/object_database.hpp>

#include <fc/io/raw.hpp>
#include <fc/container/flat.hpp>
#include <fc/uint128.hpp>

namespace graphene { namespace db {


object_database::object_database(uint8_t space_id_count, uint8_t local_type_id_count, uint8_t proto_type_id_count, uint8_t impl_type_id_count)
: _undo_db(*this)
, _space_id_count(space_id_count)
, _local_object_type_count(local_type_id_count)
, _proto_type_id_count(proto_type_id_count)
, _impl_type_id_count(impl_type_id_count)
{
   reset_indexes();

   _undo_db.enable();
}

object_database::~object_database(){}

void object_database::close()
{
}

const object* object_database::find_object( object_id_type id )const
{
   return get_index(id.space(),id.type()).find( id );
}
const object& object_database::get_object( object_id_type id )const
{
   return get_index(id.space(),id.type()).get( id );
}

const index& object_database::get_index(uint8_t space_id, uint8_t type_id)const
{
   if(space_id >= _index.size())
      FC_THROW_EXCEPTION(fc::invalid_space_id_exception, "");
   if(_index[space_id].size() <= type_id)
      FC_THROW_EXCEPTION(fc::invalid_type_id_exception, "");

   const auto& tmp = _index[space_id][type_id];
   FC_ASSERT( tmp );
   return *tmp;
}
index& object_database::get_mutable_index(uint8_t space_id, uint8_t type_id)
{
   FC_ASSERT( _index.size() > space_id, "", ("space_id",space_id)("type_id",type_id)("index.size",_index.size()) );
   FC_ASSERT( _index[space_id].size() > type_id , "", ("space_id",space_id)("type_id",type_id)("index[space_id].size",_index[space_id].size()) );
   const auto& idx = _index[space_id][type_id];
   FC_ASSERT( idx, "", ("space",space_id)("type",type_id) );
   return *idx;
}

void object_database::flush()
{
 //  ilog("Save object_database in ${d}", ("d", _data_dir));
   if( _data_dir.generic_string().size() == 0 )
      return;
   for( uint32_t space = 0; space < _index.size(); ++space )
   {
      fc::create_directories( _data_dir / "object_database" / fc::to_string(space) );
      const auto types = _index[space].size();
      for( uint32_t type = 0; type  <  types; ++type )
         if( _index[space][type] )
            _index[space][type]->save( _data_dir / "object_database" / fc::to_string(space)/fc::to_string(type) );
   }
}

void object_database::wipe(const fc::path& data_dir)
{
   close();
   ilog("Wiping object database...");
   fc::remove_all(data_dir / "object_database");
   ilog("Done wiping object databse.");
}


void object_database::open(const fc::path& data_dir)
{ try {
   ilog("Opening object database from ${d} ...", ("d", data_dir));
   _data_dir = data_dir;
   for( uint32_t space = 0; space < _index.size(); ++space )
      for( uint32_t type = 0; type  < _index[space].size(); ++type )
         if( _index[space][type] )
            _index[space][type]->open( _data_dir / "object_database" / fc::to_string(space)/fc::to_string(type) );
   ilog( "Done opening object database." );

} FC_CAPTURE_AND_RETHROW( (data_dir) ) }


void object_database::pop_undo()
{ try {
   _undo_db.pop_commit();
} FC_RETHROW() }

void object_database::save_undo( const object& obj )
{
   _undo_db.on_modify( obj );
}

void object_database::save_undo_add( const object& obj )
{
   _undo_db.on_create( obj );
}

void object_database::save_undo_remove(const object& obj)
{
   _undo_db.on_remove( obj );
}

} } // namespace graphene::db
