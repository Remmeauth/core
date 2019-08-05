#include <rem.attribute/rem.attribute.hpp>


namespace eosio {

   void attribute::confirm( const name& owner, const name& attribute_name )
   {
      require_auth(owner);

      attributes_table attributes( _self, owner.value );
      const auto& attr = attributes.get( attribute_name.value, "account does not have this attribute" );
      attributes.modify( attr, same_payer, [&]( auto& attr ) {
         attr.confirmed = true;
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
         attr.type           = static_cast<data_type>( type );
         attr.ptype          = static_cast<privacy_type>( ptype );
      });
   }

   void attribute::setbool( const name& issuer, const name& target, const name& attribute_name, bool value )
   {
      require_auth( issuer );

      attribute_info_table attributes_info( _self, attribute_name.value );
      const auto& attrinfo = attributes_info.get( attribute_name.value, "attribute does not exist" );
      check( attrinfo.type == data_type::Boolean, "invalid attribute type (Boolean)" );
      check_privacy(issuer, target, attrinfo.ptype);

      attributes_table attributes( _self, target.value );
      const auto attr_it = attributes.find( attribute_name.value );
      if ( attr_it == attributes.end() ) {
         attributes.emplace( issuer, [&]( auto& attr ) {
            attr.attribute_name = attribute_name;
            attr.data           = pack(value);
            attr.confirmed      = !need_confirm(attrinfo.ptype);
         });
      } else {
         attributes.modify( attr_it, issuer, [&]( auto& attr ) {
            attr.data           = pack(value);
            attr.confirmed      = !need_confirm(attrinfo.ptype);
         });
      }
   }

   void attribute::check_privacy(const name& issuer, const name& target, privacy_type ptype) const
   {
      switch (ptype) {
         case privacy_type::SelfAssigned:
            check(issuer == target, "self-assigned check"); //TODO: change message
            break;
         case privacy_type::PublicPointer:
            break;
         case privacy_type::PublicConfirmedPointer:
            break;
         case privacy_type::PrivatePointer:
            check(issuer == _self, "private pointer check"); //TODO: change message
            break;
         case privacy_type::PrivateConfirmedPointer:
            check(issuer == _self, "private confirmed pointer check"); //TODO: change message
            break;
      }
   }

   bool attribute::need_confirm(privacy_type ptype) const
   {
      return ptype == privacy_type::PublicConfirmedPointer ||
         ptype == privacy_type::PrivateConfirmedPointer;
   }

} /// namespace eosio

EOSIO_DISPATCH( eosio::attribute, (confirm)(create)(setbool) )