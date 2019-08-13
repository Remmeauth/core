#include <rem.attr/rem.attr.hpp>


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
         attr.type           = type;
         attr.ptype          = ptype;
      });
   }

   void attribute::setattr( const name& issuer, const name& receiver, const name& attribute_name, const std::vector<char>& value )
   {
      require_auth( issuer );
      check( !value.empty(), "value is empty" );

      attribute_info_table attributes_info( _self, attribute_name.value );
      const auto& attrinfo = attributes_info.get( attribute_name.value, "attribute does not exist" );
      check_privacy(issuer, receiver, attrinfo.ptype);

      require_recipient( receiver );

      attributes_table attributes( _self, receiver.value );
      const auto attr_it = attributes.find( attribute_name.value );
      if ( attr_it == attributes.end() ) {
         attributes.emplace( issuer, [&]( auto& attr ) {
            attr.attribute_name = attribute_name;
            attr.issuer         = issuer;
            attr.data           = value;
            attr.confirmed      = !need_confirm(attrinfo.ptype);
         });
      } else { //TODO: does caller can update attribute?
         attributes.modify( attr_it, issuer, [&]( auto& attr ) {
            attr.issuer         = issuer;
            attr.data           = value;
            attr.confirmed      = !need_confirm(attrinfo.ptype);
         });
      }
   }

   void attribute::unsetattr( const name& issuer, const name& receiver, const name& attribute_name )
   {
      require_auth( issuer );

      attribute_info_table attributes_info( _self, attribute_name.value );
      const auto& attrinfo = attributes_info.get( attribute_name.value, "attribute does not exist" );
      check_privacy(issuer, receiver, attrinfo.ptype);

      require_recipient( receiver );

      attributes_table attributes( _self, receiver.value );
      const auto& attr = attributes.get( attribute_name.value, "attribute hasn`t been set for account" );
      check( attr.issuer == issuer, "only account that assigned attribute can unset it" );

      attributes.erase(attr);
   }

   void attribute::check_privacy(const name& issuer, const name& receiver, int32_t ptype) const
   {
      switch (static_cast<privacy_type>(ptype)) {
         case privacy_type::SelfAssigned:
            check(issuer == receiver, "self-assigned check"); //TODO: change message
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

   bool attribute::need_confirm(int32_t ptype) const
   {
      return static_cast<privacy_type>(ptype) == privacy_type::PublicConfirmedPointer ||
         static_cast<privacy_type>(ptype) == privacy_type::PrivateConfirmedPointer;
   }

} /// namespace eosio

EOSIO_DISPATCH( eosio::attribute, (confirm)(create)(setattr)(unsetattr) )