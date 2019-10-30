/**
 *  @copyright defined in eos/LICENSE.txt
 */

#include <rem.system/rem.system.hpp>
#include <rem.utils/rem.utils.hpp>

#define USE_KECCAK
#include <sha3/sha3.c>

#include "validate_address.cpp"

namespace eosio {
   using eosiosystem::system_contract;

   void utils::validateaddr( const name& chain_id, const string& address ) {
      chains_index chains_table(swap_contract, swap_contract.value);
      auto it = chains_table.find(chain_id.value);
      check(it != chains_table.end() && it->output, "not supported chain id");

      if (chain_id.to_string() == "ethropsten" || chain_id.to_string() == "eth") {
         validate_eth_address(address);
      }
   }
} /// namespace eosio

EOSIO_DISPATCH( eosio::utils, (validateaddr) )
