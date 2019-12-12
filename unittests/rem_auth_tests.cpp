/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/testing/tester.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include <fc/crypto/pke.hpp>

#include <boost/test/unit_test.hpp>

#include <contracts.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif


using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

using mvo = mutable_variant_object;

const char *AUTH_SYMBOL_NAME = "AUTH";
symbol AUTH_SYMBOL(4, AUTH_SYMBOL_NAME);

eosio::chain::asset auth_from_string(const std::string& s) {
   return asset::from_string(s + " " + AUTH_SYMBOL_NAME);
}

class rem_auth_tester : public TESTER {
public:
   rem_auth_tester();

   void deploy_contract( bool call_init = true ) {
      set_code( config::system_account_name, contracts::rem_system_wasm() );
      set_abi( config::system_account_name, contracts::rem_system_abi().data() );
      if( call_init ) {
         base_tester::push_action(config::system_account_name, N(init),
                                  config::system_account_name,  mutable_variant_object()
                                     ("version", 0)
                                     ("core", CORE_SYM_STR)
         );
      }
   }

   auto delegate_bandwidth( name from, name receiver, asset stake_quantity, uint8_t transfer = 1) {
      auto r = base_tester::push_action(config::system_account_name, N(delegatebw), from, mvo()
         ("from", from )
         ("receiver", receiver)
         ("stake_quantity", stake_quantity)
         ("transfer", transfer)
      );
      produce_block();
      return r;
   }

   void create_currency( name contract, name manager, asset maxsupply, const private_key_type* signer = nullptr ) {
      auto act =  mutable_variant_object()
         ("issuer",       manager )
         ("maximum_supply", maxsupply );

      base_tester::push_action(contract, N(create), contract, act );
   }

   auto issue( name contract, name manager, name to, asset amount ) {
      auto r = base_tester::push_action( contract, N(issue), manager, mutable_variant_object()
         ("to",      to )
         ("quantity", amount )
         ("memo", "")
      );
      produce_block();
      return r;
   }

   auto set_privileged( name account ) {
      auto r = base_tester::push_action(config::system_account_name, N(setpriv), config::system_account_name,  mvo()("account", account)("is_priv", 1));
      produce_block();
      return r;
   }

   auto transfer(const name& from, const name& to, const asset& quantity, const string& memo) {
      auto r = base_tester::push_action(N(rem.token), N(transfer), from, mvo()
         ("from", from)
         ("to", to)
         ("quantity", quantity)
         ("memo", memo)
      );
      produce_block();
      return r;
   }

   auto updateauth(const name &account, const name& code_account) {
      auto auth = authority(get_public_key(account, "active"));
      auth.accounts.push_back(permission_level_weight{{code_account, config::rem_code_name}, 1});

      auto r = base_tester::push_action(N(rem), N(updateauth), account, mvo()
         ("account", account.to_string())
         ("permission", "active")
         ("parent", "owner")
         ("auth", auth)
      );
      produce_blocks();
      return r;
   }

   auto addkeyacc(const name &account, const crypto::public_key &key, const crypto::signature &signed_by_key,
                  const string &extra_pub_key, const asset &price_limit, const string &payer_str,
                  const vector<permission_level> &auths) {
      auto r = base_tester::push_action(N(rem.auth), N(addkeyacc), auths, mvo()
         ("account",  account)
         ("pub_key_str", key )
         ("signed_by_pub_key", signed_by_key )
         ("extra_pub_key", extra_pub_key )
         ("price_limit", price_limit )
         ("payer_str", payer_str )
      );
      produce_block();
      return r;
   }

   auto addkeyapp(const name &account, const crypto::public_key &new_key, const crypto::signature &signed_by_new_key,
                  const string &extra_pub_key, const crypto::public_key &key, const crypto::signature &signed_by_key,
                  const asset &price_limit, const string &payer_str, const vector<permission_level> &auths) {
      auto r = base_tester::push_action(N(rem.auth), N(addkeyapp), auths, mvo()
         ("account",  account)
         ("new_pub_key_str", new_key )
         ("signed_by_new_pub_key", signed_by_new_key )
         ("extra_pub_key", extra_pub_key )
         ("pub_key_str", key )
         ("signed_by_pub_key", signed_by_key )
         ("price_limit", price_limit )
         ("payer_str", payer_str )
      );
      produce_block();
      return r;
   }

   auto revokeacc(const name &account, const crypto::public_key &key, const vector<permission_level>& auths) {
      auto r = base_tester::push_action(N(rem.auth), N(revokeacc), auths, mvo()
         ("account",  account)
         ("pub_key_str", key )
      );
      produce_block();
      return r;
   }

   auto revokeapp(const name &account, const crypto::public_key &revoke_key, const crypto::public_key &key,
                  const crypto::signature &signed_by_key, const vector<permission_level>& auths) {
      auto r = base_tester::push_action(N(rem.auth), N(revokeapp), auths, mvo()
         ("account",  account)
         ("revocation_pub_key_str", revoke_key )
         ("pub_key_str", key )
         ("signed_by_pub_key", signed_by_key )
      );
      produce_block();
      return r;
   }

   auto buyauth(const name &account, const asset &quantity, const double &max_price, const vector<permission_level>& auths) {
      auto r = base_tester::push_action(N(rem.auth), N(buyauth), auths, mvo()
         ("account",  account)
         ("quantity", quantity )
         ("max_price", max_price )
      );
      produce_block();
      return r;
   }

   auto setprice(const name& producer, std::map<name, double> &pairs_data) {
      auto r = base_tester::push_action(N(rem.oracle), N(setprice), producer, mvo()
         ("producer",  name(producer))
         ("pairs_data", pairs_data )
      );
      produce_block();
      return r;
   }

   auto addpair(const name& pair, const vector<permission_level>& level) {
      auto r = base_tester::push_action(N(rem.oracle), N(addpair), level, mvo()
         ("pair", pair )
      );
      produce_block();
      return r;
   }

   auto register_producer(name producer) {
      auto r = base_tester::push_action(config::system_account_name, N(regproducer), producer, mvo()
         ("producer",  name(producer))
         ("producer_key", get_public_key( producer, "active" ) )
         ("url", "" )
         ("location", 0 )
      );
      produce_block();
      return r;
   }

   void votepro(account_name voter, vector<account_name> producers) {
      std::sort( producers.begin(), producers.end() );
      base_tester::push_action(config::system_account_name, N(voteproducer), voter, mvo()
         ("voter", name(voter))
         ("proxy", name(0) )
         ("producers", producers)
      );
      produce_blocks();
   };

   variant get_authkeys_tbl( const name& account ) {
      return get_singtable(N(rem.auth), account, N(authkeys), "authkeys");
   }

   variant get_singtable(const name& contract, const name& scope, const name &table, const string &type) {
      vector<char> data;
      const auto &db = control->db();
      const auto *t_id = db.find<table_id_object, by_code_scope_table>(
         boost::make_tuple(contract, scope, table));
      if (!t_id) {
         return variant();
      }

      const auto &idx = db.get_index<key_value_index, by_scope_primary>();

      auto itr = idx.upper_bound(boost::make_tuple(t_id->id, std::numeric_limits<uint64_t>::max()));
      if (itr == idx.begin()) {
         return variant();
      }
      --itr;
      if (itr->t_id != t_id->id) {
         return variant();
      }

      data.resize(itr->value.size());
      memcpy(data.data(), itr->value.data(), data.size());
      return data.empty() ? variant() : abi_ser.binary_to_variant(type, data, abi_serializer_max_time);
   }

   variant get_remprice_tbl( const name& pair ) {
      vector<char> data = get_row_by_account( N(rem.oracle), N(rem.oracle), N(remprice), pair );
      return data.empty() ? fc::variant() : abi_ser_oracle.binary_to_variant( "remprice", data, abi_serializer_max_time );
   }

   asset get_balance( const account_name& act ) {
      return get_currency_balance(N(rem.token), symbol(CORE_SYMBOL), act);
   }

   asset get_balance_auth( const account_name& act ) {
      return get_currency_balance(N(rem.token), AUTH_SYMBOL, act);
   }

   auto get_stats( const symbol& sym ) {
      auto symbol_code = sym.to_symbol_code().value;
      vector<char> data = get_row_by_account( N(rem.token), name(symbol_code), N(stat), name(symbol_code) );
      return data.empty() ? fc::variant() : abi_ser_token.binary_to_variant( "currency_stats", data, abi_serializer_max_time );
   }

   auto get_storage_fee(const double &amount_keys) {
      auto rem_price_data = get_remprice_tbl(N(rem.usd));
      double amount = amount_keys * 10000 / rem_price_data["price"].as_double();
      return asset{ static_cast<int64_t>(amount), symbol(CORE_SYMBOL) };
   }

   auto get_storage_fee1() {
      double key_store_price = 1;
      uint32_t amount_purchased_auth = 0;
      auto rem_price_data = get_remprice_tbl(N(rem.usd));
      double auth_rem_price = key_store_price / rem_price_data["price"].as_double();
      variant auth_supply = get_stats(AUTH_SYMBOL);
      asset auth_contract_balance = get_balance(N(rem.auth));

//      return auth_contract_balance / ( + amount_purchased_auth);
      return asset::from_string(auth_supply["supply"].as_string()).get_amount();
   }

   void set_code_abi(const account_name& account, const vector<uint8_t>& wasm, const char* abi, const private_key_type* signer = nullptr) {
      wdump((account));
      set_code(account, wasm, signer);
      set_abi(account, abi, signer);
      if (account == N(rem.auth)) {
         const auto& accnt = control->db().get<account_object,by_name>( account );
         abi_def abi_definition;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi_definition), true);
         abi_ser.set_abi(abi_definition, abi_serializer_max_time);
      } else if (account == N(rem.token)) {
         const auto& accnt = control->db().get<account_object,by_name>( account );
         abi_def abi_definition;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi_definition), true);
         abi_ser_token.set_abi(abi_definition, abi_serializer_max_time);
      } else if (account == N(rem.oracle)) {
         const auto& accnt = control->db().get<account_object,by_name>( account );
         abi_def abi_definition;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi_definition), true);
         abi_ser_oracle.set_abi(abi_definition, abi_serializer_max_time);
      }
      produce_blocks();
   }

   static string join(vector<string> &&vec, string delim = string("*")) {
      return std::accumulate(std::next(vec.begin()), vec.end(), vec[0],
                             [&delim](string &a, string &b) {
                                return a + delim + b;
                             });
   }

   abi_serializer abi_ser;
   abi_serializer abi_ser_token;
   abi_serializer abi_ser_oracle;
};

rem_auth_tester::rem_auth_tester() {
   // Create rem.msig and rem.token, rem.auth
   create_accounts({N(rem.msig), N(rem.token), N(rem.rex), N(rem.ram), N(rem.ramfee), N(rem.oracle),
                    N(rem.stake), N(rem.bpay), N(rem.spay), N(rem.vpay), N(rem.saving), N(rem.auth) });

   // Register producers
   const auto producer_candidates = {
      N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
      N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
      N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ)
   };
   // Set code for the following accounts:
   //  - rem (code: rem.bios) (already set by tester constructor)
   //  - rem.msig (code: rem.msig)
   //  - rem.token (code: rem.token)
   //  - rem.auth (code: rem.auth)
   set_code_abi(N(rem.msig),
                contracts::rem_msig_wasm(),
                contracts::rem_msig_abi().data());//, &rem_active_pk);
   set_code_abi(N(rem.token),
                contracts::rem_token_wasm(),
                contracts::rem_token_abi().data()); //, &rem_active_pk);
   set_code_abi(N(rem.auth),
                contracts::rem_auth_wasm(),
                contracts::rem_auth_abi().data()); //, &rem_active_pk);
   set_code_abi(N(rem.oracle),
                contracts::rem_oracle_wasm(),
                contracts::rem_oracle_abi().data()); //, &rem_active_pk);

   // Set privileged for rem.msig and rem.token
   set_privileged(N(rem.msig));
   set_privileged(N(rem.token));

   // Verify rem.msig and rem.token is privileged
   const auto& rem_msig_acc = get<account_metadata_object, by_name>(N(rem.msig));
   BOOST_TEST(rem_msig_acc.is_privileged() == true);
   const auto& rem_token_acc = get<account_metadata_object, by_name>(N(rem.token));
   BOOST_TEST(rem_token_acc.is_privileged() == true);

   // Create SYS, AUTH tokens in rem.token, set its manager as rem
   const auto max_supply_core     = core_from_string("1000000000.0000"); /// 10x larger than 1B initial tokens
   const auto max_supply_auth     = auth_from_string("100000000000.0000"); /// 10x larger than 1B initial tokens
   const auto initial_supply_core = core_from_string("100000000.0000");  /// 10x larger than 1B initial tokens

   create_currency(N(rem.token), config::system_account_name, max_supply_core);
   create_currency(N(rem.token), N(rem.auth), max_supply_auth);
   // Issue the genesis supply of 1 billion SYS tokens to rem.system
   issue(N(rem.token), config::system_account_name, config::system_account_name, initial_supply_core);

   auto actual = get_balance(config::system_account_name);
   BOOST_REQUIRE_EQUAL(initial_supply_core, actual);

   struct rem_genesis_account {
      account_name name;
      uint64_t     initial_balance;
   };

   std::vector<rem_genesis_account> genesis_test( {
     {N(b1),        100'000'000'0000ll},
     {N(whale1),     40'000'000'0000ll},
     {N(whale2),     30'000'000'0000ll},
     {N(whale3),     20'000'000'0000ll},
     {N(proda),         500'000'0000ll},
     {N(prodb),         500'000'0000ll},
     {N(prodc),         500'000'0000ll},
     {N(prodd),         500'000'0000ll},
     {N(prode),         500'000'0000ll},
     {N(prodf),         500'000'0000ll},
     {N(prodg),         500'000'0000ll},
     {N(prodh),         500'000'0000ll},
     {N(prodi),         500'000'0000ll},
     {N(prodj),         500'000'0000ll},
     {N(prodk),         500'000'0000ll},
     {N(prodl),         500'000'0000ll},
     {N(prodm),         500'000'0000ll},
     {N(prodn),         500'000'0000ll},
     {N(prodo),         500'000'0000ll},
     {N(prodp),         500'000'0000ll},
     {N(prodq),         500'000'0000ll},
     {N(prodr),         500'000'0000ll},
     {N(prods),         500'000'0000ll},
     {N(prodt),         500'000'0000ll},
     {N(produ),         500'000'0000ll},
     {N(runnerup1),     200'000'0000ll},
     {N(runnerup2),     150'000'0000ll},
     {N(runnerup3),     100'000'0000ll},
   });

   // Create genesis accounts
   for( const auto& account : genesis_test ) {
      create_account( account.name, config::system_account_name );
   }

   deploy_contract();

   // Buy ram and stake cpu and net for each genesis accounts
   for( const auto& account : genesis_test ) {
      const auto stake_quantity = account.initial_balance - 1000;

      const auto r = delegate_bandwidth(N(rem.stake), account.name, asset(stake_quantity));
      BOOST_REQUIRE( !r->except_ptr );
   }

   for( const auto& producer : producer_candidates ) {
      register_producer(producer);
   }

   const auto whales_as_producers = { N(b1), N(whale1), N(whale2) };
   for( const auto& producer : whales_as_producers ) {
      register_producer(producer);
   }

   votepro(N(whale1), { N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                        N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                        N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ) });
   votepro( N(whale2), { N(proda), N(prodb), N(prodc), N(prodd), N(prode) } );
   votepro( N(b1), { N(proda), N(prodb), N(prodc), N(prodd), N(prode) } );
   updateauth(N(rem.auth), N(rem.auth));

   // add new supported pairs to the rem.oracle
   vector<name> supported_pairs = {
      N(rem.usd), N(rem.eth), N(rem.btc),
   };
   for (const auto &pair : supported_pairs) {
      addpair(pair, { {N(rem.oracle), config::active_name} });
   }
   map<name, double> pair_price {
      {N(rem.usd), 0.003210},
      {N(rem.btc), 0.0000003957},
      {N(rem.eth), 0.0000176688}
   };
   const auto _producers = control->head_block_state()->active_schedule.producers;
   for (const auto &producer : _producers) {
      setprice(producer.producer_name, pair_price);
   }
}

BOOST_AUTO_TEST_SUITE(rem_auth_tests)

BOOST_FIXTURE_TEST_CASE( addkeyacc_pay_by_rem_test, rem_auth_tester ) {
   try {
      name account = N(proda);
      vector<permission_level> auths_level = { permission_level{account, config::active_name} };
      // set account permission rem@code to the rem.auth (allow to execute the action on behalf of the account to rem.auth)
      updateauth(account, N(rem.auth));
      crypto::private_key key_priv = crypto::private_key::generate();
      crypto::public_key key_pub   = key_priv.get_public_key();
      const auto price_limit       = core_from_string("500.0000");
      string extra_pub_key         = "MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAIZDXel8Nh0xnGOo39XE3Jqdi6iQpxRs\n"
                                     "/r82O1HnpuJFd/jyM3iWInPZvmOnPCP3/Nx4fRNj1y0U9QFnlfefNeECAwEAAQ==";
      string payer_str;

      sha256 digest = sha256::hash(join( { account.to_string(), string(key_pub), extra_pub_key, payer_str } ));
      auto signed_by_key = key_priv.sign(digest);

      // tokens to pay for torewards action
      transfer(config::system_account_name, account, core_from_string("500.0000"), "initial transfer");
      auto account_balance_before = get_balance(account);

      addkeyacc(account, key_pub, signed_by_key, extra_pub_key, price_limit, payer_str, auths_level);

      // account balance after addkeyacc should be a account_balance_before - 1 AUTH (to current market price)
      auto account_balance_after = get_balance(account);
      auto rem_price_data = get_remprice_tbl(N(rem.usd));
      auto auth_stats = get_stats(AUTH_SYMBOL);
      auto data = get_authkeys_tbl(account);

      asset storage_fee = get_storage_fee(1);

      auto ct = control->head_block_time();
      BOOST_REQUIRE_EQUAL(data["owner"].as_string(), account.to_string());
      BOOST_REQUIRE_EQUAL(data["public_key"].as_string(), string(key_pub));
      BOOST_REQUIRE_EQUAL(data["not_valid_before"].as_string(), string(ct));
      BOOST_REQUIRE_EQUAL(data["not_valid_after"].as_string(), string(ct + days(360)));
      BOOST_REQUIRE_EQUAL(data["extra_public_key"].as_string(), extra_pub_key);
      BOOST_REQUIRE_EQUAL(data["revoked_at"].as_string(), "0"); // if not revoked == 0
      BOOST_REQUIRE_EQUAL(account_balance_before - storage_fee, account_balance_after);
      BOOST_REQUIRE_EQUAL(auth_stats["supply"].as_string(), "0.0000 AUTH");

      // action's authorizing actor 'fail' does not exist
      BOOST_REQUIRE_THROW(
         addkeyacc( account, key_pub, signed_by_key, extra_pub_key, price_limit, payer_str,
                    { permission_level{N(fail), config::active_name} } ),
                    transaction_exception);
      // Missing authority of prodb
      BOOST_REQUIRE_THROW(
         addkeyacc( account, key_pub, signed_by_key, extra_pub_key, price_limit, "prodb",
                   { permission_level{account, config::active_name} } ),
                   missing_auth_exception);
      // Missing authority of proda
      BOOST_REQUIRE_THROW(
         addkeyacc( account, key_pub, signed_by_key, extra_pub_key, price_limit, "prodb",
                    { permission_level{N(prodb), config::active_name} } ),
                    missing_auth_exception);
      // action's authorizing actor "" does not exist
      BOOST_REQUIRE_THROW(
         addkeyacc( account, key_pub, signed_by_key, extra_pub_key, price_limit, "prodb", { permission_level{} } ),
                    transaction_exception);
      // Error expected key different than recovered key
      BOOST_REQUIRE_THROW(
         addkeyacc( account, get_public_key(N(prodb), "active"), signed_by_key, extra_pub_key, price_limit, payer_str, auths_level ),
                    crypto_api_exception);
      // overdrawn balance
      BOOST_REQUIRE_THROW(
         addkeyacc( account, key_pub, signed_by_key, extra_pub_key, price_limit, payer_str, auths_level ),
                    eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( addkeyacc_pay_by_rem_with_another_payer_test, rem_auth_tester ) {
   try {
      name account = N(proda);
      name payer = N(prodb);
      vector<permission_level> auths_level = { permission_level{account, config::active_name},
                                               permission_level{payer, config::active_name} };
      // set account permission rem@code to the rem.auth (allow to execute the action on behalf of the account to rem.auth)
      updateauth(account, N(rem.auth));
      updateauth(payer, N(rem.auth));
      crypto::private_key key_priv = crypto::private_key::generate();
      crypto::public_key key_pub   = key_priv.get_public_key();
      const auto price_limit       = core_from_string("500.0000");
      string extra_pub_key         = "MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAIZDXel8Nh0xnGOo39XE3Jqdi6iQpxRs\n"
                                     "/r82O1HnpuJFd/jyM3iWInPZvmOnPCP3/Nx4fRNj1y0U9QFnlfefNeECAwEAAQ==";
      string payer_str             = payer.to_string();

      sha256 digest = sha256::hash(join( { account.to_string(), string(key_pub), extra_pub_key, payer_str } ));
      auto signed_by_key = key_priv.sign(digest);

      // tokens to pay for torewards action
      transfer(config::system_account_name, payer, core_from_string("500.0000"), "initial transfer");
      auto payer_balance_before = get_balance(payer);

      addkeyacc(account, key_pub, signed_by_key, extra_pub_key, price_limit, payer_str, auths_level);

      // account balance after addkeyacc should be a account_balance_before - 1 AUTH (to current market price)
      auto payer_balance_after = get_balance(payer);
      asset storage_fee = get_storage_fee(1);
      auto auth_stats = get_stats(AUTH_SYMBOL);
      auto data = get_authkeys_tbl(account);

      auto ct = control->head_block_time();
      BOOST_REQUIRE_EQUAL(data["owner"].as_string(), account.to_string());
      BOOST_REQUIRE_EQUAL(data["public_key"].as_string(), string(key_pub));
      BOOST_REQUIRE_EQUAL(data["not_valid_before"].as_string(), string(ct));
      BOOST_REQUIRE_EQUAL(data["not_valid_after"].as_string(), string(ct + days(360)));
      BOOST_REQUIRE_EQUAL(data["extra_public_key"].as_string(), extra_pub_key);
      BOOST_REQUIRE_EQUAL(data["revoked_at"].as_string(), "0"); // if not revoked == 0
      BOOST_REQUIRE_EQUAL(payer_balance_before - storage_fee, payer_balance_after);
      BOOST_REQUIRE_EQUAL(auth_stats["supply"].as_string(), "0.0000 AUTH");

      // action's authorizing actor 'fail' does not exist
      BOOST_REQUIRE_THROW(
         addkeyacc( account, key_pub, signed_by_key, extra_pub_key, price_limit, payer_str,
                    { permission_level{N(fail), config::active_name} } ),
                    transaction_exception);
      // Missing authority of prodc
      BOOST_REQUIRE_THROW(
         addkeyacc( account, key_pub, signed_by_key, extra_pub_key, price_limit, "prodc",
                    { permission_level{account, config::active_name} } ),
                    missing_auth_exception);
      // Missing authority of proda
      BOOST_REQUIRE_THROW(
         addkeyacc( account, key_pub, signed_by_key, extra_pub_key, price_limit, "prodb",
                    { permission_level{N(prodb), config::active_name} } ),
                    missing_auth_exception);
      // action's authorizing actor "" does not exist
      BOOST_REQUIRE_THROW(
         addkeyacc( account, key_pub, signed_by_key, extra_pub_key, price_limit, "prodb", { permission_level{} } ),
                    transaction_exception);
      // Error expected key different than recovered key
      BOOST_REQUIRE_THROW(
         addkeyacc( account, get_public_key(N(prodb), "active"), signed_by_key, extra_pub_key, price_limit, payer_str, auths_level ),
                    crypto_api_exception);
      // overdrawn balance
      BOOST_REQUIRE_THROW(
         addkeyacc( account, key_pub, signed_by_key, extra_pub_key, price_limit, payer_str, auths_level ),
                    eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( addkeyacc_pay_by_auth_test, rem_auth_tester ) {
   try {
      name account = N(proda);
      vector<permission_level> auths_level = { permission_level{account, config::active_name} };
      // set account permission rem@code to the rem.auth (allow to execute the action on behalf of the account to rem.auth)
      updateauth(account, N(rem.auth));
      crypto::private_key key_priv = crypto::private_key::generate();
      crypto::public_key key_pub   = key_priv.get_public_key();
      const auto price_limit       = auth_from_string("1.0000");
      string extra_pub_key         = "MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAIZDXel8Nh0xnGOo39XE3Jqdi6iQpxRs\n"
                                     "/r82O1HnpuJFd/jyM3iWInPZvmOnPCP3/Nx4fRNj1y0U9QFnlfefNeECAwEAAQ==";
      string payer_str;

      sha256 digest = sha256::hash(join( { account.to_string(), string(key_pub), extra_pub_key, payer_str } ));
      auto signed_by_key = key_priv.sign(digest);

      // tokens to pay for torewards action
      transfer(config::system_account_name, account, core_from_string("500.0000"), "initial transfer");
      // buy auth credit for paying addkeyacc
      auto account_balance_before = get_balance(account);
      auto auth_stats_before = get_stats(AUTH_SYMBOL);
      buyauth(account, price_limit, 1, auths_level); // buy 1 AUTH credit
      // account balance after addkeyacc should be a account_balance_before - 1 AUTH (to current market price)
      auto account_balance_after = get_balance(account);
      auto auth_stats_after = get_stats(AUTH_SYMBOL);
      auto account_auth_balance_before = get_balance_auth(account);
      asset storage_fee = get_storage_fee(1);

      addkeyacc(account, key_pub, signed_by_key, extra_pub_key, price_limit, payer_str, auths_level);


      auto account_auth_balance_after = get_balance_auth(account);
      auto auth_supply_before = asset::from_string(auth_stats_before["supply"].as_string());
      auto auth_supply_after = asset::from_string(auth_stats_after["supply"].as_string());

      auto data = get_authkeys_tbl(account);

      auto ct = control->head_block_time();
      BOOST_REQUIRE_EQUAL(data["owner"].as_string(), account.to_string());
      BOOST_REQUIRE_EQUAL(data["public_key"].as_string(), string(key_pub));
      BOOST_REQUIRE_EQUAL(data["not_valid_before"].as_string(), string(ct));
      BOOST_REQUIRE_EQUAL(data["not_valid_after"].as_string(), string(ct + days(360)));
      BOOST_REQUIRE_EQUAL(data["extra_public_key"].as_string(), extra_pub_key);
      BOOST_REQUIRE_EQUAL(data["revoked_at"].as_string(), "0"); // if not revoked == 0
      BOOST_REQUIRE_EQUAL(account_balance_before - storage_fee, account_balance_after);
      BOOST_REQUIRE_EQUAL(account_auth_balance_before.get_amount() - 10'000, account_auth_balance_after.get_amount());
      BOOST_REQUIRE_EQUAL(auth_supply_before.get_amount() + 10'000, auth_supply_after.get_amount());

      // action's authorizing actor 'fail' does not exist
      BOOST_REQUIRE_THROW(
         addkeyacc( account, key_pub, signed_by_key, extra_pub_key, price_limit, payer_str,
                    { permission_level{N(fail), config::active_name} } ),
                    transaction_exception);
      // Missing authority of prodb
      BOOST_REQUIRE_THROW(
         addkeyacc( account, key_pub, signed_by_key, extra_pub_key, price_limit, "prodb",
                    { permission_level{account, config::active_name} } ),
                    missing_auth_exception);
      // Missing authority of proda
      BOOST_REQUIRE_THROW(
         addkeyacc( account, key_pub, signed_by_key, extra_pub_key, price_limit, "prodb",
                    { permission_level{N(prodb), config::active_name} } ),
                    missing_auth_exception);
      // action's authorizing actor "" does not exist
      BOOST_REQUIRE_THROW(
         addkeyacc( account, key_pub, signed_by_key, extra_pub_key, price_limit, "prodb", { permission_level{} } ),
                    transaction_exception);
      // Error expected key different than recovered key
      BOOST_REQUIRE_THROW(
         addkeyacc( account, get_public_key(N(prodb), "active"), signed_by_key, extra_pub_key, price_limit, payer_str, auths_level ),
                    crypto_api_exception);
      // overdrawn balance
      BOOST_REQUIRE_THROW(
         addkeyacc( account, key_pub, signed_by_key, extra_pub_key, price_limit, payer_str, auths_level ),
                    eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( addkeyacc_pay_by_auth_with_another_payer_test, rem_auth_tester ) {
   try {
      name account = N(proda);
      name payer = N(prodb);
      vector<permission_level> auths_level = { permission_level{account, config::active_name},
                                               permission_level{payer, config::active_name}};
      // set account permission rem@code to the rem.auth (allow to execute the action on behalf of the account to rem.auth)
      updateauth(account, N(rem.auth));
      updateauth(payer, N(rem.auth));
      crypto::private_key key_priv = crypto::private_key::generate();
      crypto::public_key key_pub   = key_priv.get_public_key();
      const auto price_limit       = auth_from_string("1.0000");
      string extra_pub_key         = "MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAIZDXel8Nh0xnGOo39XE3Jqdi6iQpxRs\n"
                                     "/r82O1HnpuJFd/jyM3iWInPZvmOnPCP3/Nx4fRNj1y0U9QFnlfefNeECAwEAAQ==";
      string payer_str             = payer.to_string();

      sha256 digest = sha256::hash(join( { account.to_string(), string(key_pub), extra_pub_key, payer_str } ));
      auto signed_by_key = key_priv.sign(digest);

      // tokens to pay for torewards action
      transfer(config::system_account_name, payer, core_from_string("500.0000"), "initial transfer");

      auto payer_balance_before = get_balance(payer);
      auto auth_stats_before = get_stats(AUTH_SYMBOL);

      buyauth(payer, price_limit, 1, auths_level); // buy 1 AUTH credit
      // account balance after addkeyacc should be a account_balance_before - 1 AUTH (to current market price)
      auto payer_balance_after = get_balance(payer);
      auto auth_stats_after = get_stats(AUTH_SYMBOL);
      auto payer_balance_auth = get_balance_auth(account);
      asset storage_fee = get_storage_fee(1);

      addkeyacc(account, key_pub, signed_by_key, extra_pub_key, price_limit, payer_str, auths_level);

      auto auth_supply_before = asset::from_string(auth_stats_before["supply"].as_string());
      auto auth_supply_after = asset::from_string(auth_stats_after["supply"].as_string());

      auto data = get_authkeys_tbl(account);

      auto ct = control->head_block_time();
      BOOST_REQUIRE_EQUAL(data["owner"].as_string(), account.to_string());
      BOOST_REQUIRE_EQUAL(data["public_key"].as_string(), string(key_pub));
      BOOST_REQUIRE_EQUAL(data["not_valid_before"].as_string(), string(ct));
      BOOST_REQUIRE_EQUAL(data["not_valid_after"].as_string(), string(ct + days(360)));
      BOOST_REQUIRE_EQUAL(data["extra_public_key"].as_string(), extra_pub_key);
      BOOST_REQUIRE_EQUAL(data["revoked_at"].as_string(), "0"); // if not revoked == 0
      BOOST_REQUIRE_EQUAL(payer_balance_before - storage_fee, payer_balance_after);
      BOOST_REQUIRE_EQUAL(payer_balance_auth.get_amount(), 0);
      BOOST_REQUIRE_EQUAL(auth_supply_before.get_amount() + 10'000, auth_supply_after.get_amount());

      // action's authorizing actor 'fail' does not exist
      BOOST_REQUIRE_THROW(
         addkeyacc( account, key_pub, signed_by_key, extra_pub_key, price_limit, payer_str,
                    { permission_level{N(fail), config::active_name} } ),
                    transaction_exception);
      // Missing authority of prodc
      BOOST_REQUIRE_THROW(
         addkeyacc( account, key_pub, signed_by_key, extra_pub_key, price_limit, "prodc",
                    { permission_level{account, config::active_name} } ),
                    missing_auth_exception);
      // Missing authority of proda
      BOOST_REQUIRE_THROW(
         addkeyacc( account, key_pub, signed_by_key, extra_pub_key, price_limit, "prodb",
                    { permission_level{N(prodb), config::active_name} } ),
                    missing_auth_exception);
      // action's authorizing actor "" does not exist
      BOOST_REQUIRE_THROW(
         addkeyacc( account, key_pub, signed_by_key, extra_pub_key, price_limit, "prodb", { permission_level{} } ),
                    transaction_exception);
      // Error expected key different than recovered key
      BOOST_REQUIRE_THROW(
         addkeyacc( account, get_public_key(N(prodb), "active"), signed_by_key, extra_pub_key, price_limit, payer_str, auths_level ),
                    crypto_api_exception);
      // overdrawn balance
      BOOST_REQUIRE_THROW(
         addkeyacc( account, key_pub, signed_by_key, extra_pub_key, price_limit, payer_str, auths_level ),
                    eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( addkeyapp_pay_by_rem_test, rem_auth_tester ) {
   try {
      name account = N(proda);
      vector<permission_level> auths_level = { permission_level{N(prodb), config::active_name} }; // prodb as a executor
      // set account permission rem@code to the rem.auth (allow to execute the action on behalf of the account to rem.auth)
      updateauth(account, N(rem.auth));
      crypto::private_key new_key_priv = crypto::private_key::generate();
      crypto::private_key key_priv     = crypto::private_key::generate();
      crypto::public_key new_key_pub   = new_key_priv.get_public_key();
      crypto::public_key key_pub       = key_priv.get_public_key();
      const auto price_limit           = core_from_string("500.0000");
      string extra_pub_key             = "MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAIZDXel8Nh0xnGOo39XE3Jqdi6iQpxRs\n"
                                         "/r82O1HnpuJFd/jyM3iWInPZvmOnPCP3/Nx4fRNj1y0U9QFnlfefNeECAwEAAQ==";
      string payer_str;

      sha256 digest_addkeyacc = sha256::hash(join( { account.to_string(), string(key_pub), extra_pub_key, payer_str } ));

      sha256 digest_addkeyapp = sha256::hash(join({ account.to_string(), string(new_key_pub), extra_pub_key,
                                                    string(key_pub), payer_str }) );

      auto signed_by_key = key_priv.sign(digest_addkeyacc);
      auto signed_by_new_key_app = new_key_priv.sign(digest_addkeyapp);
      auto signed_by_key_app = key_priv.sign(digest_addkeyapp);

      // tokens to pay for torewards action
      transfer(config::system_account_name, account, core_from_string("1000.0000"), "initial transfer");
      auto account_balance_before = get_balance(account);

      addkeyacc(account, key_pub, signed_by_key, extra_pub_key, price_limit, payer_str,
                { permission_level{account, config::active_name} });
      addkeyapp(account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                key_pub, signed_by_key_app, price_limit, payer_str, auths_level);

      // account balance after addkeyapp should be a account_balance_before -  10.0000 tokens (torewards action)
      auto account_balance_after = get_balance(account);
      auto auth_stats = get_stats(AUTH_SYMBOL);
      asset storage_fee = get_storage_fee(1);
      auto data = get_authkeys_tbl(account);

      auto ct = control->head_block_time();
      BOOST_REQUIRE_EQUAL(data["owner"].as_string(), account.to_string());
      BOOST_REQUIRE_EQUAL(data["public_key"].as_string(), string(new_key_pub));
      BOOST_REQUIRE_EQUAL(data["not_valid_before"].as_string(), string(ct));
      BOOST_REQUIRE_EQUAL(data["not_valid_after"].as_string(), string(ct + days(360)));
      BOOST_REQUIRE_EQUAL(data["extra_public_key"].as_string(), extra_pub_key);
      BOOST_REQUIRE_EQUAL(data["revoked_at"].as_string(), "0"); // if not revoked == 0
      BOOST_REQUIRE_EQUAL(account_balance_before - (storage_fee + storage_fee), account_balance_after);
      BOOST_REQUIRE_EQUAL(auth_stats["supply"].as_string(), "0.0000 AUTH");

      // Missing required authority payer
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, "accountnum3", auths_level),
                    missing_auth_exception );
      // character is not in allowed character set for names
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, "dlas*fas.", auths_level),
                    eosio_assert_message_exception );
      // action's authorizing actor 'fail' does not exist
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, payer_str,
                    { permission_level{N(fail), config::active_name} }),
                    transaction_exception );
      // action's authorizing actor "" does not exist
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, payer_str, { permission_level{} } ),
                    transaction_exception);
      // expected key different than recovered new key
      BOOST_REQUIRE_THROW(
         addkeyapp( account, get_public_key(N(prodb), "active"), signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, payer_str, auths_level ),
                    eosio_assert_message_exception);
      // expected key different than recovered user key
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    get_public_key(N(prodb)), signed_by_key_app, price_limit, payer_str, auths_level ),
                    eosio_assert_message_exception);
      // overdrawn balance
      transfer(account, config::system_account_name, core_from_string("350.0000"), "too small balance test");
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, payer_str, auths_level ),
                    eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( addkeyapp_pay_by_rem_with_another_payer_test, rem_auth_tester ) {
   try {
      name account = N(proda);
      name payer = N(prodb);
      vector<permission_level> auths_level = { permission_level{N(prodb), config::active_name} }; // prodb as a executor
      // set account permission rem@code to the rem.auth (allow to execute the action on behalf of the account to rem.auth)
      updateauth(account, N(rem.auth));
      updateauth(payer, N(rem.auth));
      crypto::private_key new_key_priv = crypto::private_key::generate();
      crypto::private_key key_priv     = crypto::private_key::generate();
      crypto::public_key new_key_pub   = new_key_priv.get_public_key();
      crypto::public_key key_pub       = key_priv.get_public_key();
      const auto price_limit           = core_from_string("700.0000");
      string extra_pub_key             = "MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAIZDXel8Nh0xnGOo39XE3Jqdi6iQpxRs\n"
                                         "/r82O1HnpuJFd/jyM3iWInPZvmOnPCP3/Nx4fRNj1y0U9QFnlfefNeECAwEAAQ==";
      string payer_str                 = payer.to_string();


      sha256 digest_addkeyacc = sha256::hash(join( { account.to_string(), string(key_pub), extra_pub_key, payer_str } ));

      sha256 digest_addkeyapp = sha256::hash(join({ account.to_string(), string(new_key_pub), extra_pub_key,
                                                    string(key_pub), payer_str }) );

      auto signed_by_key          = key_priv.sign(digest_addkeyacc);
      auto signed_by_new_key_app  = new_key_priv.sign(digest_addkeyapp);
      auto signed_by_key_app      = key_priv.sign(digest_addkeyapp);

      // tokens to pay for torewards action
      transfer(config::system_account_name, payer, core_from_string("1000.0000"), "initial transfer");
      auto payer_balance_before = get_balance(payer);

      addkeyacc(account, key_pub, signed_by_key, extra_pub_key, price_limit, payer_str,
                { permission_level{account, config::active_name}, permission_level{N(prodb), config::active_name} });
      addkeyapp(account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                key_pub, signed_by_key_app, price_limit, payer_str, auths_level);

      // account balance after addkeyacc should be a account_balance_before - 1 AUTH (to current market price)
      auto payer_balance_after = get_balance(payer);
      auto auth_stats = get_stats(AUTH_SYMBOL);
      asset storage_fee = get_storage_fee(1);

      auto data = get_authkeys_tbl(account);

      auto ct = control->head_block_time();
      BOOST_REQUIRE_EQUAL(data["owner"].as_string(), account.to_string());
      BOOST_REQUIRE_EQUAL(data["public_key"].as_string(), string(new_key_pub));
      BOOST_REQUIRE_EQUAL(data["not_valid_before"].as_string(), string(ct));
      BOOST_REQUIRE_EQUAL(data["not_valid_after"].as_string(), string(ct + days(360)));
      BOOST_REQUIRE_EQUAL(data["extra_public_key"].as_string(), extra_pub_key);
      BOOST_REQUIRE_EQUAL(data["revoked_at"].as_string(), "0"); // if not revoked == 0
      BOOST_REQUIRE_EQUAL(payer_balance_before - (storage_fee + storage_fee), payer_balance_after);
      BOOST_REQUIRE_EQUAL(auth_stats["supply"].as_string(), "0.0000 AUTH");

      // Missing required authority payer
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, "accountnum3", auths_level),
                    missing_auth_exception );
      // character is not in allowed character set for names
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, "dlas*fas.", auths_level),
                    eosio_assert_message_exception );
      // action's authorizing actor 'fail' does not exist
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, payer_str,
                    { permission_level{N(fail), config::active_name} }),
                    transaction_exception );
      // action's authorizing actor "" does not exist
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, payer_str, { permission_level{} } ),
                    transaction_exception);
      // expected key different than recovered new key
      BOOST_REQUIRE_THROW(
         addkeyapp( account, get_public_key(N(prodb), "active"), signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, payer_str, auths_level ),
                    eosio_assert_message_exception);
      // expected key different than recovered user key
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    get_public_key(N(prodb)), signed_by_key_app, price_limit, payer_str, auths_level ),
                    eosio_assert_message_exception);
      // overdrawn balance
      transfer(payer, config::system_account_name, core_from_string("350.0000"), "too small balance test");
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, payer_str, auths_level ),
                    eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( addkeyapp_pay_by_auth_test, rem_auth_tester ) {
   try {
      name account = N(proda);
      vector<permission_level> auths_level = { permission_level{N(prodb), config::active_name} }; // prodb as a executor
      // set account permission rem@code to the rem.auth (allow to execute the action on behalf of the account to rem.auth)
      updateauth(account, N(rem.auth));
      crypto::private_key new_key_priv = crypto::private_key::generate();
      crypto::private_key key_priv     = crypto::private_key::generate();
      crypto::public_key new_key_pub   = new_key_priv.get_public_key();
      crypto::public_key key_pub       = key_priv.get_public_key();
      const auto price_limit           = auth_from_string("2.0000");
      string extra_pub_key             = "MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAIZDXel8Nh0xnGOo39XE3Jqdi6iQpxRs\n"
                                         "/r82O1HnpuJFd/jyM3iWInPZvmOnPCP3/Nx4fRNj1y0U9QFnlfefNeECAwEAAQ==";
      string payer_str;

      sha256 digest_addkeyacc = sha256::hash(join( { account.to_string(), string(key_pub), extra_pub_key, payer_str } ));

      sha256 digest_addkeyapp = sha256::hash(join({ account.to_string(), string(new_key_pub), extra_pub_key,
                                                    string(key_pub), payer_str }) );

      auto signed_by_key          = key_priv.sign(digest_addkeyacc);
      auto signed_by_new_key_app  = new_key_priv.sign(digest_addkeyapp);
      auto signed_by_key_app      = key_priv.sign(digest_addkeyapp);

      // tokens to pay for torewards action
      transfer(config::system_account_name, account, core_from_string("1000.0000"), "initial transfer");

      auto account_balance_before = get_balance(account);
      auto auth_stats_before = get_stats(AUTH_SYMBOL);

      buyauth(account, price_limit, 1, { permission_level{account, config::active_name} }); // Buy 2 AUTH credit
      // account balance after addkeyacc should be a account_balance_before - 1 AUTH (to current market price)
      auto account_balance_after = get_balance(account);
      auto auth_stats_after = get_stats(AUTH_SYMBOL);
      auto account_auth_balance_before = get_balance_auth(account);
      asset storage_fee = get_storage_fee(2);

      addkeyacc(account, key_pub, signed_by_key, extra_pub_key, price_limit, payer_str,
                { permission_level{account, config::active_name} });
      addkeyapp(account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                key_pub, signed_by_key_app, price_limit, payer_str, auths_level);

      auto account_auth_balance_after = get_balance_auth(account);
      auto auth_supply_before = asset::from_string(auth_stats_before["supply"].as_string());
      auto auth_supply_after = asset::from_string(auth_stats_after["supply"].as_string());

      auto data = get_authkeys_tbl(account);

      auto ct = control->head_block_time();
      BOOST_REQUIRE_EQUAL(data["owner"].as_string(), account.to_string());
      BOOST_REQUIRE_EQUAL(data["public_key"].as_string(), string(new_key_pub));
      BOOST_REQUIRE_EQUAL(data["not_valid_before"].as_string(), string(ct));
      BOOST_REQUIRE_EQUAL(data["not_valid_after"].as_string(), string(ct + days(360)));
      BOOST_REQUIRE_EQUAL(data["extra_public_key"].as_string(), extra_pub_key);
      BOOST_REQUIRE_EQUAL(data["revoked_at"].as_string(), "0"); // if not revoked == 0
      BOOST_REQUIRE_EQUAL(account_balance_before - storage_fee, account_balance_after);
      BOOST_REQUIRE_EQUAL(account_auth_balance_before.get_amount() - 2 * 10'000, account_auth_balance_after.get_amount());
      BOOST_REQUIRE_EQUAL(auth_supply_before.get_amount() + 2 * 10'000, auth_supply_after.get_amount());

      // Missing required authority payer
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, "accountnum3", auths_level),
                    missing_auth_exception );
      // character is not in allowed character set for names
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, "dlas*fas.", auths_level),
                    eosio_assert_message_exception );
      // action's authorizing actor 'fail' does not exist
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, payer_str,
                    { permission_level{N(fail), config::active_name} }),
                    transaction_exception );
      // action's authorizing actor "" does not exist
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, payer_str, { permission_level{} } ),
                    transaction_exception);
      // expected key different than recovered new key
      BOOST_REQUIRE_THROW(
         addkeyapp( account, get_public_key(N(prodb), "active"), signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, payer_str, auths_level ),
                    eosio_assert_message_exception);
      // expected key different than recovered user key
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    get_public_key(N(prodb)), signed_by_key_app, price_limit, payer_str, auths_level ),
                    eosio_assert_message_exception);
      // overdrawn balance
      transfer(account, config::system_account_name, core_from_string("350.0000"), "too small balance test");
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, payer_str, auths_level ),
                    eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( addkeyapp_pay_by_auth_with_another_payer_test, rem_auth_tester ) {
   try {
      name account = N(proda);
      name payer = N(prodb);
      vector<permission_level> auths_level = { permission_level{N(prodb), config::active_name} }; // prodb as a executor
      // set account permission rem@code to the rem.auth (allow to execute the action on behalf of the account to rem.auth)
      updateauth(account, N(rem.auth));
      updateauth(payer, N(rem.auth));
      crypto::private_key new_key_priv = crypto::private_key::generate();
      crypto::private_key key_priv     = crypto::private_key::generate();
      crypto::public_key new_key_pub   = new_key_priv.get_public_key();
      crypto::public_key key_pub       = key_priv.get_public_key();
      const auto price_limit           = auth_from_string("2.0000");
      string extra_pub_key             = "MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAIZDXel8Nh0xnGOo39XE3Jqdi6iQpxRs\n"
                                         "/r82O1HnpuJFd/jyM3iWInPZvmOnPCP3/Nx4fRNj1y0U9QFnlfefNeECAwEAAQ==";
      string payer_str                 = payer.to_string();

      sha256 digest_addkeyacc = sha256::hash(join( { account.to_string(), string(key_pub), extra_pub_key, payer_str } ));

      sha256 digest_addkeyapp = sha256::hash(join({ account.to_string(), string(new_key_pub), extra_pub_key,
                                                    string(key_pub), payer_str }) );

      auto signed_by_key          = key_priv.sign(digest_addkeyacc);
      auto signed_by_new_key_app  = new_key_priv.sign(digest_addkeyapp);
      auto signed_by_key_app      = key_priv.sign(digest_addkeyapp);

      // tokens to pay for torewards action
      transfer(config::system_account_name, payer, core_from_string("1000.0000"), "initial transfer");

      auto payer_balance_before = get_balance(payer);
      auto auth_stats_before = get_stats(AUTH_SYMBOL);

      buyauth(payer, price_limit, 1, auths_level); // buy 1 AUTH credit
      // account balance after addkeyacc should be a account_balance_before - 1 AUTH (to current market price)
      auto payer_balance_after = get_balance(payer);
      auto auth_stats_after = get_stats(AUTH_SYMBOL);
      auto payer_auth_balance_before = get_balance_auth(payer);
      asset storage_fee = get_storage_fee(2);

      addkeyacc(account, key_pub, signed_by_key, extra_pub_key, price_limit, payer_str,
                { permission_level{account, config::active_name}, permission_level{N(prodb), config::active_name} });
      addkeyapp(account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                key_pub, signed_by_key_app, price_limit, payer_str, auths_level);

      auto payer_auth_balance_after = get_balance_auth(payer);
      auto auth_supply_before = asset::from_string(auth_stats_before["supply"].as_string());
      auto auth_supply_after = asset::from_string(auth_stats_after["supply"].as_string());

      auto data = get_authkeys_tbl(account);

      auto ct = control->head_block_time();
      BOOST_REQUIRE_EQUAL(data["owner"].as_string(), account.to_string());
      BOOST_REQUIRE_EQUAL(data["public_key"].as_string(), string(new_key_pub));
      BOOST_REQUIRE_EQUAL(data["not_valid_before"].as_string(), string(ct));
      BOOST_REQUIRE_EQUAL(data["not_valid_after"].as_string(), string(ct + days(360)));
      BOOST_REQUIRE_EQUAL(data["extra_public_key"].as_string(), extra_pub_key);
      BOOST_REQUIRE_EQUAL(data["revoked_at"].as_string(), "0"); // if not revoked == 0
      BOOST_REQUIRE_EQUAL(payer_balance_before - storage_fee, payer_balance_after);
      BOOST_REQUIRE_EQUAL(payer_auth_balance_before.get_amount() - 2 * 10'000, payer_auth_balance_after.get_amount());
      BOOST_REQUIRE_EQUAL(auth_supply_before.get_amount() + 2 * 10'000, auth_supply_after.get_amount());

      // Missing required authority payer
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, "accountnum3", auths_level),
                    missing_auth_exception );
      // character is not in allowed character set for names
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, "dlas*fas.", auths_level),
                    eosio_assert_message_exception );
      // action's authorizing actor 'fail' does not exist
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, payer_str,
                    { permission_level{N(fail), config::active_name} }),
                    transaction_exception );
      // action's authorizing actor "" does not exist
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, payer_str, { permission_level{} } ),
         transaction_exception);
      // expected key different than recovered new key
      BOOST_REQUIRE_THROW(
         addkeyapp( account, get_public_key(N(prodb), "active"), signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, payer_str, auths_level ),
                    eosio_assert_message_exception);
      // expected key different than recovered user key
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    get_public_key(N(prodb)), signed_by_key_app, price_limit, payer_str, auths_level ),
                    eosio_assert_message_exception);
      // overdrawn balance
      BOOST_REQUIRE_THROW(
         addkeyapp( account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                    key_pub, signed_by_key_app, price_limit, payer_str, auths_level ),
                    eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( addkeyapp_require_app_auth_test, rem_auth_tester ) {
   try{
      name account = N(proda);
      name executor = N(prodb);
      vector<permission_level> auths_level = { permission_level{executor, config::active_name} }; // prodb as a executor
      // set account permission rem@code to the rem.auth (allow to execute the action on behalf of the account to rem.auth)
      updateauth(account, N(rem.auth));
      updateauth(executor, N(rem.auth));
      crypto::private_key new_key_priv = crypto::private_key::generate();
      crypto::public_key new_key_pub   = new_key_priv.get_public_key();
      crypto::private_key key_priv     = crypto::private_key::generate();
      crypto::public_key key_pub       = key_priv.get_public_key();
      const auto price_limit           = core_from_string("400.0000");
      string extra_pub_key             = "MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAIZDXel8Nh0xnGOo39XE3Jqdi6iQpxRs\n"
                                         "/r82O1HnpuJFd/jyM3iWInPZvmOnPCP3/Nx4fRNj1y0U9QFnlfefNeECAwEAAQ==";
      string payer_str;

      sha256 digest_addkeyacc = sha256::hash(join( { account.to_string(), string(key_pub), extra_pub_key, payer_str } ));

      sha256 digest_addkeyapp = sha256::hash(join({ account.to_string(), string(new_key_pub), extra_pub_key,
                                                    string(key_pub), payer_str }) );

      auto signed_by_key = key_priv.sign(digest_addkeyacc);
      auto signed_by_new_key_app = new_key_priv.sign(digest_addkeyapp);
      auto signed_by_key_app = key_priv.sign(digest_addkeyapp);

      // tokens to pay for torewards action
      transfer(config::system_account_name, account, core_from_string("1200.0000"), "initial transfer");

      // account has no linked app keys
      BOOST_REQUIRE_THROW(
         addkeyapp(account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                   key_pub, signed_by_key_app, price_limit, payer_str, auths_level),
                   eosio_assert_message_exception);

      // Add new key to account
      addkeyacc(account, key_pub, signed_by_key, extra_pub_key, price_limit, payer_str, { permission_level{account, config::active_name} });

      crypto::private_key nonexistkey_priv = crypto::private_key::generate();
      crypto::public_key nonexistkey_pub   = nonexistkey_priv.get_public_key();
      sha256 nonexist_digest = sha256::hash(join({ account.to_string(), string(new_key_pub), extra_pub_key,
                                                    string(nonexistkey_pub), payer_str }) );

      signed_by_new_key_app = new_key_priv.sign(nonexist_digest);
      auto signed_by_nonexistkey_app = nonexistkey_priv.sign(nonexist_digest);

      // account has no active app keys (nonexist key)
      BOOST_REQUIRE_THROW(
         addkeyapp(account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                   nonexistkey_pub, signed_by_nonexistkey_app, price_limit, payer_str, auths_level),
                   eosio_assert_message_exception);

      // key_lifetime 360 days
      produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(31104000));

      signed_by_new_key_app = new_key_priv.sign(digest_addkeyapp);
      // account has no active app keys (expired key)
      BOOST_REQUIRE_THROW(
         addkeyapp(account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                   key_pub, signed_by_key_app, price_limit, payer_str, auths_level),
                   eosio_assert_message_exception);

      addkeyacc(account, key_pub, signed_by_key, extra_pub_key, price_limit, payer_str, { permission_level{account, config::active_name} });
      revokeacc(account, key_pub, { permission_level{account, config::active_name} });

      // account has no active app keys (revoked)
      BOOST_REQUIRE_THROW(
         addkeyapp(account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                   key_pub, signed_by_key_app, price_limit, payer_str, auths_level),
                   eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( revokedacc_test, rem_auth_tester ) {
   try {
      name account = N(proda);
      vector<permission_level> auths_level = { permission_level{account, config::active_name} };
      // set account permission rem@code to the rem.auth (allow to execute the action on behalf of the account to rem.auth)
      updateauth(account, N(rem.auth));
      crypto::private_key key_priv = crypto::private_key::generate();
      crypto::public_key key_pub   = key_priv.get_public_key();
      const auto price_limit       = core_from_string("400.0000");
      string extra_pub_key         = "MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAIZDXel8Nh0xnGOo39XE3Jqdi6iQpxRs\n"
                                     "/r82O1HnpuJFd/jyM3iWInPZvmOnPCP3/Nx4fRNj1y0U9QFnlfefNeECAwEAAQ==";
      string payer_str;

      sha256 digest = sha256::hash(join( { account.to_string(), string(key_pub), extra_pub_key, payer_str } ));

      auto signed_by_key = key_priv.sign(digest);

      // tokens to pay for torewards action
      transfer(config::system_account_name, account, core_from_string("500.0000"), "initial transfer");

      addkeyacc(account, key_pub, signed_by_key, extra_pub_key, price_limit, payer_str, auths_level);

      time_point ct = control->head_block_time();

      revokeacc(account, key_pub, { permission_level{account, config::active_name} });
      uint32_t revoked_at = control->head_block_time().sec_since_epoch();

      auto data = get_authkeys_tbl(account);

      BOOST_REQUIRE_EQUAL(data["owner"].as_string(), account.to_string());
      BOOST_REQUIRE_EQUAL(data["public_key"].as_string(), string(key_pub));
      BOOST_REQUIRE_EQUAL(data["not_valid_before"].as_string(), string(ct));
      BOOST_REQUIRE_EQUAL(data["not_valid_after"].as_string(), string(ct + days(360)));
      BOOST_REQUIRE_EQUAL(data["extra_public_key"].as_string(), extra_pub_key);
      BOOST_REQUIRE_EQUAL(data["revoked_at"].as_string(), std::to_string(revoked_at) ); // if not revoked == 0

      // Missing required authority account
      BOOST_REQUIRE_THROW(
         revokeacc(account, key_pub, { permission_level{N(prodb), config::active_name} }), missing_auth_exception
      );

      crypto::private_key nonexistkey_priv = crypto::private_key::generate();
      crypto::public_key nonexistkey_pub = key_priv.get_public_key();
      // account has no active app keys
      BOOST_REQUIRE_THROW(
         revokeacc(account, nonexistkey_pub, auths_level), eosio_assert_message_exception
      );
      // account has no active app keys (revoke again same key)
      BOOST_REQUIRE_THROW(
         revokeacc(account, key_pub, auths_level), eosio_assert_message_exception
      );

   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( revokedapp_test, rem_auth_tester ) {
   try {
      name account = N(proda);
      name executor = N(prodb);
      vector<permission_level> auths_level = { permission_level{N(prodb), config::active_name} };
      // set account permission rem@code to the rem.auth (allow to execute the action on behalf of the account to rem.auth)
      updateauth(account, N(rem.auth));
      updateauth(executor, N(rem.auth));
      crypto::private_key revoke_key_priv = crypto::private_key::generate();
      crypto::public_key revoke_key_pub   = revoke_key_priv.get_public_key();
      const auto price_limit              = core_from_string("400.0000");
      string extra_pub_key                = "MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAIZDXel8Nh0xnGOo39XE3Jqdi6iQpxRs\n"
                                            "/r82O1HnpuJFd/jyM3iWInPZvmOnPCP3/Nx4fRNj1y0U9QFnlfefNeECAwEAAQ==";
      string payer_str;

      sha256 addkeyacc_digest = sha256::hash(join( { account.to_string(), string(revoke_key_pub), extra_pub_key, payer_str } ));
      sha256 revokeapp_digest = sha256::hash(join( { account.to_string(), string(revoke_key_pub), string(revoke_key_pub) } ));

      auto signed_by_addkey = revoke_key_priv.sign(addkeyacc_digest);
      auto signed_by_revkey = revoke_key_priv.sign(revokeapp_digest);

      // tokens to pay for torewards action
      transfer(config::system_account_name, account, core_from_string("1000.0000"), "initial transfer");

      addkeyacc(account, revoke_key_pub, signed_by_addkey, extra_pub_key, price_limit, payer_str, { permission_level{account, config::active_name} });
      time_point ct = control->head_block_time();

      // revoke key and sign digest by revoke key
      revokeapp(account, revoke_key_pub, revoke_key_pub, signed_by_revkey, auths_level);
      uint32_t revoked_at = control->head_block_time().sec_since_epoch();

      auto data = get_authkeys_tbl(account);

      BOOST_REQUIRE_EQUAL(data["owner"].as_string(), account.to_string());
      BOOST_REQUIRE_EQUAL(data["public_key"].as_string(), string(revoke_key_pub));
      BOOST_REQUIRE_EQUAL(data["not_valid_before"].as_string(), string(ct));
      BOOST_REQUIRE_EQUAL(data["not_valid_after"].as_string(), string(ct + days(360)));
      BOOST_REQUIRE_EQUAL(data["extra_public_key"].as_string(), extra_pub_key);
      BOOST_REQUIRE_EQUAL(data["revoked_at"].as_string(), std::to_string(revoked_at) ); // if not revoked == 0

      // add new key to test revokeapp throw
      addkeyacc(account, revoke_key_pub, signed_by_addkey, extra_pub_key, price_limit, payer_str,
                { permission_level{account, config::active_name} });

      // action's authorizing actor "" does not exist
      BOOST_REQUIRE_THROW(
         revokeapp(account, revoke_key_pub, revoke_key_pub,signed_by_revkey, { permission_level{} }), transaction_exception
      );
      // expected key different than recovered user key
      crypto::private_key nonexistkey_priv = crypto::private_key::generate();
      crypto::public_key nonexistkey_pub = nonexistkey_priv.get_public_key();
      BOOST_REQUIRE_THROW(
         revokeapp(account, nonexistkey_pub, revoke_key_pub, signed_by_revkey, auths_level), eosio_assert_message_exception
      );
      // account has no active app keys (revoke nonexist key)
      revokeapp_digest = sha256::hash(join( { account.to_string(), string(nonexistkey_pub), string(revoke_key_pub) }));
      signed_by_revkey = revoke_key_priv.sign(revokeapp_digest);
      BOOST_REQUIRE_THROW(
         revokeapp(account, nonexistkey_pub, revoke_key_pub, signed_by_revkey, auths_level), eosio_assert_message_exception
      );
      // account has no active app keys (revoke again same key)
      revokeapp_digest = sha256::hash(join( { account.to_string(), string(revoke_key_pub), string(revoke_key_pub) } ));
      signed_by_revkey = revoke_key_priv.sign(revokeapp_digest);
      revokeapp(account, revoke_key_pub, revoke_key_pub, signed_by_revkey, auths_level);
      BOOST_REQUIRE_THROW(
         revokeapp(account, revoke_key_pub, revoke_key_pub, signed_by_revkey, auths_level), eosio_assert_message_exception
      );
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( revokedapp_and_sign_by_another_key_test, rem_auth_tester ) {
   try {
      name account = N(proda);
      vector<permission_level> auths_level = { permission_level{N(prodb), config::active_name} };
      // set account permission rem@code to the rem.auth (allow to execute the action on behalf of the account to rem.auth)
      updateauth(account, N(rem.auth));
      crypto::private_key key_priv = crypto::private_key::generate();
      crypto::public_key key_pub = key_priv.get_public_key();
      crypto::private_key revoke_key_priv = crypto::private_key::generate();
      crypto::public_key revoke_key_pub   = revoke_key_priv.get_public_key();
      const auto price_limit              = core_from_string("400.0000");
      string extra_pub_key                = "MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAIZDXel8Nh0xnGOo39XE3Jqdi6iQpxRs\n"
                                            "/r82O1HnpuJFd/jyM3iWInPZvmOnPCP3/Nx4fRNj1y0U9QFnlfefNeECAwEAAQ==";
      string payer_str;

      sha256 addkeyacc_digest = sha256::hash(join( { account.to_string(), string(key_pub), extra_pub_key, payer_str } ));
      sha256 revokeapp_digest = sha256::hash(join( { account.to_string(), string(revoke_key_pub), string(key_pub) } ));

      auto signed_by_addkey = key_priv.sign(addkeyacc_digest);
      auto signed_by_revkey = key_priv.sign(revokeapp_digest);

      // tokens to pay for torewards action
      transfer(config::system_account_name, account, core_from_string("1000.0000"), "initial transfer");

      // Add key_pub
      addkeyacc(account, key_pub, signed_by_addkey, extra_pub_key, price_limit, payer_str, { permission_level{account, config::active_name} });
      // Add revoke_key_pub
      addkeyacc_digest = sha256::hash(join( { account.to_string(), string(revoke_key_pub), extra_pub_key, payer_str } ));
      signed_by_addkey = revoke_key_priv.sign(addkeyacc_digest);
      produce_blocks();
      addkeyacc(account, revoke_key_pub, signed_by_addkey, extra_pub_key, price_limit, payer_str,
                { permission_level{account, config::active_name} });
      time_point ct = control->head_block_time();

      // revoke key and sign digest by key
      revokeapp(account, revoke_key_pub, key_pub, signed_by_revkey, auths_level);
      uint32_t revoked_at = control->head_block_time().sec_since_epoch();

      auto data = get_authkeys_tbl(account);

      BOOST_REQUIRE_EQUAL(data["owner"].as_string(), account.to_string());
      BOOST_REQUIRE_EQUAL(data["public_key"].as_string(), string(revoke_key_pub));
      BOOST_REQUIRE_EQUAL(data["not_valid_before"].as_string(), string(ct));
      BOOST_REQUIRE_EQUAL(data["not_valid_after"].as_string(), string(ct + days(360)));
      BOOST_REQUIRE_EQUAL(data["extra_public_key"].as_string(), extra_pub_key);
      BOOST_REQUIRE_EQUAL(data["revoked_at"].as_string(), std::to_string(revoked_at) ); // if not revoked == 0
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( revoke_require_app_auth_test, rem_auth_tester ) {
   try {
      name account  = N(proda);
      name executor = N(prodb);
      vector<permission_level> auths_level = { permission_level{N(prodb), config::active_name} }; // prodb as a executor
      // set account permission rem@code to the rem.auth (allow to execute the action on behalf of the account to rem.auth)
      updateauth(account, N(rem.auth));
      updateauth(executor, N(rem.auth));
      crypto::private_key new_key_priv = crypto::private_key::generate();
      crypto::public_key new_key_pub   = new_key_priv.get_public_key();
      crypto::private_key key_priv     = crypto::private_key::generate();
      crypto::public_key key_pub       = key_priv.get_public_key();
      const auto price_limit           = core_from_string("400.0000");
      string extra_pub_key             = "MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAIZDXel8Nh0xnGOo39XE3Jqdi6iQpxRs\n"
                                         "/r82O1HnpuJFd/jyM3iWInPZvmOnPCP3/Nx4fRNj1y0U9QFnlfefNeECAwEAAQ==";
      string payer_str;

      sha256 digest_addkeyacc = sha256::hash(join( { account.to_string(), string(key_pub), extra_pub_key, payer_str } ));

      sha256 digest_addkeyapp = sha256::hash(join({ account.to_string(), string(new_key_pub), extra_pub_key,
                                                    string(key_pub), payer_str }) );

      auto signed_by_key = key_priv.sign(digest_addkeyacc);
      auto signed_by_new_key_app = new_key_priv.sign(digest_addkeyapp);
      auto signed_by_key_app = key_priv.sign(digest_addkeyapp);

      // tokens to pay for torewards action
      transfer(config::system_account_name, account, core_from_string("1000.0000"), "initial transfer");

      // account has no linked app keys
      BOOST_REQUIRE_THROW(
         addkeyapp(account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                   key_pub, signed_by_key_app, price_limit,payer_str, auths_level),
                   eosio_assert_message_exception);

      // Add new key to account
      addkeyacc(account, key_pub, signed_by_key, extra_pub_key, price_limit, payer_str,
                { permission_level{account, config::active_name} });

      crypto::private_key nonexistkey_priv = crypto::private_key::generate();
      crypto::public_key nonexistkey_pub = nonexistkey_priv.get_public_key();
      sha256 nonexist_digest = sha256::hash(join({ account.to_string(), string(new_key_pub), extra_pub_key,
                                                   string(nonexistkey_pub), payer_str }) );

      signed_by_new_key_app = new_key_priv.sign(nonexist_digest);
      auto signed_by_nonexistkey_app = nonexistkey_priv.sign(nonexist_digest);

      // account has no active app keys (nonexist key)
      BOOST_REQUIRE_THROW(
         addkeyapp(account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                   nonexistkey_pub, signed_by_nonexistkey_app, price_limit, payer_str, auths_level),
                   eosio_assert_message_exception);

      // key_lifetime 360 days
      produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(31104000));

      signed_by_new_key_app = new_key_priv.sign(digest_addkeyapp);
      // account has no active app keys (expired key)
      BOOST_REQUIRE_THROW(
         addkeyapp(account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                   key_pub, signed_by_key_app, price_limit, payer_str, auths_level),
                   eosio_assert_message_exception);

      addkeyacc(account, key_pub, signed_by_key, extra_pub_key, price_limit, payer_str, { permission_level{account, config::active_name} });
      revokeacc(account, key_pub, { permission_level{account, config::active_name} });

      // account has no active app keys (revoked)
      BOOST_REQUIRE_THROW(
         addkeyapp(account, new_key_pub, signed_by_new_key_app, extra_pub_key,
                   key_pub, signed_by_key_app, price_limit, payer_str, auths_level),
                   eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( buyauth_tests, rem_auth_tester ) {
   try {
      name account = N(prodb);
      vector<permission_level> auths_level = { permission_level{account, config::active_name} };
      transfer(config::system_account_name, account, core_from_string("5000.0000"), "initial transfer");
      updateauth(N(prodb), N(rem.auth));

      auto account_balance_before = get_balance(account);

      buyauth(account, auth_from_string("1.2345"), 1, auths_level);

      auto account_auth_balance = get_balance_auth(account);
      auto account_balance_after = get_balance(account);
      auto storage_fee = get_storage_fee(1.2345);

      BOOST_REQUIRE_EQUAL(account_auth_balance, auth_from_string("1.2345"));
      BOOST_REQUIRE_EQUAL(account_balance_before - storage_fee, account_balance_after);

      // quantity should be a positive value
      BOOST_REQUIRE_THROW(buyauth(account, auth_from_string("-1.2345"), 1, auths_level), eosio_assert_message_exception);
      // maximum price should be a positive value
      BOOST_REQUIRE_THROW(buyauth(account, auth_from_string("1.2345"), -1, auths_level), eosio_assert_message_exception);
      // symbol precision mismatch
      BOOST_REQUIRE_THROW(buyauth(account, auth_from_string("1.23450"), 1, auths_level), eosio_assert_message_exception);
      // currently rem-usd price is above maximum price
      BOOST_REQUIRE_THROW(buyauth(account, auth_from_string("1.2345"), 0.00001, auths_level), eosio_assert_message_exception);
      // missing authority of account
      BOOST_REQUIRE_THROW(buyauth(account, auth_from_string("1.2345"), 1, { permission_level{N(proda), config::active_name} }), missing_auth_exception);
      // overdrawn balance
      BOOST_REQUIRE_THROW(buyauth(account, auth_from_string("100000.23450"), 1, auths_level), eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
