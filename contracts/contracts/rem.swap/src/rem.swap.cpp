/**
 *  @copyright defined in eos/LICENSE.txt
 */

#include <rem.swap/rem.swap.hpp>
#include <rem.token/rem.token.hpp>
#include <rem.system/rem.system.hpp>

#include "base58.cpp"
#include "system_info.cpp"

namespace eosio {

   using eosiosystem::system_contract;

   swap::swap(name receiver, name code,  datastream<const char*> ds) : contract(receiver, code, ds),
   swap_table(_self, _self.value),
   swap_params_table(_self, _self.value),
   chains_table(_self, _self.value) {
      if (!swap_params_table.exists()) {
         swap_params_table.set(swapparams{}, system_account);
      }
      swap_params_data = swap_params_table.get();
   }

   void swap::init(const name &rampayer, const string &txid, const string &swap_pubkey,
                   const asset &quantity, const string &return_address, const string &return_chain_id,
                   const block_timestamp &swap_timestamp) {
      require_auth(rampayer);

      const asset min_account_stake = get_min_account_stake();
      const asset producers_reward(swap_params_data.in_swap_fee, system_contract::get_core_symbol() );

      check_pubkey_prefix(swap_pubkey);
      check(quantity.is_valid(), "invalid quantity");
      check(quantity.symbol == min_account_stake.symbol, "symbol precision mismatch");
      check(quantity.amount >= min_account_stake.amount + producers_reward.amount, "the quantity must be greater "
                                                                                   "than the swap fee");
      time_point swap_timepoint = swap_timestamp.to_time_point();

      string swap_payload = join({swap_pubkey.substr(3), txid, swap_params_data.chain_id, quantity.to_string(),
                                  return_address, return_chain_id, std::to_string(swap_timepoint.sec_since_epoch())});

      checksum256 swap_hash = sha256(swap_payload);

      auto swap_hash_idx = swap_table.get_index<"byhash"_n>();
      auto swap_hash_itr = swap_hash_idx.find(swap_data::get_swap_hash(swap_hash));

      auto swap_expiration_delta = current_time_point().time_since_epoch() - swap_lifetime.time_since_epoch();
      check(time_point(swap_expiration_delta) < swap_timepoint, "swap lifetime expired");
      check(current_time_point() + swap_active_lifetime > swap_timepoint, "swap cannot be initialized "
                                                                          "with a future timestamp");

      const bool is_producer = is_block_producer(rampayer);
      if (swap_hash_itr == swap_hash_idx.end()) {
         swap_table.emplace(rampayer, [&](auto &s) {
            s.key = swap_table.available_primary_key();
            s.txid = txid;
            s.swap_id = swap_hash;
            s.swap_timestamp = swap_timestamp;
            s.status = static_cast<int8_t>(swap_status::INITIALIZED);
            if (is_producer) s.provided_approvals.push_back(rampayer);
         });
      } else {
         check(is_producer, "block producer authorization required");
         check(swap_hash_itr->status != static_cast<int8_t>(swap_status::CANCELED), "swap already canceled");

         const vector<name> &approvals = swap_hash_itr->provided_approvals;
         bool is_already_approved = std::find(approvals.begin(), approvals.end(), rampayer) == approvals.end();

         check(is_already_approved, "approval already exists");

         swap_table.modify(*swap_hash_itr, rampayer, [&](auto &s) {
            s.provided_approvals.push_back(rampayer);
         });
      }

      if (is_producer) {
         cleanup_swaps();
         swap_hash_itr = swap_hash_idx.find(swap_data::get_swap_hash(swap_hash));
         bool is_status_issued = swap_hash_itr->status == static_cast<int8_t>(swap_status::ISSUED);
         if (is_swap_confirmed(swap_hash_itr->provided_approvals) && !is_status_issued) {
            issue_tokens(rampayer, quantity);
            swap_table.modify(*swap_hash_itr, rampayer, [&](auto &s) {
               s.status = static_cast<int8_t>(swap_status::ISSUED);
            });
         }
      }
   }

   void swap::finish(const name &rampayer, const name &receiver, const string &txid, const string &swap_pubkey_str,
                     asset &quantity, const string &return_address, const string &return_chain_id,
                     const block_timestamp &swap_timestamp, const signature &sign) {

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
      auto swap_hash_itr = swap_hash_idx.find(swap_data::get_swap_hash(swap_hash));

      const time_point swap_timepoint = swap_hash_itr->swap_timestamp.to_time_point();
      check(time_point_sec(current_time_point()) < swap_timepoint + swap_active_lifetime,
            "swap has to be canceled after expiration");

      const asset producers_reward(swap_params_data.in_swap_fee, system_contract::get_core_symbol());
      quantity.amount -= producers_reward.amount;
      to_rewards(producers_reward);
      transfer(receiver, quantity);

      swap_table.modify(*swap_hash_itr, rampayer, [&](auto &s) {
         s.status = static_cast<int8_t>(swap_status::FINISHED);
      });
   }

   void swap::finishnewacc(const name &rampayer, const name &receiver, const string &owner_pubkey_str,
                           const string &active_pubkey_str, const string &txid, const string &swap_pubkey_str,
                           asset &quantity, const string &return_address, const string &return_chain_id,
                           const block_timestamp &swap_timestamp, const signature &sign) {

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
      auto swap_hash_itr = swap_hash_idx.find(swap_data::get_swap_hash(swap_hash));

      const time_point swap_timepoint = swap_hash_itr->swap_timestamp.to_time_point();
      check(time_point_sec(current_time_point()) < swap_timepoint + swap_active_lifetime,
            "swap has to be canceled after expiration");

      public_key owner_key = string_to_public_key(owner_pubkey_str);
      public_key active_key = string_to_public_key(active_pubkey_str);
      const asset min_account_stake = get_min_account_stake();
      const asset producers_reward(swap_params_data.in_swap_fee, system_contract::get_core_symbol());

      quantity.amount -= (min_account_stake.amount + producers_reward.amount);

      to_rewards(producers_reward);
      create_user(receiver, owner_key, active_key, min_account_stake);

      if ( quantity.amount > 0 ) {
         transfer(receiver, quantity);
      }

      swap_table.modify(*swap_hash_itr, rampayer, [&](auto &s) {
         s.status = static_cast<int8_t>(swap_status::FINISHED);
      });
   }

   void swap::cancel(const name &rampayer, const string &txid, const string &swap_pubkey_str,
                     asset &quantity, const string &return_address, const string &return_chain_id,
                     const block_timestamp &swap_timestamp) {

      require_auth(rampayer);
      time_point swap_timepoint = swap_timestamp.to_time_point();

      const checksum256 swap_hash = get_swap_id(
         txid, swap_pubkey_str, quantity, return_address,
         return_chain_id, swap_timestamp
      );

      auto swap_hash_idx = swap_table.get_index<"byhash"_n>();
      auto swap_hash_itr = swap_hash_idx.find(swap_data::get_swap_hash(swap_hash));

      validate_swap(swap_hash);
      check(time_point_sec(current_time_point()) > swap_timepoint + swap_active_lifetime,
            "swap has to be canceled after expiration");

      const asset producers_reward(swap_params_data.in_swap_fee, system_contract::get_core_symbol());
      quantity.amount -= producers_reward.amount;

      string retire_memo = return_chain_id + ' ' + return_address;
      to_rewards(producers_reward);
      retire_tokens(quantity, retire_memo);
      require_recipient(_self);

      swap_table.modify(*swap_hash_itr, rampayer, [&](auto &s) {
         s.status = static_cast<int8_t>(swap_status::CANCELED);
      });
   }

   void swap::setswapfee(const int64_t &amount) {
      require_auth( _self );
      check(amount > 0, "amount must be a positive");

      swap_params_data.in_swap_fee = amount;
      swap_params_table.set(swap_params_data, same_payer);
   }

   void swap::setminswpout(const int64_t &amount) {
      require_auth( _self );
      check(amount > 0, "amount must be a positive");

      swap_params_data.out_swap_min_amount = amount;
      swap_params_table.set(swap_params_data, same_payer);
   }

   void swap::setchainid(const string &chain_id) {
      require_auth( _self );
      check(!chain_id.empty(), "invalid chain id");

      swap_params_data.chain_id = chain_id;
      swap_params_table.set(swap_params_data, same_payer);
   }

   void swap::addchain(const name &chain_id, const bool &input, const bool &output) {
      require_auth( _self );

      auto it = chains_table.find(chain_id.value);
      if (it == chains_table.end()) {
         chains_table.emplace( _self, [&]( auto& c ) {
            c.chain = chain_id;
            c.input = input;
            c.output = output;
         });
      } else {
         chains_table.modify(*it, _self, [&](auto &c) {
            c.input = input;
            c.output = output;
         });
      }
   }

   checksum256 swap::get_swap_id(const string &txid, const string &swap_pubkey_str, const asset &quantity,
                                 const string &return_address, const string &return_chain_id,
                                 const block_timestamp &swap_timestamp) {

      time_point swap_timepoint = swap_timestamp.to_time_point();

      string swap_payload = join({swap_pubkey_str.substr(3), txid, swap_params_data.chain_id, quantity.to_string(),
                                  return_address, return_chain_id, std::to_string(swap_timepoint.sec_since_epoch())});

      checksum256 swap_hash = sha256(swap_payload);


      return swap_hash;
   }

   checksum256 swap::get_digest_msg(const name &receiver, const string &owner_key, const string &active_key,
                                    const string &txid, const asset &quantity, const string &return_address,
                                    const string &return_chain_id, const block_timestamp &swap_timestamp) {

      time_point swap_timepoint = swap_timestamp.to_time_point();

      string payload = join({txid, swap_params_data.chain_id, quantity.to_string(), return_address,
                             return_chain_id, std::to_string(swap_timepoint.sec_since_epoch())});

      string sign_payload = (owner_key.size() == 0) ? join({receiver.to_string(), payload}) :
                            join({receiver.to_string(), owner_key, active_key, payload});

      checksum256 digest = sha256(sign_payload);
      return digest;
   }

   void swap::validate_pubkey(const signature &sign, const checksum256 &digest, const string &swap_pubkey_str) const {
      public_key swap_pubkey = string_to_public_key(swap_pubkey_str);
      assert_recover_key(digest, sign, swap_pubkey);
   }

   void swap::validate_swap(const checksum256 &swap_hash) const {

      auto swap_hash_idx = swap_table.get_index<"byhash"_n>();
      auto swap_hash_itr = swap_hash_idx.find(swap_data::get_swap_hash(swap_hash));

      check(swap_hash_itr != swap_hash_idx.end(), "swap doesn't exist");
      check(swap_hash_itr->status != static_cast<int8_t>(swap_status::CANCELED), "swap already canceled");
      check(swap_hash_itr->status != static_cast<int8_t>(swap_status::FINISHED), "swap already finished");

      time_point swap_timepoint = swap_hash_itr->swap_timestamp.to_time_point();
      auto swap_expiration_delta = current_time_point().time_since_epoch() - swap_lifetime.time_since_epoch();
      check(time_point(swap_expiration_delta) < swap_timepoint, "swap lifetime expired");

      check(is_swap_confirmed(swap_hash_itr->provided_approvals), "not enough active producers approvals");
   }

   void swap::validate_address(const name &chain_id, const string &address) {
      action(
         permission_level{_self, system_contract::active_permission},
         "rem.utils"_n, "validateaddr"_n,
         std::make_tuple(chain_id, address)
      ).send();
   }

   void swap::cleanup_swaps() {
      const uint8_t max_clear_depth = 10;
      size_t i = 0;
      for (auto _table_itr = swap_table.begin(); _table_itr != swap_table.end();) {
         time_point swap_timepoint = _table_itr->swap_timestamp.to_time_point();
         bool not_expired = time_point_sec(current_time_point()) <= swap_timepoint + swap_lifetime;

         if (not_expired || i >= max_clear_depth) {
            break;
         } else {
            _table_itr = swap_table.erase(_table_itr);
            ++i;
         }
      }
   }

   void swap::ontransfer(name from, name to, asset quantity, string memo) {
      if (to != _self || from == _self) {
         return;
      }
      auto space_pos = memo.find(' ');
      check((space_pos != string::npos), "invalid memo");

      string return_chain_id = memo.substr(0, space_pos);
      check(return_chain_id.size() > 0, "wrong chain id");

      string return_address = memo.substr(space_pos + 1);
      check(return_address.size() > 0, "wrong address");

      validate_address(name(return_chain_id), return_address);

      check(quantity.symbol == system_contract::get_core_symbol(), "symbol precision mismatch");
      check(quantity.amount >= swap_params_data.out_swap_min_amount, "the quantity must be greater than the swap minimum amount");

      string retire_memo = return_chain_id + ' ' + return_address;
      retire_tokens(quantity, retire_memo);
      require_recipient(_self);
   }

   void swap::transfer(const name &receiver, const asset &quantity) {
      token::transfer_action transfer(system_contract::token_account, {_self, system_contract::active_permission});
      transfer.send(_self, receiver, quantity, string("atomic-swap"));
   }

   void swap::create_user(const name &user, const public_key &owner_key,
                          const public_key &active_key, const asset &min_account_stake) {

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

      eosiosystem::system_contract::newaccount_action newaccount(system_account, {_self, system_contract::active_permission});
      eosiosystem::system_contract::delegatebw_action delegatebw(system_account, {_self, system_contract::active_permission});

      newaccount.send(_self, user, owner, active);
      delegatebw.send(_self, user, min_account_stake, true);
   }

   void swap::to_rewards(const asset &quantity) {
      eosiosystem::system_contract::torewards_action torewards(system_account, {_self, system_contract::active_permission});
      torewards.send(_self, quantity);
   }

   void swap::retire_tokens(const asset &quantity, const string &memo) {
      token::retire_action retire(system_contract::token_account, {_self, system_contract::active_permission});
      retire.send(quantity, memo);
   }

   void swap::issue_tokens(const name &rampayer, const asset &quantity) {
      token::issue_action issue(system_contract::token_account, {_self, system_contract::active_permission});
      issue.send(_self, quantity, string("swap issue tokens"));
   }
} /// namespace eosio
