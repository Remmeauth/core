/**
 *  @copyright defined in eos/LICENSE.txt
 */

#pragma once

#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>

#include <rem.attr/rem.attr.hpp>

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
   class [[eosio::contract("rem.auth")]] auth : public attribute {
   public:

      auth(name receiver, name code,  datastream<const char*> ds):attribute(receiver, code, ds),
      authkeys_tbl(get_self(), get_self().value){};

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
       * @details Add new authentication key by using correspond to the account authentication key.
       *
       * @param account - the owner account to execute the addkeyapp action for,
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
      void revokeacc(const name &account, const string &revoke_pub_key_str);

      /**
       * Revoke active authentication key action.
       *
       * @details Revoke already added active authentication key by using correspond to account authentication key.
       *
       * @param account - the owner account to execute the revokeapp action for,
       * @param revoke_pub_key_str - the public key to be revoked on the corresponding account,
       * @param pub_key_str - the public key which is tied to the corresponding account,
       * @param signed_by_pub_key - the signature that was signed by pub_key_str.
       */
      [[eosio::action]]
      void revokeapp(const name &account, const string &revoke_pub_key_str,
                     const string &pub_key_str, const signature &signed_by_pub_key);

      /**
       * Buy AUTH credits action.
       *
       * @details buy AUTH credit at a current REM-USD price indicated in the rem.orace contract.
       *
       * @param account - the account to transfer from,
       * @param quantity - the quantity of AUTH credits to be purchased,
       * @param max_price - the maximum price for one AUTH credit, that amount of REM tokens that can be debited
       * for one AUTH credit.
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
       * @param memo - the memo string to accompany the transaction,
       * @param pub_key_str - the public key which is tied to the corresponding account,
       * @param signed_by_pub_key - the signature that was signed by pub_key_str,
       */
      [[eosio::action]]
      void transfer(const name &from, const name &to, const asset &quantity, const string &memo,
                    const string &pub_key_str, const signature &signed_by_pub_key);

      /**
       * Cleanup authkeys table action.
       *
       * @details Delete expired keys (keys for which not_valid_after plus expiration_time has passed).
       */
      [[eosio::action]]
      void cleanupkeys();

      using addkeyacc_action = action_wrapper<"addkeyacc"_n, &auth::addkeyacc>;
      using addkeyapp_action = action_wrapper<"addkeyapp"_n, &auth::addkeyapp>;
      using revokeacc_action = action_wrapper<"revokeacc"_n, &auth::revokeacc>;
      using revokeapp_action = action_wrapper<"revokeapp"_n, &auth::revokeapp>;
      using buyauth_action   = action_wrapper<"buyauth"_n,     &auth::buyauth>;
      using transfer_action  = action_wrapper<"transfer"_n,   &auth::transfer>;
   private:
      static constexpr symbol auth_symbol{"AUTH", 4};
      static constexpr name system_account = "rem"_n;

      const asset key_storage_fee{1'0000, auth_symbol};
      const time_point key_lifetime = time_point(days(360));
      const time_point key_cleanup_time = time_point(days(180)); // the time that should be passed after not_valid_after to delete key

      struct [[eosio::table]] authkeys {
         uint64_t          key;
         name              owner;
         public_key        public_key;
         string            extra_public_key;
         block_timestamp   not_valid_before;
         block_timestamp   not_valid_after;
         uint32_t          revoked_at;

         uint64_t primary_key()const         { return key;         }
         uint64_t by_name()const             { return owner.value; }
         uint64_t by_not_valid_before()const { return not_valid_before.to_time_point().elapsed.count(); }
         uint64_t by_not_valid_after()const  { return not_valid_after.to_time_point().elapsed.count(); }
         uint64_t by_revoked()const          { return revoked_at;  }

         EOSLIB_SERIALIZE( authkeys, (key)(owner)(public_key)(extra_public_key)(not_valid_before)(not_valid_after)(revoked_at))
      };
      typedef multi_index<"authkeys"_n, authkeys,
            indexed_by<"byname"_n,       const_mem_fun < authkeys, uint64_t, &authkeys::by_name>>,
            indexed_by<"bynotvalbfr"_n,  const_mem_fun <authkeys, uint64_t, &authkeys::by_not_valid_before>>,
            indexed_by<"bynotvalaftr"_n, const_mem_fun <authkeys, uint64_t, &authkeys::by_not_valid_after>>,
            indexed_by<"byrevoked"_n,    const_mem_fun <authkeys, uint64_t, &authkeys::by_revoked>>
            > authkeys_idx;

      authkeys_idx authkeys_tbl;

      struct [[eosio::table]] account {
         asset    balance;

         uint64_t primary_key()const { return balance.symbol.code().raw(); }
      };
      typedef multi_index<"accounts"_n, account> accounts;

      struct [[eosio::table]] remprice {
         name                    pair;
         double                  price = 0;
         vector<double>          price_points;
         block_timestamp         last_update;

         uint64_t primary_key()const { return pair.value; }

         // explicit serialization macro is not necessary, used here only to improve compilation time
         EOSLIB_SERIALIZE( remprice, (pair)(price)(price_points)(last_update))
      };
      typedef multi_index< "remprice"_n, remprice> remprice_idx;

      void sub_storage_fee(const name &account, const asset &price_limit);
      void transfer_tokens(const name &from, const name &to, const asset &quantity, const string &memo);
      void to_rewards(const name& payer, const asset &quantity);

      auto find_active_appkey(const name &account, const public_key &key);
      void require_app_auth(const name &account, const public_key &key);

      asset get_balance(const name& token_contract_account, const name& owner, const symbol& sym);
      asset get_purchase_fee(const asset &quantity_auth);
      double get_account_discount(const name &account) const;

      void check_permission(const name& issuer, const name& receiver, int32_t ptype) const;
      bool need_confirm(int32_t ptype) const;
   };
   /** @}*/ // end of @defgroup eosioauth rem.auth
} /// namespace eosio
