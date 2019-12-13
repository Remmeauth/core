#pragma once

#include <eosio/eosio.hpp>

namespace eosio {

   struct [[eosio::table, eosio::contract("rem.auth")]] attribute_info {
      name    attribute_name;
      int32_t type;
      int32_t ptype;
      bool valid = true;

      uint64_t next_id = 0;

      uint64_t primary_key() const { return attribute_name.value; }
      bool is_valid() const { return valid; }
   };
   typedef eosio::multi_index< "attrinfo"_n, attribute_info > attribute_info_table;


   struct attribute_t {
      std::vector<char>  data;
      std::vector<char>  pending;
   };

   struct [[eosio::table, eosio::contract("rem.auth")]] attribute_data {
      uint64_t     id;
      name         receiver;
      name         issuer;
      attribute_t  attribute;

      uint64_t primary_key() const { return id; }
      uint64_t by_receiver() const { return receiver.value; }
      uint64_t by_issuer()   const { return issuer.value; }
      uint128_t by_receiver_issuer() const { return combine_receiver_issuer(receiver, issuer); }

      static uint128_t combine_receiver_issuer(name receiver, name issuer)
      {
         uint128_t result = receiver.value;
         result <<= 64;
         result |= issuer.value;
         return result;
      }
   };
   typedef eosio::multi_index< "attributes"_n, attribute_data,
                               indexed_by<"reciss"_n, const_mem_fun<attribute_data, uint128_t, &attribute_data::by_receiver_issuer>  >
                               > attributes_table;

   class [[eosio::contract("rem.auth")]] attribute : public contract {
   public:
      using contract::contract;

      static bool has_attribute( const name& attr_contract_account, const name& issuer, const name& receiver, const name& attribute_name );

      template< class T >
      static T get_attribute( const name& attr_contract_account, const name& issuer, const name& receiver, const name& attribute_name );

      [[eosio::action]]
      void confirm( const name& owner, const name& issuer, const name& attribute_name );

      [[eosio::action]]
      void create( const name& attribute_name, int32_t type, int32_t ptype );

      [[eosio::action]]
      void invalidate( const name& attribute_name );

      [[eosio::action]]
      void remove( const name& attribute_name );

      [[eosio::action]]
      void setattr( const name& issuer, const name& receiver, const name& attribute_name, const std::vector<char>& value );

      [[eosio::action]]
      void unsetattr( const name& issuer, const name& receiver, const name& attribute_name );

      using confirm_action    = eosio::action_wrapper<"confirm"_n,       &attribute::confirm>;
      using create_action     = eosio::action_wrapper<"create"_n,         &attribute::create>;
      using invalidate_action = eosio::action_wrapper<"invalidate"_n, &attribute::invalidate>;
      using remove_action     = eosio::action_wrapper<"remove"_n,         &attribute::remove>;
      using setattr_action    = eosio::action_wrapper<"setattr"_n,       &attribute::setattr>;
      using unsetattr_action  = eosio::action_wrapper<"unsetattr"_n,   &attribute::unsetattr>;

   private:
      enum class data_type : int32_t { Boolean = 0, Int, LargeInt, Double, ChainAccount, UTFString, DateTimeUTC, CID, OID, Binary, Set, MaxVal };
      enum class privacy_type : int32_t { SelfAssigned = 0, PublicPointer, PublicConfirmedPointer, PrivatePointer, PrivateConfirmedPointer, MaxVal };

      void check_permission(const name& issuer, const name& receiver, int32_t ptype) const;
      bool need_confirm(int32_t ptype) const;
   };

   inline bool attribute::has_attribute( const name& attr_contract_account, const name& issuer, const name& receiver, const name& attribute_name )
   {
      attribute_info_table attributes_info{ attr_contract_account, attr_contract_account.value };
      const auto it = attributes_info.find( attribute_name.value );

      if ( it == attributes_info.end() ) {
         return false;
      }

      if ( !it->is_valid() ) {
         return false;
      }

      attributes_table attributes( attr_contract_account, attribute_name.value );
      auto idx = attributes.get_index<"reciss"_n>();
      const auto attr_it = idx.find( attribute_data::combine_receiver_issuer(receiver, issuer) );

      return attr_it != idx.end();
   }

   template< class T >
   T attribute::get_attribute( const name& attr_contract_account, const name& issuer, const name& receiver, const name& attribute_name )
   {
      attribute_info_table attributes_info{ attr_contract_account, attr_contract_account.value };
      const auto& attribute_info = attributes_info.get( attribute_name.value, "attribute doesn't exist" );
      check( attribute_info.is_valid(), "attribute is marked for deletion" );

      attributes_table attributes( attr_contract_account, attribute_name.value );
      const auto& idx = attributes.get_index<"reciss"_n>();
      const auto& attr = idx.get( attribute_data::combine_receiver_issuer(receiver, issuer), "attribute not set by issuer to receiver" );

      const T value = unpack< T >( attr.attribute.data.data(), sizeof( T ) );

      return value;
   }
} /// namespace eosio
