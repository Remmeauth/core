#pragma once

#include <eosio/eosio.hpp>


namespace eosio {

   class [[eosio::contract("rem.attribute")]] attribute : public contract {
   public:
      using contract::contract;

      [[eosio::action]]
      void create( const name& attribute_name, int32_t type, int32_t ptype );

      [[eosio::action]]
      void setbool( const name& issuer, const name& target, const name& attribute_name, bool value );

      using create_action = eosio::action_wrapper<"create"_n, &attribute::create>;
      using setbool_action = eosio::action_wrapper<"setbool"_n, &attribute::setbool>;

   private:
      enum class data_type { Boolean = 0, Int, LargeInt, MaxVal };
      enum class privacy_type { SelfAssigned = 0, PublicPointer, PublicConfirmedPointer, PrivatePointer, PrivateConfirmedPointer, MaxVal };

      struct [[eosio::table]] attribute_info {
         name          attribute_name;
         data_type     type;
         privacy_type  ptype;

         uint64_t primary_key() const { return attribute_name.value; }
      };

      struct [[eosio::table]] attribute_data {
         name               attribute_name;
         std::vector<char>  data;

         uint64_t primary_key() const { return attribute_name.value; }
      };

      typedef eosio::multi_index< "attrinfo"_n, attribute_info > attribute_info_table;
      typedef eosio::multi_index< "attributes"_n, attribute_data > attributes_table;
   };

} /// namespace eosio