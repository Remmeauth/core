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
      checksum256 digest = sha256(join( { account.to_string(), pub_key_str, extra_pub_key, payer_str } ));
      eosio::assert_recover_key(digest, signed_by_pub_key, pub_key);

      authkeys_idx authkeys_tbl(get_self(), account.value);
      authkeys_tbl.emplace(get_self(), [&](auto &k) {
         k.key = authkeys_tbl.available_primary_key();
         k.owner = account;
         k.public_key = pub_key;
         k.extra_public_key = extra_pub_key;
         k.not_valid_before = current_time_point();
         k.not_valid_after = current_time_point() + key_lifetime;
         k.revoked_at = 0; // if not revoked == 0
      });

      sub_storage_fee(payer, price_limit);
   }

   void auth::addkeyapp(const name &account, const string &new_pub_key_str, const signature &signed_by_new_pub_key,
                        const string &extra_pub_key, const string &pub_key_str, const signature &signed_by_pub_key,
                        const asset &price_limit, const string &payer_str)
   {
      bool is_payer = payer_str.empty();
      name payer = is_payer ? account : name(payer_str);
      if (!is_payer) { require_auth(payer); }

      checksum256 digest = sha256(join( { account.to_string(), new_pub_key_str, extra_pub_key, pub_key_str, payer_str } ));

      public_key new_pub_key = string_to_public_key(new_pub_key_str);
      public_key pub_key = string_to_public_key(pub_key_str);

      check(assert_recover_key(digest, signed_by_new_pub_key, new_pub_key), "expected key different than recovered new key");
      check(assert_recover_key(digest, signed_by_pub_key, pub_key), "expected key different than recovered account key");
      require_app_auth(account, pub_key);

      authkeys_idx authkeys_tbl(get_self(), account.value);
      authkeys_tbl.emplace(get_self(), [&](auto &k) {
         k.key = authkeys_tbl.available_primary_key();
         k.owner = account;
         k.public_key = new_pub_key;
         k.extra_public_key = extra_pub_key;
         k.not_valid_before = current_time_point();
         k.not_valid_after = current_time_point() + key_lifetime;
         k.revoked_at = 0; // if not revoked == 0
      });

      sub_storage_fee(payer, price_limit);
   }

   auto auth::get_authkey_it(const authkeys_idx &authkeys_tbl, const name &account, const public_key &pub_key)
   {
      auto it = authkeys_tbl.begin();

      for(; it != authkeys_tbl.end(); ++it) {
         auto ct = current_time_point();

         bool is_before_time_valid = ct > it->not_valid_before.to_time_point();
         bool is_after_time_valid = ct < it->not_valid_after.to_time_point();
         bool is_revoked = it->revoked_at;

         if (!is_before_time_valid || !is_after_time_valid || is_revoked) {
            continue;
         } else if (it->public_key == pub_key) {
            break;
         }
      }
      return it;
   }

   void auth::revokeacc(const name &account, const string &pub_key_str)
   {
      require_auth(account);
      public_key pub_key = string_to_public_key(pub_key_str);
      require_app_auth(account, pub_key);

      authkeys_idx authkeys_tbl(get_self(), account.value);
      auto it = get_authkey_it(authkeys_tbl, account, pub_key);

      time_point ct = current_time_point();
      authkeys_tbl.modify(*it, get_self(), [&](auto &r) {
         r.revoked_at = ct.sec_since_epoch();
      });
   }

   void auth::revokeapp(const name &account, const string &revocation_pub_key_str,
                        const string &pub_key_str, const signature &signed_by_pub_key)
   {
      public_key revocation_pub_key = string_to_public_key(revocation_pub_key_str);
      public_key pub_key = string_to_public_key(pub_key_str);

      checksum256 digest = sha256(join( { account.to_string(), revocation_pub_key_str, pub_key_str } ));

      public_key expected_pub_key = recover_key(digest, signed_by_pub_key);
      check(expected_pub_key == pub_key, "expected key different than recovered account key");
      require_app_auth(account, revocation_pub_key);
      require_app_auth(account, pub_key);

      authkeys_idx authkeys_tbl(get_self(), account.value);
      auto it = get_authkey_it(authkeys_tbl, account, revocation_pub_key);

      time_point ct = current_time_point();
      authkeys_tbl.modify(*it, get_self(), [&](auto &r) {
         r.revoked_at = ct.sec_since_epoch();
      });
   }

   void auth::transfer(const name &from, const name &to, const asset &quantity,
                       const string &pub_key_str, const signature &signed_by_pub_key)
   {
      checksum256 digest = sha256(join( { from.to_string(), to.to_string(), quantity.to_string(), pub_key_str } ));

      public_key pub_key = string_to_public_key(pub_key_str);
      require_app_auth(from, pub_key);
      check(assert_recover_key(digest, signed_by_pub_key, pub_key), "expected key different than recovered account key");

      transfer_tokens(from, to, quantity, string("authentication app transfer"));
   }

   void auth::buyauth(const name &account, const asset &quantity, const double &max_price)
   {
      require_auth(account);
      check(quantity.is_valid(), "invalid quantity");
      check(quantity.amount > 0, "quantity should be a positive value");
      check(max_price > 0, "maximum price should be a positive value");
      check(quantity.symbol == auth_symbol, "symbol precision mismatch");

      double remusd = get_market_price("rem.usd"_n);

      check(max_price > remusd, "currently rem-usd price is above maximum price");
      asset purchase_fee = get_authrem_price(quantity);

      transfer_tokens(account, get_self(), purchase_fee, "purchase fee AUTH credits");
      issue_tokens(quantity);
      transfer_tokens(get_self(), account, quantity, "buying an AUTH credits");
   }

   void auth::sub_storage_fee(const name &account, const asset &price_limit)
   {
      bool is_pay_by_auth = price_limit.symbol == auth_symbol;
      bool is_pay_by_rem = price_limit.symbol == system_contract::get_core_symbol();
      check(is_pay_by_rem || is_pay_by_auth, "unavailable payment method");
      check(price_limit.is_valid(), "invalid price limit");
      check(price_limit.amount > 0, "price limit should be a positive value");

      asset account_auth_balance = get_balance(system_contract::token_account, account, auth_symbol);
      asset authrem_price{0, auth_symbol};
      asset buyauth_unit_price{0, auth_symbol};

      asset auth_credit_supply = token::get_supply(system_contract::token_account, auth_symbol.code());
      asset rem_balance = get_balance(system_contract::token_account, get_self(), system_contract::get_core_symbol());

      if (is_pay_by_rem) {
         authrem_price = get_authrem_price(key_store_price);
         check(authrem_price < price_limit, "currently rem-usd price is above price limit");
         buyauth_unit_price = key_store_price;
         transfer_tokens(account, get_self(), authrem_price, "purchase fee REM tokens");
      } else {
         check(auth_credit_supply.amount > 0, "overdrawn balance");
         transfer_tokens(account, get_self(), key_store_price, "purchase fee AUTH credits");
         retire_tokens(key_store_price);
      }

      double reward_amount = (rem_balance.amount + authrem_price.amount) /
         double(auth_credit_supply.amount + buyauth_unit_price.amount);

      to_rewards(get_self(), asset{static_cast<int64_t>(reward_amount * 10000), system_contract::get_core_symbol()});
   }

   void auth::require_app_auth(const name &account, const public_key &pub_key)
   {
      authkeys_idx authkeys_tbl(get_self(), account.value);
      auto it = authkeys_tbl.begin();

      check(it != authkeys_tbl.end(), "account has no linked app keys");

      it = get_authkey_it(authkeys_tbl, account, pub_key);
      check(it != authkeys_tbl.end(), "account has no active app keys");
   }

   asset auth::get_balance(const name& token_contract_account, const name& owner, const symbol& sym)
   {
      accounts accountstable(token_contract_account, owner.value);
      const auto it = accountstable.find(sym.code().raw());
      return it == accountstable.end() ? asset{0, sym} : it->balance;
   }

   double auth::get_market_price(const name &pair) const {
      remprice_idx remprice_table(oracle_contract, oracle_contract.value);
      auto it = remprice_table.find(pair.value);
      check(it != remprice_table.end(), "pair does not exist");
      return it->price;
   }

   asset auth::get_authrem_price(const asset &quantity)
   {
      double remusd = get_market_price("rem.usd"_n);
      double rem_amount = quantity.amount / remusd;
      return asset{static_cast<int64_t>(rem_amount), system_contract::get_core_symbol()};
   }

   void auth::issue_tokens(const asset &quantity)
   {
      token::issue_action issue(system_contract::token_account, {get_self(), system_contract::active_permission});
      issue.send(get_self(), quantity, string("buy AUTH credits"));
   }

   void auth::retire_tokens(const asset &quantity)
   {
      token::retire_action retire(system_contract::token_account, {get_self(), system_contract::active_permission});
      retire.send(quantity, string("store auth key"));
   }

   void auth::transfer_tokens(const name &from, const name &to, const asset &quantity, const string &memo)
   {
      token::transfer_action transfer(system_contract::token_account, {from, system_contract::active_permission});
      transfer.send(from, to, quantity, memo);
   }

   void auth::to_rewards(const name &payer, const asset &quantity)
   {
      system_contract::torewards_action torewards(system_account, {payer, system_contract::active_permission});
      torewards.send(payer, quantity);
   }

   bool auth::assert_recover_key(const checksum256 &digest, const signature &sign, const public_key &pub_key)
   {
      public_key expected_pub_key = recover_key(digest, sign);
      return expected_pub_key == pub_key;
   }
} /// namespace eosio

EOSIO_DISPATCH( eosio::auth, (addkeyacc)(addkeyapp)(revokeacc)(revokeapp)(buyauth)(transfer) )
