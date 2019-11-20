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
#pragma once
#include <graphene/db/object_id.hpp>
#include <fc/io/raw.hpp>
#include <fc/crypto/city.hpp>
#include <fc/uint128.hpp>

namespace graphene { namespace db {

   /**
    *  @brief base for all database objects
    *
    *  The object is the fundamental building block of the database and
    *  is the level upon which undo/redo operations are performed.  Objects
    *  are used to track data and their relationships and provide an effecient
    *  means to find and update information.
    *
    *  Objects are assigned a unique and sequential object ID by the database within
    *  the id_space defined in the object.
    *
    *  All objects must be serializable via FC_REFLECT() and their content must be
    *  faithfully restored.   Additionally all objects must be copy-constructable and
    *  assignable in a relatively efficient manner.  In general this means that objects
    *  should only refer to other objects by ID and avoid expensive operations when
    *  they are copied, especially if they are modified frequently.
    *
    *  Additionally all objects may be annotated by plugins which wish to maintain
    *  additional information to an object.  There can be at most one annotation
    *  per id_space for each object.   An example of an annotation would be tracking
    *  extra data not required by validation such as the name and description of
    *  a user asset.  By carefully organizing how information is organized and
    *  tracked systems can minimize the workload to only that which is necessary
    *  to perform their function.
    *
    *  @note Do not use multiple inheritance with object because the code assumes
    *  a static_cast will work between object and derived types.
    */
   class object
   {
      public:
         object(){}
         virtual ~object(){}

         // serialized
         object_id_type          id;

         /// these methods are implemented for derived classes by inheriting abstract_object<DerivedClass>
         virtual std::unique_ptr<object> clone()const = 0;
         virtual void               move_from( object& obj ) = 0;
         virtual fc::variant        to_variant()const  = 0;
         virtual std::vector<char>  pack()const = 0;
         virtual fc::uint128        hash()const = 0;
   };

   /**
    * @class abstract_object
    * @brief   Use the Curiously Recurring Template Pattern to automatically add the ability to
    *  clone, serialize, and move objects polymorphically.
    *
    *  http://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
    */
   template<uint8_t SpaceID, uint8_t TypeID, typename DerivedClass>
   class abstract_object : public object
   {
      public:
         enum : uint8_t {
            space_id = SpaceID,
            type_id  = TypeID
         };

         object_id<SpaceID, TypeID, DerivedClass> get_id() const { return id; }

         virtual std::unique_ptr<object> clone()const
         {
            return std::unique_ptr<object>(new DerivedClass( *static_cast<const DerivedClass*>(this) ));
         }

         virtual void    move_from( object& obj )
         {
            static_cast<DerivedClass&>(*this) = std::move( static_cast<DerivedClass&>(obj) );
         }
         virtual fc::variant to_variant()const { return fc::variant( static_cast<const DerivedClass&>(*this) ); }
         virtual std::vector<char> pack()const  { return fc::raw::pack( static_cast<const DerivedClass&>(*this) ); }
         virtual fc::uint128  hash()const  {
             auto tmp = this->pack();
             return fc::city_hash_crc_128( tmp.data(), tmp.size() );
         }
   };

} } // graphene::db

FC_REFLECT( graphene::db::object, (id) )
