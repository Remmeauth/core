#include <rem.attr/rem.attr.hpp>


namespace eosio {

   void attribute::confirm( const name& owner, const name& issuer, const name& attribute_name )
   {
      require_auth(owner);

      attributes_table attributes( _self, attribute_name.value );
      auto idx = attributes.get_index<"reciss"_n>();
      const auto attr_it = idx.find( attribute_data::combine_receiver_issuer(owner, issuer) );
      check( attr_it != idx.end() && !attr_it->attribute.pending.empty(), "nothing to confirm" );
      idx.modify( attr_it, same_payer, [&]( auto& attr ) {
         attr.attribute.data.swap(attr.attribute.pending);
         attr.attribute.pending.clear();
      });
   }

   void attribute::create( const name& attribute_name, int32_t type, int32_t ptype )
   {
      require_auth( _self );
      check( type >= 0 && type < static_cast<int32_t>( data_type::MaxVal ), "attribute type is out of range" );
      check( ptype >= 0 && ptype < static_cast<int32_t>( privacy_type::MaxVal ), "attribute privacy type is out of range" );

      attribute_info_table attributes_info( _self, _self.value );
      check( attributes_info.find( attribute_name.value ) == attributes_info.end(), "attribute with this name already exists" );

      attributes_info.emplace( _self, [&]( auto& attr ) {
         attr.attribute_name = attribute_name;
         attr.type           = type;
         attr.ptype          = ptype;
      });
   }

   void attribute::invalidate( const name& attribute_name )
   {
      require_auth( _self );

      attribute_info_table attributes_info( _self, _self.value );
      const auto& attrinfo = attributes_info.get( attribute_name.value, "attribute does not exist" );

      attributes_info.modify(attrinfo, same_payer, [&]( auto& a ) {
         a.valid = false;
      });
   }

   void attribute::remove( const name& attribute_name )
   {
      require_auth( _self );

      attribute_info_table attributes_info( _self, _self.value );
      const auto& attrinfo = attributes_info.get( attribute_name.value, "attribute does not exist" );
      check( !attrinfo.is_valid(), "call invalidate first" );

      attributes_table attributes( _self, attribute_name.value );
      check( attributes.begin() == attributes.end(), "unable to delete" );
      attributes_info.erase(attrinfo);
   }

   void attribute::setattr( const name& issuer, const name& receiver, const name& attribute_name, const std::vector<char>& value )
   {
      require_auth( issuer );
      require_recipient( receiver );

      attribute_info_table attributes_info( _self, _self.value );
      const auto& attrinfo = attributes_info.get( attribute_name.value, "attribute does not exist" );
      check(attrinfo.next_id < std::numeric_limits<uint64_t>::max(), "attribute storage is full");
      check( attrinfo.is_valid(), "this attribute is beeing deleted" );
      check_permission(issuer, receiver, attrinfo.ptype);
      check_attribute_data(value, attrinfo.type);

      const auto id = attrinfo.next_id;

      attributes_table attributes( _self, attribute_name.value );
      auto idx = attributes.get_index<"reciss"_n>();
      const auto attr_it = idx.find( attribute_data::combine_receiver_issuer(receiver, issuer) );
      if ( attr_it == idx.end() ) {
         attributes.emplace( issuer, [&]( auto& attr ) {
            attr.id = id;
            attr.issuer = issuer;
            attr.receiver = receiver;
            if (need_confirm(attrinfo.ptype)) {
               attr.attribute.pending = value;
            }
            else {
               attr.attribute.data = value;
            }
         });
         attributes_info.modify(attrinfo, same_payer, [&]( auto& a ) {
            a.next_id += 1;
         });
      } else {
         idx.modify( attr_it, issuer, [&]( auto& attr ) {
            attr.issuer = issuer;
            attr.receiver = receiver;
            if (need_confirm(attrinfo.ptype)) {
               attr.attribute.pending = value;
            }
            else {
               attr.attribute.data = value;
            }
         });
      }
   }

   void attribute::unsetattr( const name& issuer, const name& receiver, const name& attribute_name )
   {
      attribute_info_table attributes_info( _self, _self.value );
      const auto& attrinfo = attributes_info.get( attribute_name.value, "attribute does not exist" );

      if (attrinfo.is_valid()) { // when attribute became invalid anyone can unset
         if (need_confirm(attrinfo.ptype)) {
            check(has_auth(issuer) || has_auth(receiver), "missing required authority");
         } else {
            require_auth(issuer);
         }
      }
      require_recipient( receiver );

      attributes_table attributes( _self, attribute_name.value );
      auto idx = attributes.get_index<"reciss"_n>();
      const auto attr_it = idx.require_find( attribute_data::combine_receiver_issuer(receiver, issuer), "attribute hasn`t been set for account" );
      const auto erased_id = attr_it->id;
      idx.erase(attr_it);
      if (erased_id != attrinfo.next_id - 1) {
         const auto& attr_to_move = attributes.get(attrinfo.next_id - 1);
         auto moved_attr = attr_to_move;
         moved_attr.id = erased_id;
         attributes.erase(attr_to_move); //erase before emplace to avoid error when ram payer has no free RAM
         attributes.emplace( attr_to_move.issuer, [&]( auto& attr ) {
            attr = moved_attr;
         });
      }
      attributes_info.modify(attrinfo, same_payer, [&]( auto& a ) {
         a.next_id -= 1;
      });
   }

   void attribute::check_attribute_data(const std::vector<char>& data, int32_t type) const
   {
      check( !data.empty(), "value is empty" );
      switch(static_cast<data_type>( type )) {
         case data_type::Boolean:
            check( data.size() == 1, "invalid Boolean value" );
            break;
         case data_type::Int:
            check( data.size() == sizeof(int32_t), "invalid Int value" );
            break;
         case data_type::LargeInt:
            check( data.size() == sizeof(int64_t), "invalid LargeInt value" );
            break;
         case data_type::ChainAccount:
            check( data.size() == 40, "invalid ChainAccount value" );
            break;
         case data_type::UTFString:
            check( data.size() == data.front() + 1, "invalid UTFString value" );
            break;
         case data_type::DateTimeUTC:
            check( data.size() == sizeof(int64_t), "invalid DateTimeUTC value" );
            break;
         case data_type::Binary:
            check( data.size() == data.front() + 1, "invalid Binary value" );
            break;
         case data_type::Double:
            check( data.size() == sizeof(double), "invalid Double value" );
            break;
         case data_type::CID:
         case data_type::OID:
         case data_type::Set:
         default:
            break;
      }
   }

   void attribute::check_permission(const name& issuer, const name& receiver, int32_t ptype) const
   {
      if (static_cast<privacy_type>(ptype) == privacy_type::SelfAssigned) {
         check(issuer == receiver, "this attribute can only be self-assigned");
      }
      else if (static_cast<privacy_type>(ptype) == privacy_type::PrivatePointer ||
         static_cast<privacy_type>(ptype) == privacy_type::PrivateConfirmedPointer) {
         check(issuer == _self, "only contract owner can assign this attribute");
      }
   }

   bool attribute::need_confirm(int32_t ptype) const
   {
      return static_cast<privacy_type>(ptype) == privacy_type::PublicConfirmedPointer ||
         static_cast<privacy_type>(ptype) == privacy_type::PrivateConfirmedPointer;
   }
} /// namespace eosio
