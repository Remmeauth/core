/**
 *  @copyright defined in eos/LICENSE.txt
 */

#include <rem.auth/rem.auth.hpp>
#include <rem.system/rem.system.hpp>
#include <rem.swap/rem.swap.hpp>
#include <rem.oracle/rem.oracle.hpp>
#include <rem.token/rem.token.hpp>

namespace eosio {
   using eosiosystem::system_contract;

   void auth::addkeyacc(const name &account, const string &pub_key_str, const signature &signed_by_pub_key,
                        const string &extra_pub_key, const asset &price_limit, const string &payer_str)
   {
      name payer = payer_str.empty() ? account : name(payer_str);
      require_auth(account);
      require_auth(payer);

      public_key pub_key = string_to_public_key(pub_key_str);
      string payload = join( { account.to_string(), pub_key_str, extra_pub_key, payer_str } );
      checksum256 digest = sha256(payload.c_str(), payload.size());
      assert_recover_key(digest, signed_by_pub_key, pub_key);

      authkeys_tbl.emplace(get_self(), [&](auto &k) {
         k.key              = authkeys_tbl.available_primary_key();
         k.owner            = account;
         k.public_key       = pub_key;
         k.extra_public_key = extra_pub_key;
         k.not_valid_before = current_time_point();
         k.not_valid_after  = current_time_point() + key_lifetime;
         k.revoked_at       = 0; // if not revoked == 0
      });

      sub_storage_fee(payer, price_limit);
      cleanupkeys();
   }

   void auth::addkeyapp(const name &account, const string &new_pub_key_str, const signature &signed_by_new_pub_key,
                        const string &extra_pub_key, const string &pub_key_str, const signature &signed_by_pub_key,
                        const asset &price_limit, const string &payer_str)
   {
      bool is_payer = payer_str.empty();
      name payer = is_payer ? account : name(payer_str);
      if (!is_payer) { require_auth(payer); }

      string payload = join( { account.to_string(), new_pub_key_str, extra_pub_key, pub_key_str, payer_str } );
      checksum256 digest = sha256(payload.c_str(), payload.size());

      public_key new_pub_key = string_to_public_key(new_pub_key_str);
      public_key pub_key = string_to_public_key(pub_key_str);

      public_key expected_new_pub_key = recover_key(digest, signed_by_new_pub_key);
      public_key expected_pub_key = recover_key(digest, signed_by_pub_key);

      check(expected_new_pub_key == new_pub_key, "expected key different than recovered new application key");
      check(expected_pub_key == pub_key, "expected key different than recovered application key");
      require_app_auth(account, pub_key);

      authkeys_tbl.emplace(get_self(), [&](auto &k) {
         k.key              = authkeys_tbl.available_primary_key();
         k.owner            = account;
         k.public_key       = new_pub_key;
         k.extra_public_key = extra_pub_key;
         k.not_valid_before = current_time_point();
         k.not_valid_after  = current_time_point() + key_lifetime;
         k.revoked_at       = 0; // if not revoked == 0
      });

      sub_storage_fee(payer, price_limit);
      cleanupkeys();
   }

   auto auth::find_active_appkey(const name &account, const public_key &key)
   {
      auto authkeys_idx = authkeys_tbl.get_index<"byname"_n>();
      auto it = authkeys_idx.find(account.value);

      for(; it != authkeys_idx.end(); ++it) {
         auto ct = current_time_point();

         bool is_before_time_valid = ct > it->not_valid_before.to_time_point();
         bool is_after_time_valid = ct < it->not_valid_after.to_time_point();
         bool is_revoked = it->revoked_at;

         if (!is_before_time_valid || !is_after_time_valid || is_revoked) {
            continue;
         } else if (it->public_key == key) {
            break;
         }
      }
      return it;
   }

   void auth::revokeacc(const name &account, const string &revoke_pub_key_str)
   {
      require_auth(account);
      public_key revoke_pub_key = string_to_public_key(revoke_pub_key_str);
      require_app_auth(account, revoke_pub_key);

      auto it = find_active_appkey(account, revoke_pub_key);

      time_point ct = current_time_point();
      authkeys_tbl.modify(*it, get_self(), [&](auto &r) {
         r.revoked_at = ct.sec_since_epoch();
      });
   }

   void auth::revokeapp(const name &account, const string &revoke_pub_key_str,
                        const string &pub_key_str, const signature &signed_by_pub_key)
   {
      public_key revoke_pub_key = string_to_public_key(revoke_pub_key_str);
      public_key pub_key = string_to_public_key(pub_key_str);

      string payload = join( { account.to_string(), revoke_pub_key_str, pub_key_str } );
      checksum256 digest = sha256(payload.c_str(), payload.size());

      public_key expected_pub_key = recover_key(digest, signed_by_pub_key);
      check(expected_pub_key == pub_key, "expected key different than recovered application key");
      require_app_auth(account, revoke_pub_key);
      require_app_auth(account, pub_key);

      auto it = find_active_appkey(account, revoke_pub_key);

      time_point ct = current_time_point();
      authkeys_tbl.modify(*it, get_self(), [&](auto &r) {
         r.revoked_at = ct.sec_since_epoch();
      });
   }

   void auth::transfer(const name &from, const name &to, const asset &quantity, const string &memo,
                       const string &pub_key_str, const signature &signed_by_pub_key)
   {
      string payload = join( { from.to_string(), to.to_string(), quantity.to_string(), pub_key_str } );
      checksum256 digest = sha256(payload.c_str(), payload.size());

      public_key pub_key = string_to_public_key(pub_key_str);
      public_key expected_pub_key = recover_key(digest, signed_by_pub_key);

      check(expected_pub_key == pub_key, "expected key different than recovered application key");
      require_app_auth(from, pub_key);

      transfer_tokens(from, to, quantity, memo);
   }

   void auth::buyauth(const name &account, const asset &quantity, const double &max_price)
   {
      require_auth(account);
      check(quantity.is_valid(), "invalid quantity");
      check(quantity.amount > 0, "quantity should be a positive value");
      check(max_price > 0, "maximum price should be a positive value");
      check(quantity.symbol == auth_symbol, "symbol precision mismatch");

      remprice_idx remprice_table(system_contract::oracle_account, system_contract::oracle_account.value);
      auto remusd_it = remprice_table.find("rem.usd"_n.value);
      check(remusd_it != remprice_table.end(), "pair does not exist");

      double remusd_price = remusd_it->price;
      double account_discount = get_account_discount(account);
      check(max_price > remusd_price, "currently REM/USD price is above maximum price");

      asset purchase_fee = get_purchase_fee(quantity);
      purchase_fee.amount *= account_discount;

      token::issue_action issue(system_contract::token_account, { get_self(), system_contract::active_permission });

      transfer_tokens(account, get_self(), purchase_fee, "AUTH credits purchase fee");
      issue.send(get_self(), quantity, "buying an AUTH credits");
      transfer_tokens(get_self(), account, quantity, "buying an AUTH credits");
   }

   void auth::cleanupkeys() {
      const uint8_t max_clear_depth = 10;
      size_t i = 0;
      for (auto _table_itr = authkeys_tbl.begin(); _table_itr != authkeys_tbl.end();) {
         time_point not_valid_after = _table_itr->not_valid_after.to_time_point();
         bool not_expired = time_point_sec(current_time_point()) <= not_valid_after + key_cleanup_time;

         if (not_expired || i >= max_clear_depth) {
            break;
         } else {
            _table_itr = authkeys_tbl.erase(_table_itr);
            ++i;
         }
      }
   }

   void auth::sub_storage_fee(const name &account, const asset &price_limit)
   {
      bool is_pay_by_auth = (price_limit.symbol == auth_symbol);
      bool is_pay_by_rem  = (price_limit.symbol == system_contract::get_core_symbol());

      check(is_pay_by_rem || is_pay_by_auth, "unavailable payment method");
      check(price_limit.is_valid(), "invalid price limit");
      check(price_limit.amount > 0, "price limit should be a positive value");

      asset auth_credit_supply = token::get_supply(system_contract::token_account, auth_symbol.code());
      asset rem_balance = get_balance(system_contract::token_account, get_self(), system_contract::get_core_symbol());

      if (is_pay_by_rem) {
         double account_discount = get_account_discount(account);
         asset purchase_fee = get_purchase_fee(key_storage_fee);
         purchase_fee.amount *= account_discount;
         check(purchase_fee < price_limit, "currently REM/USD price is above price limit");

         transfer_tokens(account, get_self(), purchase_fee, "AUTH credits purchase fee");

         auth_credit_supply += key_storage_fee;
         rem_balance += purchase_fee;
      } else {
         check(auth_credit_supply.amount > 0, "overdrawn balance");
         transfer_tokens(account, get_self(), key_storage_fee, "AUTH credits purchase fee");

         token::retire_action retire(system_contract::token_account, { get_self(), system_contract::active_permission });
         retire.send(key_storage_fee, "the use of AUTH credit to store a key");
      }

      double reward_amount = rem_balance.amount / double(auth_credit_supply.amount);

      system_contract::torewards_action torewards(system_account, { get_self(), system_contract::active_permission });
      torewards.send(get_self(), asset{static_cast<int64_t>(reward_amount * key_storage_fee.amount), system_contract::get_core_symbol()});
   }

   double auth::get_account_discount(const name &account) const
   {
      attribute_info_table attributes_info(get_self(), get_self().value);
      if (attributes_info.begin() == attributes_info.end()) {
         return 1;
      }

      vector<char> data;
      for (auto it = attributes_info.begin(); it != attributes_info.end(); ++it) {
         attributes_table attributes(get_self(), it->attribute_name.value);
         auto idx = attributes.get_index<"reciss"_n>();
         auto attr_it = idx.find( attribute_data::combine_receiver_issuer(account, get_self()) );
         if (attr_it == idx.end() || !it->valid) {
            continue;
         }
         data = attr_it->attribute.data;
      }

      if (!data.empty()) {
         double account_discount = unpack<double>(data);
         check( account_discount >= 0 && account_discount <= 1, "attribute value error");

         return account_discount;
      }
      return 1;
   }

   void auth::require_app_auth(const name &account, const public_key &pub_key)
   {
      auto authkeys_idx = authkeys_tbl.get_index<"byname"_n>();
      auto it = authkeys_idx.find(account.value);

      check(it != authkeys_idx.end(), "account has no linked application keys");

      it = find_active_appkey(account, pub_key);
      check(it != authkeys_idx.end(), "account has no active application keys");
   }

   asset auth::get_balance(const name& token_contract_account, const name& owner, const symbol& sym)
   {
      accounts accountstable(token_contract_account, owner.value);
      const auto it = accountstable.find(sym.code().raw());
      return it == accountstable.end() ? asset{0, sym} : it->balance;
   }

   asset auth::get_purchase_fee(const asset &quantity_auth)
   {
      remprice_idx remprice_table(system_contract::oracle_account, system_contract::oracle_account.value);
      auto remusd_it = remprice_table.find("rem.usd"_n.value);
      check(remusd_it != remprice_table.end(), "pair does not exist");

      double remusd_price = remusd_it->price;
      int64_t purchase_fee = 1 / remusd_price;

      check(purchase_fee > 0, "invalid REM/USD price");
      return asset{ quantity_auth.amount * purchase_fee, system_contract::get_core_symbol() };
   }

   void auth::transfer_tokens(const name &from, const name &to, const asset &quantity, const string &memo)
   {
      token::transfer_action transfer(system_contract::token_account, {from, system_contract::active_permission});
      transfer.send(from, to, quantity, memo);
   }
} /// namespace eosio
