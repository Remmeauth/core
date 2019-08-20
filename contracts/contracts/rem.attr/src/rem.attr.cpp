#include <rem.attr/rem.attr.hpp>


namespace eosio {

   void attribute::confirm( const name& owner, const name& attribute_name )
   {
      require_auth(owner);

      attributes_table attributes( _self, owner.value );
      const auto& attr = attributes.get( attribute_name.value, "account does not have this attribute" );
      attributes.modify( attr, same_payer, [&]( auto& attr ) {
         attr.data.swap(attr.pending);
         attr.pending.clear();
      });
   }

   void attribute::create( const name& attribute_name, int32_t type, int32_t ptype )
   {
      require_auth( _self );
      check( type >= 0 && type < static_cast<int32_t>( data_type::MaxVal ), "attribute type is out of range" );
      check( ptype >= 0 && ptype < static_cast<int32_t>( privacy_type::MaxVal ), "attribute privacy type is out of range" );

      attribute_info_table attributes_info( _self, attribute_name.value );
      check( attributes_info.find( attribute_name.value ) == attributes_info.end(), "attribute with this name already exists" );

      attributes_info.emplace( _self, [&]( auto& attr ) {
         attr.attribute_name = attribute_name;
         attr.type           = type;
         attr.ptype          = ptype;
      });
   }

   void attribute::setattr( const name& issuer, const name& receiver, const name& attribute_name, const std::vector<char>& value )
   {
      require_auth( issuer );
      require_recipient( receiver );
      check( !value.empty(), "value is empty" );

      attribute_info_table attributes_info( _self, attribute_name.value );
      const auto& attrinfo = attributes_info.get( attribute_name.value, "attribute does not exist" );
      check_create_permission(issuer, receiver, attrinfo.ptype);

      const auto attribute_setter = [&]( auto& attr ) {
         attr.attribute_name = attribute_name;
         attr.issuer         = issuer;
         if (need_confirm(attrinfo.ptype)) {
            attr.pending = value;
         }
         else {
            attr.data = value;
         }
      };

      attributes_table attributes( _self, receiver.value );
      const auto attr_it = attributes.find( attribute_name.value );
      if ( attr_it == attributes.end() ) {
         attributes.emplace( issuer, attribute_setter);
      } else {
         check_delete_permission(attr_it->issuer, issuer, receiver, attrinfo.ptype);
         attributes.modify( attr_it, issuer, attribute_setter);
      }
   }

   void attribute::unsetattr( const name& issuer, const name& receiver, const name& attribute_name )
   {
      require_auth( issuer );

      attribute_info_table attributes_info( _self, attribute_name.value );
      const auto& attrinfo = attributes_info.get( attribute_name.value, "attribute does not exist" );

      require_recipient( receiver );

      attributes_table attributes( _self, receiver.value );
      const auto& attr = attributes.get( attribute_name.value, "attribute hasn`t been set for account" );
      check_delete_permission(attr.issuer, issuer, receiver, attrinfo.ptype);

      attributes.erase(attr);
   }

   void attribute::check_create_permission(const name& issuer, const name& receiver, int32_t ptype) const
   {
      if (static_cast<privacy_type>(ptype) == privacy_type::SelfAssigned) {
         check(issuer == receiver, "this attribute can only be self-assigned");
      }
      else if (static_cast<privacy_type>(ptype) == privacy_type::PrivatePointer ||
         static_cast<privacy_type>(ptype) == privacy_type::PrivateConfirmedPointer) {
         check(issuer == _self, "only contract owner can assign this attribute");
      }
   }

   void attribute::check_delete_permission(const name& initial_issuer, const name& issuer, const name& receiver, int32_t ptype) const
   {
      bool is_creator = initial_issuer == issuer;
      bool is_confirmable = static_cast<privacy_type>(ptype) == privacy_type::PublicConfirmedPointer ||
         static_cast<privacy_type>(ptype) == privacy_type::PrivateConfirmedPointer;
      check(is_creator || (is_confirmable && issuer == receiver), "only creator or receiver (in case of confirmable attribute) can unset attribute");
   }

   bool attribute::need_confirm(int32_t ptype) const
   {
      return static_cast<privacy_type>(ptype) == privacy_type::PublicConfirmedPointer ||
         static_cast<privacy_type>(ptype) == privacy_type::PrivateConfirmedPointer;
   }

} /// namespace eosio

EOSIO_DISPATCH( eosio::attribute, (confirm)(create)(setattr)(unsetattr) )