/**
 *  @copyright defined in eos/LICENSE.txt
 */

#include <rem.utils/rem.utils.hpp>

namespace eosio {

   void utils::validateaddr( const name& chain_id, const string& address ) {
      if (chain_id.to_string() == "ethropsten" || chain_id.to_string() == "eth") {
         validate_eth_address(address);
      }
   }
} /// namespace eosio

EOSIO_DISPATCH( eosio::utils, (validateaddr) )
