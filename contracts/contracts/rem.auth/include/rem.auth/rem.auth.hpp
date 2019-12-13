/**
 *  @copyright defined in eos/LICENSE.txt
 */

#pragma once

#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>

namespace eosio {

   using std::string;
   using std::vector;

   /**
    * @defgroup eosioauth rem.auth
    * @ingroup eosiocontracts
    *
    * rem.auth contract
    *
    * @details rem.auth contract defines the structures and actions that allow users and contracts to add, store, revoke
    * public keys.
    * @{
    */
   class [[eosio::contract("rem.auth")]] auth : public contract {
   public:
      using contract::contract;

      /**
       * Add new authentication key action.
       *
       * @details Add new authentication key by user account.
       *
       * @param account - the owner account to execute the addkeyacc action for,
       * @param pub_key_str - the public key that signed the payload,
       * @param signed_by_pub_key - the signature that was signed by pub_key_str,
       * @param price_limit - the maximum price which will be charged for storing the key can be in REM and AUTH,
       * @param extra_pub_key - the public key for authorization in external services,
       * @param payer_str - the account from which resources are debited.
       */
      [[eosio::action]]
      void addkeyacc(const name &account, const string &pub_key_str, const signature &signed_by_pub_key,
                     const string &extra_pub_key, const asset &price_limit, const string &payer_str);

      /**
       * Add new authentication key action.
       *
       * @details Add new authentication key by using correspond to account authentication key.
       *
       * @param account - the owner account to execute the addkeyacc action for,
       * @param new_pub_key_str - the public key that will be added,
       * @param signed_by_new_pub_key - the signature that was signed by new_pub_key_str,
       * @param extra_pub_key - the public key for authorization in external services,
       * @param pub_key_str - the public key which is tied to the corresponding account,
       * @param sign_by_key - the signature that was signed by pub_key_str,
       * @param price_limit - the maximum price which will be charged for storing the key can be in REM and AUTH,
       * @param payer_str - the account from which resources are debited.
       */
      [[eosio::action]]
      void addkeyapp(const name &account, const string &new_pub_key_str, const signature &signed_by_new_pub_key,
                     const string &extra_pub_key, const string &pub_key_str, const signature &signed_by_pub_key,
                     const asset &price_limit, const string &payer_str);

      /**
       * Revoke active authentication key action.
       *
       * @details Revoke already added active authentication key by user account.
       *
       * @param account - the owner account to execute the revokeacc action for,
       * @param pub_key_str - the public key to be revoked on the corresponding account.
       */
      [[eosio::action]]
      void revokeacc(const name &account, const string &pub_key_str);

      /**
       * Revoke active authentication key action.
       *
       * @details Revoke already added active authentication key by using correspond to account authentication key.
       *
       * @param account - the owner account to execute the revokeacc action for,
       * @param revoke_pub_key_str - the public key to be revoked on the corresponding account,
       * @param pub_key_str - the public key which is tied to the corresponding account,
       * @param signed_by_pub_key - the signature that was signed by pub_key_str.
       */
      [[eosio::action]]
      void revokeapp(const name &account, const string &revocation_pub_key_str,
                     const string &pub_key_str, const signature &signed_by_pub_key);

      /**
       * Buy AUTH credits action.
       *
       * @details buy AUTH credit at a current REM-USD price in rem.orace.
       *
       * @param account - the account to transfer from,
       * @param quantity - the quantity of AUTH credits to be purchased,
       * @param max_price - the maximum price for one AUTH credit,
       * Amount of REM tokens that can be debited for one AUTH credit.
       */
      [[eosio::action]]
      void buyauth(const name &account, const asset &quantity, const double &max_price);

      /**
       * Transfer action.
       *
       * @details Allows `from` account to transfer to `to` account the `quantity` tokens.
       * One account is debited and the other is credited with quantity tokens.
       *
       * @param from - the account to transfer from,
       * @param to - the account to be transferred to,
       * @param quantity - the quantity of tokens to be transferred,
       * @param pub_key_str - the public key which is tied to the corresponding account,
       * @param signed_by_pub_key - the signature that was signed by pub_key_str,
       */
      [[eosio::action]]
      void transfer(const name &from, const name &to, const asset &quantity,
                    const string &pub_key_str, const signature &signed_by_pub_key);

      using addkeyacc_action = action_wrapper<"addkeyacc"_n, &auth::addkeyacc>;
      using addkeyapp_action = action_wrapper<"addkeyapp"_n, &auth::addkeyapp>;
      using revokeacc_action = action_wrapper<"revokeacc"_n, &auth::revokeacc>;
      using revokeapp_action = action_wrapper<"revokeapp"_n, &auth::revokeapp>;
      using buyauth_action = action_wrapper<"buyauth"_n, &auth::buyauth>;
      using transfer_action = action_wrapper<"transfer"_n, &auth::transfer>;
   private:
      static constexpr symbol auth_symbol{"AUTH", 4};
      static constexpr name system_account = "rem"_n;
      static constexpr name oracle_contract = "rem.oracle"_n;

      const asset key_store_price{10000, auth_symbol};
      const time_point key_lifetime = time_point(seconds(31104000)); // 360 days

      struct [[eosio::table]] authkeys {
         uint64_t          key;
         name              owner;
         public_key        public_key;
         string            extra_public_key;
         block_timestamp   not_valid_before;
         block_timestamp   not_valid_after;
         uint32_t          revoked_at;

         uint64_t primary_key() const { return key; }
         uint64_t by_not_valid_before() const { return not_valid_before.to_time_point().elapsed.count(); }
         uint64_t by_not_valid_after() const { return not_valid_after.to_time_point().elapsed.count(); }
         uint64_t by_revoked() const { return revoked_at; }

         EOSLIB_SERIALIZE( authkeys, (key)(owner)(public_key)(extra_public_key)(not_valid_before)(not_valid_after)(revoked_at))
      };

      struct [[eosio::table]] account {
         asset    balance;

         uint64_t primary_key()const { return balance.symbol.code().raw(); }
      };

      typedef multi_index<"accounts"_n, account> accounts;
      typedef multi_index<"authkeys"_n, authkeys,
            indexed_by<"bynotvalbfr"_n, const_mem_fun <authkeys, uint64_t, &authkeys::by_not_valid_before>>,
            indexed_by<"bynotvalaftr"_n, const_mem_fun <authkeys, uint64_t, &authkeys::by_not_valid_after>>,
            indexed_by<"byrevoked"_n, const_mem_fun <authkeys, uint64_t, &authkeys::by_revoked>>
            > authkeys_idx;

      void sub_storage_fee(const name &account, const asset &price_limit);

      void issue_tokens(const asset &quantity);
      void retire_tokens(const asset &quantity);
      void transfer_tokens(const name &from, const name &to, const asset &quantity, const string &memo);
      void to_rewards(const name& payer, const asset &quantity);

      auto get_authkey_it(const authkeys_idx &authkeys_tbl, const name &account, const public_key &key);
      asset get_balance( const name& token_contract_account, const name& owner, const symbol& sym );
      asset get_authrem_price(const asset &quantity);
      double get_market_price(const name &pair) const;

      void require_app_auth(const name &account, const public_key &key);
      bool assert_recover_key(const checksum256 &digest, const signature &sign, const public_key &key);

      checksum256 sha256(const string &str) {
         return eosio::sha256(str.c_str(), str.size());
      }
   };
   /** @}*/ // end of @defgroup eosioauth rem.auth
} /// namespace eosio
