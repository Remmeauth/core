/**
 *  @copyright defined in eos/LICENSE.txt
 */

#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include <map>
#include <string>

namespace eosio {

   using std::string;

   /**
    * @defgroup eosioutils rem.utils
    * @ingroup eosiocontracts
    *
    * rem.utils contract
    *
    * @details rem.utils contract defines the structures and actions that allow users and contracts to use helpful
    * tools. Implement validation address another blockchain.
    * @{
    */
   class [[eosio::contract("rem.utils")]] utils : public contract {
   public:
      using contract::contract;

      /**
       * Validate address action.
       *
       * @details Validation blockchain address.
       *
       * @param name - the chain id validation for,
       * @param address - the address in the corresponding chain network.
       */
      [[eosio::action]]
      void validateaddr( const name& chain_id, const string& address );

      using validate_address_action = action_wrapper<"validateaddr"_n, &utils::validateaddr>;
   private:
      // TODO: when rem.swap will be merge, delete table
      static constexpr name swap_contract = "rem.swap"_n;

      struct [[eosio::table]] chains {
         name chain;
         bool input;
         bool output;

         uint64_t primary_key() const { return chain.value; }

         // explicit serialization macro is not necessary, used here only to improve compilation time
         EOSLIB_SERIALIZE( chains, (chain)(input)(output))
      };
      typedef multi_index<"chains"_n, chains> chains_index;

      void validate_eth_address(string address);
      void validate_eth_address_checksum(string checksum_address);
      string bytetohex(unsigned char *data, int len);
      std::array<unsigned char, 32> sha3_256(const string& address);
   };
   /** @}*/ // end of @defgroup eosioutils rem.utils
} /// namespace eosio
