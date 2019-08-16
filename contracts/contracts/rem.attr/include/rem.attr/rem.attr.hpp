#pragma once

#include <eosio/eosio.hpp>


namespace eosio {

   class [[eosio::contract("rem.attr")]] attribute : public contract {
   public:
      using contract::contract;

      [[eosio::action]]
      void confirm( const name& owner, const name& attribute_name );

      [[eosio::action]]
      void create( const name& attribute_name, int32_t type, int32_t ptype );

      [[eosio::action]]
      void setattr( const name& issuer, const name& receiver, const name& attribute_name, const std::vector<char>& value );

      [[eosio::action]]
      void unsetattr( const name& issuer, const name& receiver, const name& attribute_name );

      using confirm_action   = eosio::action_wrapper<"confirm"_n,     &attribute::confirm>;
      using create_action    = eosio::action_wrapper<"create"_n,       &attribute::create>;
      using setattr_action   = eosio::action_wrapper<"setattr"_n,     &attribute::setattr>;
      using unsetattr_action = eosio::action_wrapper<"unsetattr"_n, &attribute::unsetattr>;

   private:
      enum class data_type : int32_t { Boolean = 0, Int, LargeInt, ChainAccount, UTFString, DateTimeUTC, CID, OID, Binary, Set, MaxVal };
      enum class privacy_type : int32_t { SelfAssigned = 0, PublicPointer, PublicConfirmedPointer, PrivatePointer, PrivateConfirmedPointer, MaxVal };

      struct [[eosio::table]] attribute_info {
         name    attribute_name;
         int32_t type;
         int32_t ptype;

         uint64_t primary_key() const { return attribute_name.value; }
      };

      struct [[eosio::table]] attribute_data {
         name               attribute_name;
         name               issuer;
         std::vector<char>  data;
         std::vector<char>  pending;

         uint64_t primary_key() const { return attribute_name.value; }
      };

      typedef eosio::multi_index< "attrinfo"_n, attribute_info > attribute_info_table;
      typedef eosio::multi_index< "attributes"_n, attribute_data > attributes_table;

      void check_create_permission(const name& issuer, const name& receiver, int32_t ptype) const;
      void check_delete_permission(const name& initial_issuer, const name& issuer, const name& receiver, int32_t ptype) const;
      bool need_confirm(int32_t ptype) const;
   };

} /// namespace eosio