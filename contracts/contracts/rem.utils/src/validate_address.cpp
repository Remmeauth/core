/**
 *  @copyright defined in eos/LICENSE.txt
 */

#include <rem.utils/rem.utils.hpp>

namespace eosio {
   void utils::validate_eth_address(string address) {
      if ( address.substr(0, 2) == "0x" ) { address = address.substr(2); }

      check( address.size() == 40, "invalid address length" );
      for (const auto& ch: address) {
         check(std::isxdigit(ch), "invalid hex symbol in ethereum address");
      }

      if (!is_lower(address)) {
         validate_eth_address_checksum(address);
      }
   }

   void utils::validate_eth_address_checksum(string checksum_address) {
      string address = checksum_address;
      std::transform(address.begin(), address.end(), address.begin(), ::tolower);

      auto address_hash = sha3_256(address);
      string address_hash_str = bytetohex((unsigned char*)(&address_hash), sizeof(address_hash));

      char pivot;
      for (int i = 0; i < address.size(); ++i) {
         int v = (toupper(address_hash_str[i]) >= 'A') ? (toupper(address_hash_str[i]) - 'A' + 10) :
                                                         (toupper(address_hash_str[i]) - '0');
         pivot = v >= 8 ? toupper(address[i]) : tolower(address[i]);
         check(pivot == checksum_address[i], "invalid ethereum address checksum");
      }
   }

   std::array<unsigned char, 32> utils::sha3_256(const string& address) {
      sha3_ctx shactx;
      rhash_keccak_256_init(&shactx);
      checksum256 result;
      auto result_data = result.extract_as_byte_array();
      rhash_keccak_update(&shactx, (const unsigned char*)address.c_str(), 40);
      rhash_keccak_final(&shactx, result_data.data());
      return result_data;
   }

   string utils::bytetohex(unsigned char *data, int len) {
      constexpr char hexmap[] = { '0', '1', '2', '3', '4', '5', '6', '7',
                                  '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

      std::string s(len * 2, ' ');
      for (int i = 0; i < len; ++i) {
         s[2 * i]     = hexmap[(data[i] & 0xF0) >> 4];
         s[2 * i + 1] = hexmap[data[i] & 0x0F];
      }
      return s;
   }

   bool utils::is_lower(const string &address) {
      string lower_address = address;
      std::transform(lower_address.begin(), lower_address.end(), lower_address.begin(), ::tolower);

      return lower_address == address;
   }
} /// namespace eosio
