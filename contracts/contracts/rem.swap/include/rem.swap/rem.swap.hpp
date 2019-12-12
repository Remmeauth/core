/**
 *  @copyright defined in eos/LICENSE.txt
 */

#pragma once

#include <eosio/action.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/singleton.hpp>
#include <eosio/eosio.hpp>
#include <eosio/time.hpp>

#include <numeric>

namespace eosio {

   using std::string;
   using std::vector;

   /**
    * @defgroup remswap rem.swap
    * @ingroup eosiocontracts
    *
    * rem.swap contract
    *
    * @details rem.swap contract defines the structures and actions that allow users to init token swap,
    * finish token swap, create new account and cancel token swap.
    * @{
    */
   class [[eosio::contract("rem.swap")]] swap : public contract {
   public:
      swap(name receiver, name code, datastream<const char *> ds);

      /**
       * Initiate token swap action.
       *
       * @details Initiate token swap in remchain.
       *
       * @param rampayer - the owner account to execute the init action for,
       * @param txid - the transfer transaction id in blockchain,
       * @param swap_pubkey - the public key created for the token swap,
       * @param quantity - the quantity of tokens in the transfer transaction,
       * @param return_address - the address that will receive swap tokens back,
       * @param return_chain_id - the chain identifier of the return address,
       * @param swap_timestamp - the timestamp transfer transaction in blockchain.
       */
      [[eosio::action]]
      void init(const name &rampayer, const string &txid, const string &swap_pubkey,
                const asset &quantity, const string &return_address, const string &return_chain_id,
                const block_timestamp &swap_timestamp);

      /**
       * Cancel token swap action.
       *
       * @details Cancel already initialized token swap.
       *
       * @param rampayer - the owner account to execute the cancel action for,
       * @param txid - the transfer transaction identifier in blockchain,
       * @param swap_pubkey - the public key created for the token swap,
       * @param quantity - the quantity of tokens in the transfer transaction,
       * @param return_address - the address that will receive swap tokens back,
       * @param return_chain_id - the chain identifier of the return address,
       * @param swap_timestamp - the timestamp transfer transaction in blockchain.
       */
      [[eosio::action]]
      void cancel(const name &rampayer, const string &txid, const string &swap_pubkey_str,
                  asset &quantity, const string &return_address, const string &return_chain_id,
                  const block_timestamp &swap_timestamp);

      /**
       * Finish token swap action.
       *
       * @details Finish already approved token swap.
       *
       * @param rampayer - the owner account to execute the finish action for,
       * @param receiver - the account to be swap finished to,
       * @param txid - the transfer transaction identifier in blockchain,
       * @param swap_pubkey - the public key created for the token swap,
       * @param quantity - the quantity of tokens in the transfer transaction,
       * @param return_address - the address that will receive swap tokens back,
       * @param return_chain_id - the chain identifier of the return address,
       * @param swap_timestamp - the timestamp transfer transaction in blockchain,
       * @param sign - the signature of payload that signed by swap_pubkey.
       */
      [[eosio::action]]
      void finish(const name &rampayer, const name &receiver, const string &txid, const string &swap_pubkey_str,
                  asset &quantity, const string &return_address, const string &return_chain_id,
                  const block_timestamp &swap_timestamp, const signature &sign);

      /**
       * Finish token swap action.
       *
       * @details Finish already approved token swap and create new account.
       *
       * @param rampayer - the owner account to execute the finish action for,
       * @param receiver - the account to be swap finished to,
       * @param owner_key - owner public key of the account to be created,
       * @param active_key - active public key of the account to be created,
       * @param txid - the transfer transaction identifier in blockchain,
       * @param swap_pubkey - the public key created for the token swap,
       * @param quantity - the quantity of tokens in the transfer transaction,
       * @param return_address - the address that will receive swap tokens back,
       * @param return_chain_id - the chain identifier of the return address,
       * @param swap_timestamp - the timestamp transfer transaction in blockchain,
       * @param sign - the signature of payload that signed by swap_pubkey.
       */
      [[eosio::action]]
      void finishnewacc(const name &rampayer, const name &receiver, const string &owner_pubkey_str,
                        const string &active_pubkey_str, const string &txid, const string &swap_pubkey_str,
                        asset &quantity, const string &return_address, const string &return_chain_id,
                        const block_timestamp &swap_timestamp, const signature &sign);

      /**
       * Set token swap contract parameters action.
       *
       * @details Change token swap contract parameters, action permitted only for producers.
       *
       * @param chain_id - the chain identifier of the remchain,
       * @param eth_swap_contract_address - the address of the ethereum token swap contract,
       * @param eth_return_chainid - the chain identifier of the ethereum network for out token swap.
       */
      [[eosio::action]]
      void setswapparam(const string &chain_id, const string &eth_swap_contract_address, const string &eth_return_chainid);

      /**
       * Add supported chain identifier action.
       *
       * @details Add supported chain identifier, action permitted only for producers.
       *
       * @param chain_id - the chain identifier to be added,
       * @param input - is supported input way to swap tokens according to chain identifier,
       * @param output - is supported output way to swap tokens according to chain identifier,
       * @param in_swap_min_amount - the minimum amount to swap tokens from chain_id to remchain,
       * @param out_swap_min_amount - the minimum amount to swap tokens from remchain to chain_id.
       */
      [[eosio::action]]
      void addchain(const name &chain_id, const bool &input, const bool &output,
                    const int64_t &in_swap_min_amount, const int64_t &out_swap_min_amount);

      /**
       * Init swap action.
       *
       * @details Initiate token swap from remchain to sender network.
       * Action initiated after transfer tokens to swap contract with valid data.
       *
       * @param from - the account to transfer from,
       * @param to - the account to be transferred to (remme swap contract),
       * @param quantity - the quantity of tokens to be transferred,
       * @param memo :
       *       @param return_chain_id - the chain identifier of the return address,
       *       @param return_address - the address that will receive swapped tokens back.
       */
      [[eosio::on_notify("rem.token::transfer")]]
      void ontransfer(name from, name to, asset quantity, string memo);

      using init_swap_action = action_wrapper<"init"_n, &swap::init>;
      using finish_swap_action = action_wrapper<"finish"_n, &swap::finish>;
      using finish_swap_and_create_acc_action = action_wrapper<"finishnewacc"_n, &swap::finishnewacc>;
      using cancel_swap_action = action_wrapper<"cancel"_n, &swap::cancel>;
      using set_swapparams_action = action_wrapper<"setswapparam"_n, &swap::setswapparam>;
      using add_chain_action = action_wrapper<"addchain"_n, &swap::addchain>;

   private:
      enum class swap_status : int8_t {
         CANCELED = -1,
         INITIALIZED = 0,
         ISSUED = 1,
         FINISHED = 2
      };

      static constexpr name system_account = "rem"_n;

      const time_point swap_lifetime = time_point(days(180));
      const time_point swap_active_lifetime = time_point(days(7));

      struct [[eosio::table]] swap_data {
         uint64_t          key;
         string            txid;
         checksum256       swap_id;
         block_timestamp   swap_timestamp;
         int8_t            status;

         vector<name>      provided_approvals;

         uint64_t primary_key() const { return key; }

         fixed_bytes<32> by_swap_id() const { return get_swap_hash(swap_id); }

         static fixed_bytes<32> get_swap_hash(const checksum256 &hash) {
            const uint128_t *p128 = reinterpret_cast<const uint128_t *>(&hash);
            fixed_bytes<32> k;
            k.data()[0] = p128[0];
            k.data()[1] = p128[1];
            return k;
         }

         // explicit serialization macro is not necessary, used here only to improve compilation time
         EOSLIB_SERIALIZE( swap_data, (key)(txid)(swap_id)(swap_timestamp)
                                      (status)(provided_approvals)
         )
      };

      struct [[eosio::table]] swapparams {
         string    chain_id = "0";
         string    eth_swap_contract_address = "0";
         string    eth_return_chainid = "0";

         // explicit serialization macro is not necessary, used here only to improve compilation time
         EOSLIB_SERIALIZE( swapparams, (chain_id)(eth_swap_contract_address)(eth_return_chainid) )
      };

      struct [[eosio::table]] chains {
         name     chain;
         bool     input;
         bool     output;
         int64_t  in_swap_min_amount  = 1000000; // minimum amount to swap tokens in remchain
         int64_t  out_swap_min_amount = 5000000; // minimum amount to swap tokens from remchain

         uint64_t primary_key() const { return chain.value; }

         // explicit serialization macro is not necessary, used here only to improve compilation time
         EOSLIB_SERIALIZE( chains, (chain)(input)(output)(in_swap_min_amount)(out_swap_min_amount) )
      };

      typedef multi_index<"swaps"_n, swap_data,
              indexed_by<"byhash"_n, const_mem_fun <swap_data, fixed_bytes<32>, &swap_data::by_swap_id>>
              > swap_index;
      swap_index swap_table;

      typedef singleton<"swapparams"_n, swapparams> swap_params;
      swap_params swap_params_table;
      swapparams swap_params_data;

      typedef multi_index<"chains"_n, chains> chains_index;
      chains_index chains_table;

      bool is_block_producer(const name &user) const;
      bool is_swap_confirmed(const vector <name> &provided_approvals) const;
      asset get_min_account_stake() const;
      vector<name> get_producers() const;
      asset get_producers_reward(const name &chain_id) const;

      checksum256 get_swap_id(const string &txid, const string &swap_pubkey_str, const asset &quantity,
                              const string &return_address, const string &return_chain_id,
                              const block_timestamp &swap_timestamp);

      checksum256 get_digest_msg(const name &receiver, const string &owner_key, const string &active_key,
                                 const string &txid, const asset &quantity, const string &return_address,
                                 const string &return_chain_id, const block_timestamp &swap_timestamp);

      void to_rewards(const asset &quantity);
      void retire_tokens(const asset &quantity, const string &memo);
      void transfer(const name &receiver, const asset &quantity, const string &memo);
      void issue_tokens(const name& rampayer, const asset &quantity);
      void create_user(const name &user, const public_key &owner_key,
                       const public_key &active_key, const asset &min_account_stake);

      void validate_swap(const checksum256 &swap_hash) const;
      void validate_address(const name &chain_id, const string &address);
      void validate_pubkey(const signature &sign, const checksum256 &digest, const string &swap_pubkey_str) const;
      void cleanup_swaps();

      void check_pubkey_prefix(const string &pubkey_str) const;
   };
   /** @}*/ // end of @defgroup remswap rem.swap
   inline auto get_base58_map(std::array<int8_t, 256> &base58_map, bool &map_initialized) {
      const char base58_chars[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
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
   inline std::array<uint8_t, size> base58_to_binary(std::string_view s) {
      bool map_initialized = false;
      std::array<int8_t, 256> base58_map{{0}};
      std::array<uint8_t, size> result{{0}};
      for (auto& src_digit : s) {
         int carry = get_base58_map(base58_map, map_initialized)[src_digit];
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

   inline public_key string_to_public_key(std::string_view s) {
      public_key key;
      bool is_k1_type = s.size() >= 3 && ( s.substr(0, 3) == "EOS" || s.substr(0, 3) == "REM");
      bool is_r1_type = s.size() >= 7 && s.substr(0, 7) == "PUB_R1_";

      check(is_k1_type || is_r1_type, "unrecognized public key format");
      auto whole = base58_to_binary<37>( is_k1_type ? s.substr(3) : s.substr(7) );
      check(whole.size() == std::get_if<0>(&key)->size() + 4, "invalid public key length");

      memcpy(std::get_if<0>(&key)->data(), whole.data(), std::get_if<0>(&key)->size());
      return key;
   }

   inline string join( vector<string>&& vec, string delim = "*" ) {
      return std::accumulate(std::next(vec.begin()), vec.end(), vec[0],
                             [&delim](string& a, string& b) {
                                return a + delim + b;
      });
   }
} /// namespace eosio
