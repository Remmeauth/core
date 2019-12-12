/**
 *  @copyright defined in eos/LICENSE.txt
 */

#include <rem.swap/rem.swap.hpp>
#include <rem.token/rem.token.hpp>
#include <rem.utils/rem.utils.hpp>
#include <rem.system/rem.system.hpp>

namespace eosio {

   using eosiosystem::system_contract;

   swap::swap(name receiver, name code, datastream<const char*> ds) : contract(receiver, code, ds),
   swap_table(get_self(), get_self().value),
   swap_params_table(get_self(), get_self().value),
   chains_table(get_self(), get_self().value) {
      if (!swap_params_table.exists()) {
         swap_params_table.set(swapparams{}, system_account);
      }
      swap_params_data = swap_params_table.get();
   }

   void swap::init(const name &rampayer, const string &txid, const string &swap_pubkey,
                   const asset &quantity, const string &return_address, const string &return_chain_id,
                   const block_timestamp &swap_timestamp)
   {
      require_auth(rampayer);

      const asset min_account_stake = get_min_account_stake();
      const asset producers_reward = get_producers_reward(name(return_chain_id));

      check_pubkey_prefix(swap_pubkey);
      check(quantity.is_valid(), "invalid quantity");
      check(quantity.symbol == min_account_stake.symbol, "symbol precision mismatch");
      check(quantity.amount >= min_account_stake.amount + producers_reward.amount, "the quantity must be greater "
                                                                                   "than the swap fee");
      time_point swap_timepoint = swap_timestamp.to_time_point();

      string swap_payload = join({swap_pubkey.substr(3), txid, swap_params_data.chain_id, quantity.to_string(),
                                  return_address, return_chain_id, std::to_string(swap_timepoint.sec_since_epoch())});
      checksum256 swap_hash = sha256(swap_payload.c_str(), swap_payload.size());

      auto swap_hash_idx = swap_table.get_index<"byhash"_n>();
      auto swap_hash_it = swap_hash_idx.find(swap_data::get_swap_hash(swap_hash));

      auto swap_expiration_delta = current_time_point().time_since_epoch() - swap_lifetime.time_since_epoch();
      check(time_point(swap_expiration_delta) < swap_timepoint, "swap lifetime expired");
      check(current_time_point() > swap_timepoint, "swap cannot be initialized "
                                                   "with a future timestamp");

      const bool is_producer = is_block_producer(rampayer);
      if (swap_hash_it == swap_hash_idx.end()) {
         swap_table.emplace(rampayer, [&](auto &s) {
            s.key            = swap_table.available_primary_key();
            s.txid           = txid;
            s.swap_id        = swap_hash;
            s.swap_timestamp = swap_timestamp;
            s.status         = static_cast<int8_t>(swap_status::INITIALIZED);
            if (is_producer) s.provided_approvals.push_back(rampayer);
         });
      } else {

         if (is_producer) {
            check(is_producer, "block producer authorization required");
            check(swap_hash_it->status != static_cast<int8_t>(swap_status::CANCELED), "swap already canceled");

            const vector <name> &approvals = swap_hash_it->provided_approvals;
            bool is_already_approved = std::find(approvals.begin(), approvals.end(), rampayer) == approvals.end();

            check(is_already_approved, "approval already exists");

            swap_table.modify(*swap_hash_it, rampayer, [&](auto &s) {
               s.provided_approvals.push_back(rampayer);
            });
         }

      }
      // moved out, because existing case when the majority of the active producers = 1
      if (is_producer) {
         cleanup_swaps();
         swap_hash_it = swap_hash_idx.find(swap_data::get_swap_hash(swap_hash));
         bool is_status_issued = swap_hash_it->status == static_cast<int8_t>(swap_status::ISSUED);
         if (is_swap_confirmed(swap_hash_it->provided_approvals) && !is_status_issued) {
            issue_tokens(rampayer, quantity);
            swap_table.modify(*swap_hash_it, rampayer, [&](auto &s) {
               s.status = static_cast<int8_t>(swap_status::ISSUED);
            });
         }
      }
   }

   void swap::finish(const name &rampayer, const name &receiver, const string &txid, const string &swap_pubkey_str,
                     asset &quantity, const string &return_address, const string &return_chain_id,
                     const block_timestamp &swap_timestamp, const signature &sign)
   {
      require_auth(rampayer);

      const checksum256 swap_hash = get_swap_id(
         txid, swap_pubkey_str, quantity, return_address,
         return_chain_id, swap_timestamp
      );

      const checksum256 digest = get_digest_msg(
         receiver, {}, {}, txid, quantity,
         return_address, return_chain_id, swap_timestamp
      );

      validate_swap(swap_hash);
      validate_pubkey( sign, digest, swap_pubkey_str );

      auto swap_hash_idx = swap_table.get_index<"byhash"_n>();
      auto swap_hash_it = swap_hash_idx.find(swap_data::get_swap_hash(swap_hash));

      const time_point swap_timepoint = swap_hash_it->swap_timestamp.to_time_point();
      check(time_point_sec(current_time_point()) < swap_timepoint + swap_active_lifetime,
            "swap has to be canceled after expiration");

      const asset producers_reward = get_producers_reward(name(return_chain_id));
      quantity.amount -= producers_reward.amount;
      to_rewards(producers_reward);
      transfer(receiver, quantity, "Swap from `" + return_chain_id + "`");

      swap_table.modify(*swap_hash_it, rampayer, [&](auto &s) {
         s.status = static_cast<int8_t>(swap_status::FINISHED);
      });
   }

   void swap::finishnewacc(const name &rampayer, const name &receiver, const string &owner_pubkey_str,
                           const string &active_pubkey_str, const string &txid, const string &swap_pubkey_str,
                           asset &quantity, const string &return_address, const string &return_chain_id,
                           const block_timestamp &swap_timestamp, const signature &sign)
   {
      require_auth(rampayer);

      const checksum256 swap_hash = get_swap_id(
         txid, swap_pubkey_str, quantity, return_address,
         return_chain_id, swap_timestamp
      );

      const checksum256 digest = get_digest_msg(
         receiver, owner_pubkey_str, active_pubkey_str,
         txid, quantity, return_address,
         return_chain_id, swap_timestamp
      );

      validate_swap(swap_hash);
      validate_pubkey( sign, digest, swap_pubkey_str );

      auto swap_hash_idx = swap_table.get_index<"byhash"_n>();
      auto swap_hash_it = swap_hash_idx.find(swap_data::get_swap_hash(swap_hash));

      const time_point swap_timepoint = swap_hash_it->swap_timestamp.to_time_point();
      check(time_point_sec(current_time_point()) < swap_timepoint + swap_active_lifetime,
            "swap has to be canceled after expiration");

      public_key owner_key = string_to_public_key(owner_pubkey_str);
      public_key active_key = string_to_public_key(active_pubkey_str);
      const asset min_account_stake = get_min_account_stake();
      const asset producers_reward = get_producers_reward(name(return_chain_id));

      quantity.amount -= (min_account_stake.amount + producers_reward.amount);

      to_rewards(producers_reward);
      create_user(receiver, owner_key, active_key, min_account_stake);

      if ( quantity.amount > 0 ) {
         transfer(receiver, quantity, "Swap from `" + return_chain_id + "`");
      }

      swap_table.modify(*swap_hash_it, rampayer, [&](auto &s) {
         s.status = static_cast<int8_t>(swap_status::FINISHED);
      });
   }

   void swap::cancel(const name &rampayer, const string &txid, const string &swap_pubkey_str,
                     asset &quantity, const string &return_address, const string &return_chain_id,
                     const block_timestamp &swap_timestamp)
   {
      require_auth(rampayer);
      time_point swap_timepoint = swap_timestamp.to_time_point();

      const checksum256 swap_hash = get_swap_id(
         txid, swap_pubkey_str, quantity, return_address,
         return_chain_id, swap_timestamp
      );

      auto swap_hash_idx = swap_table.get_index<"byhash"_n>();
      auto swap_hash_it = swap_hash_idx.find(swap_data::get_swap_hash(swap_hash));

      validate_swap(swap_hash);
      check(time_point_sec(current_time_point()) > swap_timepoint + swap_active_lifetime,
            "swap has to be canceled after expiration");

      const asset producers_reward = get_producers_reward(name(return_chain_id));
      quantity.amount -= producers_reward.amount;

      string retire_memo = return_chain_id + ' ' + return_address;
      to_rewards(producers_reward);
      retire_tokens(quantity, retire_memo);
      require_recipient(get_self());

      swap_table.modify(*swap_hash_it, rampayer, [&](auto &s) {
         s.status = static_cast<int8_t>(swap_status::CANCELED);
      });
   }

   void swap::setswapparam(const string &chain_id, const string &eth_swap_contract_address, const string &eth_return_chainid)
   {
      require_auth( get_self() );
      check(!chain_id.empty(), "empty chain id");
      check(!eth_return_chainid.empty(), "empty ethereum return chain id");

      utils::validate_address_action validate_address(system_contract::utils_account, {get_self(), system_contract::active_permission});
      validate_address.send(name(eth_return_chainid), eth_swap_contract_address);

      swap_params_data.chain_id                   = chain_id;
      swap_params_data.eth_swap_contract_address  = eth_swap_contract_address;
      swap_params_data.eth_return_chainid         = eth_return_chainid;

      swap_params_table.set(swap_params_data, same_payer);
   }

   void swap::addchain(const name &chain_id, const bool &input, const bool &output,
                       const int64_t &in_swap_min_amount, const int64_t &out_swap_min_amount)
   {
      require_auth( get_self() );
      check(in_swap_min_amount > 0, "the minimum amount to swap tokens in remchain should be a positive");
      check(out_swap_min_amount > 0, "the minimum amount to swap tokens from remchain should be a positive");

      auto it = chains_table.find(chain_id.value);
      if (it == chains_table.end()) {
         chains_table.emplace( get_self(), [&]( auto& c ) {
            c.chain               = chain_id;
            c.input               = input;
            c.output              = output;
            c.in_swap_min_amount  = in_swap_min_amount;
            c.out_swap_min_amount = out_swap_min_amount;
         });
      } else {
         chains_table.modify(*it, get_self(), [&](auto &c) {
            c.input               = input;
            c.output              = output;
            c.in_swap_min_amount  = in_swap_min_amount;
            c.out_swap_min_amount = out_swap_min_amount;
         });
      }
   }

   checksum256 swap::get_swap_id(const string &txid, const string &swap_pubkey_str, const asset &quantity,
                                 const string &return_address, const string &return_chain_id,
                                 const block_timestamp &swap_timestamp)
   {
      time_point swap_timepoint = swap_timestamp.to_time_point();

      string swap_payload = join({swap_pubkey_str.substr(3), txid, swap_params_data.chain_id, quantity.to_string(),
                                  return_address, return_chain_id, std::to_string(swap_timepoint.sec_since_epoch())});

      checksum256 swap_hash = sha256(swap_payload.c_str(), swap_payload.size());
      return swap_hash;
   }

   checksum256 swap::get_digest_msg(const name &receiver, const string &owner_key, const string &active_key,
                                    const string &txid, const asset &quantity, const string &return_address,
                                    const string &return_chain_id, const block_timestamp &swap_timestamp)
   {
      time_point swap_timepoint = swap_timestamp.to_time_point();

      string payload = join({txid, swap_params_data.chain_id, quantity.to_string(), return_address,
                             return_chain_id, std::to_string(swap_timepoint.sec_since_epoch())});

      string sign_payload = (owner_key.size() == 0) ? join({receiver.to_string(), payload}) :
                            join({receiver.to_string(), owner_key, active_key, payload});

      checksum256 digest = sha256(sign_payload.c_str(), sign_payload.size());
      return digest;
   }

   void swap::validate_pubkey(const signature &sign, const checksum256 &digest, const string &swap_pubkey_str) const
   {
      public_key swap_pubkey = string_to_public_key(swap_pubkey_str);
      assert_recover_key(digest, sign, swap_pubkey);
   }

   void swap::validate_swap(const checksum256 &swap_hash) const
   {
      auto swap_hash_idx = swap_table.get_index<"byhash"_n>();
      auto swap_hash_it = swap_hash_idx.find(swap_data::get_swap_hash(swap_hash));

      check(swap_hash_it != swap_hash_idx.end(), "swap doesn't exist");
      check(swap_hash_it->status != static_cast<int8_t>(swap_status::CANCELED), "swap already canceled");
      check(swap_hash_it->status != static_cast<int8_t>(swap_status::FINISHED), "swap already finished");

      time_point swap_timepoint = swap_hash_it->swap_timestamp.to_time_point();
      auto swap_expiration_delta = current_time_point().time_since_epoch() - swap_lifetime.time_since_epoch();
      check(time_point(swap_expiration_delta) < swap_timepoint, "swap lifetime expired");

      check(is_swap_confirmed(swap_hash_it->provided_approvals), "not enough active producers approvals");
   }

   void swap::cleanup_swaps()
   {
      const uint8_t max_clear_depth = 10;
      size_t i = 0;
      for (auto _table_it = swap_table.begin(); _table_it != swap_table.end();) {
         time_point swap_timepoint = _table_it->swap_timestamp.to_time_point();
         bool not_expired = time_point_sec(current_time_point()) <= swap_timepoint + swap_lifetime;

         if (not_expired || i >= max_clear_depth) {
            break;
         } else {
            _table_it = swap_table.erase(_table_it);
            ++i;
         }
      }
   }

   void swap::ontransfer(name from, name to, asset quantity, string memo)
   {
      if (to != get_self() || from == get_self()) {
         return;
      }
      auto space_pos = memo.find(' ');
      check((space_pos != string::npos), "invalid memo");

      string return_chain_id = memo.substr(0, space_pos);
      check(return_chain_id.size() > 0, "invalid chain id");

      string return_address = memo.substr(space_pos + 1);
      check(return_address.size() > 0, "invalid address");

      utils::validate_address_action validate_address(system_contract::utils_account, {get_self(), system_contract::active_permission});
      validate_address.send(name(return_chain_id), return_address);

      auto chain_it = chains_table.find(name(return_chain_id).value);
      check(quantity.symbol == system_contract::get_core_symbol(), "symbol precision mismatch");
      check(chain_it != chains_table.end() && chain_it->output, "not supported chain id");
      check(quantity.amount >= chain_it->out_swap_min_amount, "the quantity must be greater than the swap minimum amount");

      string retire_memo = return_chain_id + ' ' + return_address;
      retire_tokens(quantity, retire_memo);
      require_recipient(get_self());
   }

   void swap::transfer(const name &receiver, const asset &quantity, const string &memo)
   {
      token::transfer_action transfer(system_contract::token_account, {get_self(), system_contract::active_permission});
      transfer.send(get_self(), receiver, quantity, memo);
   }

   void swap::create_user(const name &user, const public_key &owner_key,
                          const public_key &active_key, const asset &min_account_stake)
   {
      const eosiosystem::key_weight owner_pubkey_weight{
         .key = owner_key,
         .weight = 1,
      };
      const eosiosystem::key_weight active_pubkey_weight{
         .key = active_key,
         .weight = 1,
      };
      const eosiosystem::authority owner{
         .threshold = 1,
         .keys = {owner_pubkey_weight},
         .accounts = {},
         .waits = {}
      };
      const eosiosystem::authority active{
         .threshold = 1,
         .keys = {active_pubkey_weight},
         .accounts = {},
         .waits = {}
      };

      eosiosystem::system_contract::newaccount_action newaccount(system_account, {get_self(), system_contract::active_permission});
      eosiosystem::system_contract::delegatebw_action delegatebw(system_account, {get_self(), system_contract::active_permission});

      newaccount.send(get_self(), user, owner, active);
      delegatebw.send(get_self(), user, min_account_stake, true);
   }

   void swap::to_rewards(const asset &quantity)
   {
      eosiosystem::system_contract::torewards_action torewards(system_account, {get_self(), system_contract::active_permission});
      torewards.send(get_self(), quantity);
   }

   void swap::retire_tokens(const asset &quantity, const string &memo)
   {
      token::retire_action retire(system_contract::token_account, {get_self(), system_contract::active_permission});
      retire.send(quantity, memo);
   }

   void swap::issue_tokens(const name &rampayer, const asset &quantity)
   {
      token::issue_action issue(system_contract::token_account, {get_self(), system_contract::active_permission});
      issue.send(get_self(), quantity, "swap issue tokens");
   }
} /// namespace eosio
