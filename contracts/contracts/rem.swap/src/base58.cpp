/**
 *  @copyright defined in eos/LICENSE.txt
 */

namespace eosio {

   const char base58_chars[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

   bool map_initialized = false;
   std::array<int8_t, 256> base58_map{{0}};
   auto get_base58_map() {
      if(!map_initialized) {
         for (unsigned i = 0; i < base58_map.size(); ++i)
            base58_map[i] = -1;
         for (unsigned i = 0; i < sizeof(base58_chars); ++i)
            base58_map[base58_chars[i]] = i;
         map_initialized = true;
      }
      return base58_map;
   }

   template <size_t size>
   std::array<uint8_t, size> base58_to_binary(std::string_view s) {
      std::array<uint8_t, size> result{{0}};
      for (auto& src_digit : s) {
         int carry = get_base58_map()[src_digit];
         if (carry < 0)
            check(0, "invalid base-58 value");
         for (auto& result_byte : result) {
            int x = result_byte * 58 + carry;
            result_byte = x;
            carry = x >> 8;
         }
         if (carry)
            check(0, "base-58 value is out of range");
      }
      std::reverse(result.begin(), result.end());
      return result;
   }

   eosio::public_key string_to_public_key(std::string_view s) {
      eosio::public_key key;
      bool is_k1_type = s.size() >= 3 && ( s.substr(0, 3) == "EOS" || s.substr(0, 3) == "REM");
      bool is_r1_type = s.size() >= 7 && s.substr(0, 7) == "PUB_R1_";

      check(is_k1_type || is_r1_type, "unrecognized public key format");
      auto whole = base58_to_binary<37>( is_k1_type ? s.substr(3) : s.substr(7) );
      check(whole.size() == std::get_if<0>(&key)->size() + 4, "invalid public key length");

      memcpy(std::get_if<0>(&key)->data(), whole.data(), std::get_if<0>(&key)->size());
      return key;
   }

} // namespace eosio
