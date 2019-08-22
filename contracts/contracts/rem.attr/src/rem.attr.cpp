#include <rem.attr/rem.attr.hpp>


namespace eosio {

   void attribute::confirm( const name& owner, const name& issuer, const name& attribute_name )
   {
      require_auth(owner);

      attributes_table attributes( _self, owner.value );
      const auto& attr = attributes.get( issuer.value, "account does not have this attribute set by provided issuer" );
      const auto attr_it = attr.attributes.find(attribute_name);
      check( attr_it != attr.attributes.end(), "account does not have this attribute set by provided issuer" );
      check( !attr_it->second.pending.empty(), "nothing to confirm" );
      attributes.modify( attr, same_payer, [&]( auto& attr ) {
         auto& account_attribute = attr.attributes[attribute_name];
         account_attribute.data.swap(account_attribute.pending);
         account_attribute.pending.clear();
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
      check_permission(issuer, receiver, attrinfo.ptype);

      const auto attribute_setter = [&]( auto& attr ) {
         attr.issuer = issuer;
         if (need_confirm(attrinfo.ptype)) {
            attr.attributes[attribute_name].pending = value;
         }
         else {
            attr.attributes[attribute_name].data = value;
         }
      };

      attributes_table attributes( _self, receiver.value );
      const auto attr_it = attributes.find( issuer.value );
      if ( attr_it == attributes.end() ) {
         attributes.emplace( issuer, attribute_setter);
      } else {
         attributes.modify( attr_it, issuer, attribute_setter);
      }
   }

   void attribute::unsetattr( const name& issuer, const name& receiver, const name& attribute_name )
   {
      attribute_info_table attributes_info( _self, attribute_name.value );
      const auto& attrinfo = attributes_info.get( attribute_name.value, "attribute does not exist" );

      if (need_confirm(attrinfo.ptype)) {
         check( has_auth(issuer) || has_auth(receiver), "missing required authority" );
      }
      else {
         require_auth( issuer );
      }
      require_recipient( receiver );

      attributes_table attributes( _self, receiver.value );
      const auto& attr = attributes.get( issuer.value, "attribute hasn`t been set for account" );
      const auto attr_it = attr.attributes.find(attribute_name);
      check( attr_it != attr.attributes.end(), "attribute hasn`t been set for account" );
      attributes.modify( attr, issuer, [&]( auto& attr ) {
         attr.attributes.erase(attr_it);
      });
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

EOSIO_DISPATCH( eosio::attribute, (confirm)(create)(setattr)(unsetattr) )