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
#include <graphene/db/object.hpp>
#include <graphene/db/index.hpp>
#include <graphene/db/undo_database.hpp>

#include <fc/log/logger.hpp>
#include <fc/optional.hpp>

#include <map>

namespace graphene { namespace db {

   /**
    *   @class object_database
    *   @brief maintains a set of indexed objects that can be modified with multi-level rollback support
    */
   class object_database
   {
      public:
         object_database(const std::vector< uint8_t >& object_type_count);
         ~object_database();

         void reset_indexes()
         {
            _index.clear();
            _index.resize(_object_type_count.size());
            for(uint8_t i = 0; i < _object_type_count.size(); i++) {
               _index[i].resize(_object_type_count[i]);
            }
         }

         void open(const boost::filesystem::path& data_dir );

         /**
          * Saves the complete state of the object_database to disk, this could take a while
          */
         void flush();
         void wipe(const boost::filesystem::path& data_dir); // remove from disk
         void close();

         template<typename T, typename F>
         const T& create( F&& constructor )
         {
            auto& idx = get_mutable_index<T>();
            return static_cast<const T&>( idx.create( [&](object& o)
            {
               assert( dynamic_cast<T*>(&o) );
               constructor( static_cast<T&>(o) );
            } ));
         }

         ///These methods are used to retrieve indexes on the object_database. All public index accessors are const-access only.
         /// @{
         template<typename IndexType>
         const IndexType& get_index_type()const {
            static_assert( std::is_base_of<index,IndexType>::value, "Type must be an index type" );
            return static_cast<const IndexType&>( get_index( IndexType::object_type::space_id, IndexType::object_type::type_id ) );
         }
         template<typename T>
         const index&  get_index()const { return get_index(T::space_id,T::type_id); }
         const index&  get_index(uint8_t space_id, uint8_t type_id)const;
         const index&  get_index(object_id_type id)const { return get_index(id.space(),id.type()); }
         /// @}

         const object& get_object( object_id_type id )const;
         const object* find_object( object_id_type id )const;

         /// These methods are mutators of the object_database. You must use these methods to make changes to the object_database,
         /// in order to maintain proper undo history.
         ///@{

         const object& insert( object&& obj ) { return get_mutable_index(obj.id).insert( std::move(obj) ); }
         void          remove( const object& obj ) { get_mutable_index(obj.id).remove( obj ); }
         template<typename T, typename Lambda>
         void modify( const T& obj, const Lambda& m ) {
            get_mutable_index(obj.id).modify(obj,m);
         }

         ///@}

         template<typename T>
         static const T& cast( const object& obj )
         {
            assert( nullptr != dynamic_cast<const T*>(&obj) );
            return static_cast<const T&>(obj);
         }
         template<typename T>
         static T& cast( object& obj )
         {
            assert( nullptr != dynamic_cast<T*>(&obj) );
            return static_cast<T&>(obj);
         }

         template<typename T>
         const T& get( object_id_type id )const
         {
            const object& obj = get_object( id );
            assert( nullptr != dynamic_cast<const T*>(&obj) );
            return static_cast<const T&>(obj);
         }
         template<typename T>
         const T* find( object_id_type id )const
         {
            const object* obj = find_object( id );
            assert(  !obj || nullptr != dynamic_cast<const T*>(obj) );
            return static_cast<const T*>(obj);
         }

         template<typename ObjectType>
         std::vector<fc::optional<ObjectType>> get_objects(const std::vector<object_id<ObjectType::space_id, ObjectType::type_id, ObjectType>>& ids) const
         {
            std::vector<fc::optional<ObjectType>> result;
            result.reserve(ids.size());
            std::transform(ids.begin(), ids.end(), std::back_inserter(result),
                           [this](object_id<ObjectType::space_id, ObjectType::type_id, ObjectType> id) -> fc::optional<ObjectType> {
                              if(auto o = find(id))
                              {
                                 return *o;
                              }
                              return {};
                           });
            return result;
         }

         template<uint8_t SpaceID, uint8_t TypeID, typename T>
         const T* find( object_id<SpaceID,TypeID,T> id )const { return find<T>(id); }

         template<uint8_t SpaceID, uint8_t TypeID, typename T>
         const T& get( object_id<SpaceID,TypeID,T> id )const { return get<T>(id); }

         template<typename IndexType>
         IndexType* add_index()
         {
            typedef typename IndexType::object_type ObjectType;
            if(_index[ObjectType::space_id].size() <= ObjectType::type_id)
               FC_ASSERT(false, "Index for type ${t} is not allocated", ("t", static_cast<uint8_t>(ObjectType::type_id)));
            std::unique_ptr<index> indexptr( new IndexType(*this) );
            _index[ObjectType::space_id][ObjectType::type_id] = std::move(indexptr);
            return static_cast<IndexType*>(_index[ObjectType::space_id][ObjectType::type_id].get());
         }

         void pop_undo();

         boost::filesystem::path get_data_dir()const { return _data_dir; }

         /** public for testing purposes only... should be private in practice. */
         undo_database                          _undo_db;
     protected:
         template<typename IndexType>
         IndexType&    get_mutable_index_type() {
            static_assert( std::is_base_of<index,IndexType>::value, "Type must be an index type" );
            return static_cast<IndexType&>( get_mutable_index( IndexType::object_type::space_id, IndexType::object_type::type_id ) );
         }
         template<typename T>
         index& get_mutable_index()                   { return get_mutable_index(T::space_id,T::type_id); }
         index& get_mutable_index(object_id_type id)  { return get_mutable_index(id.space(),id.type());   }
         index& get_mutable_index(uint8_t space_id, uint8_t type_id);

     private:

         friend class base_primary_index;
         friend class undo_database;
         void save_undo( const object& obj );
         void save_undo_add( const object& obj );
         void save_undo_remove( const object& obj );

         boost::filesystem::path                                   _data_dir;
         std::vector<std::vector<std::unique_ptr<index>>>          _index;
         std::vector<uint8_t>                                      _object_type_count;   // second level of two-dimensional array of indexes,
   };                                                                                    // first level is size of this vector

} } // graphene::db
