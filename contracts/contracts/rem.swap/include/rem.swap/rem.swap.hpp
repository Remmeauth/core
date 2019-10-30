/**
 *  @copyright defined in eos/LICENSE.txt
 */

#pragma once

#include <eosio/action.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/singleton.hpp>
#include <eosio/eosio.hpp>
#include <eosio/permission.hpp>
#include <eosio/privileged.hpp>
#include <eosio/time.hpp>

#include <vector>
#include <numeric>
#include <string>

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
       * Init swap action.
       *
       * @details Initiate token swap in remchain.
       *
       * @param rampayer - the owner account to execute the init action for,
       * @param txid - the transfer transaction id in blockchain,
       * @param swap_pubkey - the public key created for the token swap,
       * @param quantity - the quantity of tokens in the transfer transaction,
       * @param return_address - the address that will receive swap tokens back,
       * @param return_chain_id - the chain id of the return address,
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
       * @param txid - the transfer transaction id in blockchain,
       * @param swap_pubkey - the public key created for the token swap,
       * @param quantity - the quantity of tokens in the transfer transaction,
       * @param return_address - the address that will receive swap tokens back,
       * @param return_chain_id - the chain id of the return address,
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
       * @param txid - the transfer transaction id in blockchain,
       * @param swap_pubkey - the public key created for the token swap,
       * @param quantity - the quantity of tokens in the transfer transaction,
       * @param return_address - the address that will receive swap tokens back,
       * @param return_chain_id - the chain id of the return address,
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
       * @param txid - the transfer transaction id in blockchain,
       * @param swap_pubkey - the public key created for the token swap,
       * @param quantity - the quantity of tokens in the transfer transaction,
       * @param return_address - the address that will receive swap tokens back,
       * @param return_chain_id - the chain id of the return address,
       * @param swap_timestamp - the timestamp transfer transaction in blockchain,
       * @param sign - the signature of payload that signed by swap_pubkey.
       */
      [[eosio::action]]
      void finishnewacc(const name &rampayer, const name &receiver, const string &owner_pubkey_str,
                        const string &active_pubkey_str, const string &txid, const string &swap_pubkey_str,
                        asset &quantity, const string &return_address, const string &return_chain_id,
                        const block_timestamp &swap_timestamp, const signature &sign);

      /**
       * Set swap fee action.
       *
       * @details Change amount of the swap fee, action permitted only for producers.
       *
       * @param quantity - the quantity of tokens to be deducted.
       */
      [[eosio::action]]
      void setswapfee(const int64_t &amount);

      /**
       * Set minimum amount to out swap action.
       *
       * @details Change minimum amount to out swap, action permitted only for producers.
       *
       * @param quantity - the quantity of tokens to be deducted.
       */
      [[eosio::action]]
      void setminswpout(const int64_t &amount);

      /**
       * Set remchain-id action.
       *
       * @details Change remchain-id, action permitted only for producers.
       *
       * @param chain_id - the identifier to be changed to.
       */
      [[eosio::action]]
      void setchainid(const string &chain_id);

      /**
       * Add supported chain identifier.
       *
       * @details Add supported chain identifier, action permitted only for producers.
       *
       * @param chain_id - the chain identifier to be added,
       * @param input - is supported input way to swap tokens according to chain identifier,
       * @param output - is supported output way to swap tokens according to chain identifier.
       */
      [[eosio::action]]
      void addchain(const name &chain_id, const bool &input, const bool &output);

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
       *       @param return_chain_id - the chain id of the return address,
       *       @param return_address - the address that will receive swapped tokens back.
       */
      [[eosio::on_notify("rem.token::transfer")]]
      void ontransfer(name from, name to, asset quantity, string memo);

      using init_swap_action = action_wrapper<"init"_n, &swap::init>;
      using finish_swap_action = action_wrapper<"finish"_n, &swap::finish>;
      using finish_swap_and_create_acc_action = action_wrapper<"finishnewacc"_n, &swap::finishnewacc>;
      using cancel_swap_action = action_wrapper<"cancel"_n, &swap::cancel>;
      using set_swapfee_action = action_wrapper<"setswapfee"_n, &swap::setswapfee>;
      using set_minswapout_action = action_wrapper<"setminswpout"_n, &swap::setminswpout>;
      using set_chainid_action = action_wrapper<"setchainid"_n, &swap::setchainid>;
      using add_chain_action = action_wrapper<"addchain"_n, &swap::addchain>;

   private:
      enum class swap_status : int8_t {
         CANCELED = -1,
         INITIALIZED = 0,
         ISSUED = 1,
         FINISHED = 2
      };

      static constexpr name system_account = "rem"_n;

      const time_point swap_lifetime = time_point(seconds(15552000)); // 180 days
      const time_point swap_active_lifetime = time_point(seconds(604800)); // 7 days

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
         // fee for swap in remchain
         int64_t   in_swap_fee = 1000;
         // minimum amount to swap tokens from remchain
         int64_t   out_swap_min_amount = 5000000;
         string    chain_id = "0";

         // explicit serialization macro is not necessary, used here only to improve compilation time
         EOSLIB_SERIALIZE( swapparams, (in_swap_fee)(out_swap_min_amount)(chain_id) )
      };

      struct [[eosio::table]] chains {
         name chain;
         bool input;
         bool output;

         uint64_t primary_key() const { return chain.value; }

         // explicit serialization macro is not necessary, used here only to improve compilation time
         EOSLIB_SERIALIZE( chains, (chain)(input)(output))
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
      static asset get_min_account_stake();
      asset get_swapbot_fee(const name &chain_id) const;

      checksum256 get_swap_id(const string &txid, const string &swap_pubkey_str, const asset &quantity,
                              const string &return_address, const string &return_chain_id,
                              const block_timestamp &swap_timestamp);

      checksum256 get_digest_msg(const name &receiver, const string &owner_key, const string &active_key,
                                 const string &txid, const asset &quantity, const string &return_address,
                                 const string &return_chain_id, const block_timestamp &swap_timestamp);

      checksum256 sha256(const string &str) const {
         return eosio::sha256(str.c_str(), str.size());
      }

      void to_rewards(const asset &quantity);
      void retire_tokens(const asset &quantity, const string &memo);
      void transfer(const name &receiver, const asset &quantity);
      void issue_tokens(const name& rampayer, const asset &quantity);
      void create_user(const name &user, const public_key &owner_key,
                       const public_key &active_key, const asset &min_account_stake);

      void validate_swap(const checksum256 &swap_hash) const;
      void validate_address(const name &chain_id, const string &address);
      void validate_pubkey(const signature &sign, const checksum256 &digest, const string &swap_pubkey_str) const;
      void cleanup_swaps();

      string join(vector <string> &&vec, string delim = string("*")) const;
      void check_pubkey_prefix(const string &pubkey_str) const;
   };
   /** @}*/ // end of @defgroup remswap rem.swap
} /// namespace eosio
