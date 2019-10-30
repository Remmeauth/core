/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/abi_serializer.hpp>
#include <fc/crypto/signature.hpp>
#include <eosio/testing/tester.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

#include <boost/test/unit_test.hpp>

#include <contracts.hpp>

#include <chrono>
#include <functional>
#include <thread>
#include <numeric>
#include <set>

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

const char *SYMBOL_CORE_NAME = "REM";
symbol CORE_SYMBOL_STR(4, SYMBOL_CORE_NAME);
const string REMCHAIN_ID = "93ece941df27a5787a405383a66a7c26d04e80182adf504365710331ac0625a7";

asset _core_from_string(const string &s) {
   return asset::from_string(s + " " + SYMBOL_CORE_NAME);
}

const auto _producer_candidates = {
      N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
      N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
      N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ)
};

struct init_data {
   name rampayer = N(proda);
   string txid = "79b9563d89da12715c2ea086b38a5557a521399c87d40d84b8fa5df0fd478046";
   string swap_pubkey;
   asset quantity = _core_from_string("201.0000");
   string return_address = "9f21f19180c8692ebaa061fd231cd1b029ff2326";
   string return_chain_id = "ethropsten";
   block_timestamp_type swap_timestamp;
};

class swap_tester : public TESTER {
public:
   swap_tester();

   void deploy_contract(bool call_init = true) {
      set_code(config::system_account_name, contracts::rem_system_wasm());
      set_abi(config::system_account_name, contracts::rem_system_abi().data());
      if (call_init) {
         base_tester::push_action(config::system_account_name, N(init),
                                  config::system_account_name, mutable_variant_object()
                                     ("version", 0)
                                     ("core", CORE_SYMBOL_STR)
         );
      }
   }

   auto delegate_bandwidth(name from, name receiver, asset stake_quantity, uint8_t transfer = 1) {
      auto r = base_tester::push_action(config::system_account_name, N(delegatebw), from, mvo()
         ("from", from)
         ("receiver", receiver)
         ("stake_quantity", stake_quantity)
         ("transfer", transfer)
      );
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

   void create_currency(name contract, name manager, asset maxsupply, const private_key_type *signer = nullptr) {
      auto act = mutable_variant_object()
         ("issuer", manager)
         ("maximum_supply", maxsupply);

      base_tester::push_action(contract, N(create), contract, act);
   }

   auto issue(name contract, name manager, name to, asset amount) {
      auto r = base_tester::push_action(contract, N(issue), manager, mutable_variant_object()
         ("to", to)
         ("quantity", amount)
         ("memo", "")
      );
      produce_block();
      return r;
   }

   auto set_privileged(name account) {
      auto r = base_tester::push_action(config::system_account_name, N(setpriv), config::system_account_name,
                                        mvo()("account", account)("is_priv", 1));
      produce_block();
      return r;
   }

   auto init_swap(const name &rampayer, const string &txid, const string &swap_pubkey,
                  const asset &quantity, const string &return_address, const string &return_chain_id,
                  const block_timestamp_type &swap_timestamp) {

      auto r = base_tester::push_action(N(rem.swap), N(init), rampayer, mvo()
         ("rampayer", rampayer)
         ("txid", txid)
         ("swap_pubkey", swap_pubkey)
         ("quantity", quantity)
         ("return_address", return_address)
         ("return_chain_id", return_chain_id)
         ("swap_timestamp", swap_timestamp)
      );
      produce_block();
      return r;
   }

   auto cancel_swap(const name &rampayer, const string &txid, const string &swap_pubkey,
                    const asset &quantity, const string &return_address, const string &return_chain_id,
                    const block_timestamp_type &swap_timestamp) {

      auto r = base_tester::push_action(N(rem.swap), N(cancel), rampayer, mvo()
         ("rampayer", name(rampayer))
         ("txid", txid)
         ("swap_pubkey_str", swap_pubkey)
         ("quantity", quantity)
         ("return_address", return_address)
         ("return_chain_id", return_chain_id)
         ("swap_timestamp", swap_timestamp)
      );
      produce_block();
      return r;
   }

   auto finish_swap(const name &rampayer, const name &receiver, const string &txid, const string &swap_pubkey,
                    const asset &quantity, const string &return_address, const string &return_chain_id,
                    const block_timestamp_type &swap_timestamp, const crypto::signature &sign) {

      auto r = base_tester::push_action(N(rem.swap), N(finish), rampayer, mvo()
         ("rampayer", name(rampayer))
         ("receiver", name(receiver))
         ("txid", txid)
         ("swap_pubkey_str", swap_pubkey)
         ("quantity", quantity)
         ("return_address", return_address)
         ("return_chain_id", return_chain_id)
         ("swap_timestamp", swap_timestamp)
         ("sign", sign)
      );
      produce_block();
      return r;
   }

   auto finish_swap_new_account(const name &rampayer, const name &receiver, const string &owner_pubkey_str,
                                const string &active_pubkey_str, const string &txid, const string &swap_pubkey,
                                const asset &quantity, const string &return_address, const string &return_chain_id,
                                const block_timestamp_type &swap_timestamp, const crypto::signature &sign) {

      auto r = base_tester::push_action(N(rem.swap), N(finishnewacc), rampayer, mvo()
         ("rampayer", name(rampayer))
         ("receiver", name(receiver))
         ("owner_pubkey_str", owner_pubkey_str)
         ("active_pubkey_str", active_pubkey_str)
         ("txid", txid)
         ("swap_pubkey_str", swap_pubkey)
         ("quantity", quantity)
         ("return_address", return_address)
         ("return_chain_id", return_chain_id)
         ("swap_timestamp", swap_timestamp)
         ("sign", sign)
      );
      produce_block();
      return r;
   }

   auto addchain(const name& rampayer, const name &chain_id, const bool &input, const bool& output) {

      auto r = base_tester::push_action(N(rem.swap), N(addchain), rampayer, mvo()
         ("chain_id", chain_id)
         ("input", input)
         ("output", output)
      );
      produce_block();
      return r;
   }

   auto setswapfee(const int64_t &amount) {
      vector<permission_level> level = {{N(rem.swap), config::active_name}, {N(rem), config::active_name}};
      auto r = base_tester::push_action(N(rem.swap), N(setswapfee), level, mvo()
         ("amount", amount)
      );
      produce_block();
      return r;
   }

   auto setminswpout(const int64_t &amount) {
      vector<permission_level> level = {{N(rem.swap), config::active_name}, {N(rem), config::active_name}};
      auto r = base_tester::push_action(N(rem.swap), N(setminswpout), level, mvo()
         ("amount", amount)
      );
      produce_block();
      return r;
   }

   auto setchainid(const string &chain_id) {
      vector<permission_level> level = {{N(rem.swap), config::active_name}, {N(rem), config::active_name}};
      auto r = base_tester::push_action(N(rem.swap), N(setchainid), level, mvo()
         ("chain_id", chain_id)
      );
      produce_block();
      return r;
   }

   auto register_producer(name producer) {
      auto r = base_tester::push_action(config::system_account_name, N(regproducer), producer, mvo()
         ("producer", name(producer))
         ("producer_key", get_public_key(producer, "active"))
         ("url", "")
         ("location", 0)
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

   auto updateauth(const name &account) {
      auto auth = authority(get_public_key(account, "active"));
      auth.accounts.push_back(permission_level_weight{{account, config::rem_code_name}, 1});

      auto r = base_tester::push_action(N(rem), N(updateauth), account, mvo()
         ("account", account.to_string())
         ("permission", "active")
         ("parent", "owner")
         ("auth", auth)
      );
      produce_blocks();
      return r;
   }

   variant get_singtable(const name& contract, const name &table, const string &type) {
      vector<char> data;
      const auto &db = control->db();
      const auto *t_id = db.find<table_id_object, by_code_scope_table>(
         boost::make_tuple(contract, contract, table));
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

   asset get_balance(const account_name &act) {
      return get_currency_balance(N(rem.token), CORE_SYMBOL_STR, act);
   }

   asset get_swap_fee() {
      auto swap_fee = get_singtable(N(rem.swap), N(swapparams), "swapparams")["in_swap_fee"];
      return asset(swap_fee.as_int64(), CORE_SYMBOL_STR);
   }

   variant get_supported_chain(const name& chain) {
      vector<char> data = get_row_by_account( N(rem.swap), N(rem.swap), N(chains), chain );
      return data.empty() ? variant() : abi_ser.binary_to_variant("chains", data, abi_serializer_max_time);
   }

   asset get_min_account_stake() {
      asset min_account_stake;
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(global), N(global) );
      from_variant(abi_ser.binary_to_variant( "eosio_global_state", data, abi_serializer_max_time )["min_account_stake"],
                   min_account_stake);
      return min_account_stake;
   }

   string get_pubkey_str(const crypto::private_key& priv_key) {
      crypto::public_key pub_key = priv_key.get_public_key();
      return string(pub_key);
   }

   void set_code_abi(const account_name &account, const vector<uint8_t> &wasm, const char *abi,
                     const private_key_type *signer = nullptr) {
      wdump((account));
      set_code(account, wasm, signer);
      set_abi(account, abi, signer);
      if (account == N(rem.swap)) {
         const auto &accnt = control->db().get<account_object, by_name>(account);
         abi_def abi_definition;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi_definition), true);
         abi_ser.set_abi(abi_definition, abi_serializer_max_time);
      }
      produce_blocks();
   }

   string join(vector<string> &&vec, string delim = string("*")) {
      return std::accumulate(std::next(vec.begin()), vec.end(), vec[0],
                             [&delim](string &a, string &b) {
                                return a + delim + b;
                             });
   }

   abi_serializer abi_ser;
};

swap_tester::swap_tester() {
   // Create rem.msig, rem.token and rem.swap
   create_accounts(
      {N(rem.msig), N(rem.token), N(rem.rex), N(rem.ram), N(rem.ramfee), N(rem.stake), N(rem.bpay), N(rem.spay), N(rem.vpay),
       N(rem.saving), N(rem.swap)});

   // Set code for the following accounts:
   //  - rem (code: rem.bios) (already set by tester constructor)
   //  - rem.msig (code: rem.msig)
   //  - rem.token (code: rem.token)
   //  - rem.swap (code: rem.swap)
   //  - rem.utils (code: rem.utils)
   set_code_abi(N(rem.msig),
                contracts::rem_msig_wasm(),
                contracts::rem_msig_abi().data());//, &rem_active_pk);
   set_code_abi(N(rem.token),
                contracts::rem_token_wasm(),
                contracts::rem_token_abi().data()); //, &rem_active_pk);
   set_code_abi(N(rem.swap),
                contracts::rem_swap_wasm(),
                contracts::rem_swap_abi().data()); //, &rem_active_pk);
//   set_code_abi(N(rem.utils),
//                contracts::rem_utils_wasm(),
//                contracts::rem_utils_abi().data()); //, &rem_active_pk);

   // Set privileged for rem.msig, rem.token, rem.swap
   set_privileged(N(rem.msig));
   set_privileged(N(rem.token));
   set_privileged(N(rem.swap));
//   set_privileged(N(rem.utils));

   // Verify rem.msig, rem.token, rem.swap is privileged
   const auto &rem_msig_acc = get<account_metadata_object, by_name>(N(rem.msig));
   BOOST_TEST(rem_msig_acc.is_privileged() == true);
   const auto &rem_token_acc = get<account_metadata_object, by_name>(N(rem.token));
   BOOST_TEST(rem_token_acc.is_privileged() == true);
   const auto &rem_swap_acc = get<account_metadata_object, by_name>(N(rem.swap));
   BOOST_TEST(rem_swap_acc.is_privileged() == true);
//   const auto &rem_utils_acc = get<account_metadata_object, by_name>(N(rem.utils));
//   BOOST_TEST(rem_utils_acc.is_privileged() == true);

   // Create REM tokens in rem.token, set its manager as rem.swap
   const auto max_supply = _core_from_string("1000000000.0000"); /// 10x larger than 1B initial tokens
   const auto initial_supply = _core_from_string("100000000.0000");  /// 10x larger than 1B initial tokens

   create_currency(N(rem.token), N(rem.swap), max_supply);
   // Issue the genesis supply of 1 billion REM tokens to rem.swap
   issue(N(rem.token), N(rem.swap), N(rem.swap), initial_supply);

   auto actual = get_balance(N(rem.swap));

   BOOST_REQUIRE_EQUAL(initial_supply, actual);

   struct rem_genesis_account {
      account_name name;
      uint64_t initial_balance;
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
   for (const auto &account : genesis_test) {
      create_account(account.name, N(rem.swap));
   }

   deploy_contract();

   // Buy ram and stake cpu and net for each genesis accounts
   for (const auto &account : genesis_test) {
      const auto stake_quantity = account.initial_balance - 1000;
      const auto r = delegate_bandwidth(N(rem.stake), account.name, asset(stake_quantity, CORE_SYMBOL_STR));
      BOOST_REQUIRE(!r->except_ptr);
   }

   // Register producers
   for (const auto &producer : _producer_candidates) {
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
   // set permission @rem.code to rem.swap
   updateauth(N(rem.swap));
   // add supported chain
   addchain(N(rem.swap), N(ethropsten), true, true);
   setswapfee(500000);
   setchainid(REMCHAIN_ID);
}

BOOST_AUTO_TEST_SUITE(swap_tests)

BOOST_FIXTURE_TEST_CASE(init_swap_test, swap_tester) {
   try {
      init_data init_swap_data = {
         .swap_pubkey = get_pubkey_str(crypto::private_key::generate()),
         .swap_timestamp = time_point_sec(control->head_block_time())
      };
      /* swap id consist of swap_pubkey, txid, remchain_id, quantity, return_address,
       * return_chain_id, swap_timepoint_seconds and separated by '*'. */
      time_point swap_timepoint = init_swap_data.swap_timestamp.to_time_point();
      string swap_payload = join({ init_swap_data.swap_pubkey.substr(3), init_swap_data.txid, REMCHAIN_ID,
                                  init_swap_data.quantity.to_string(), init_swap_data.return_address,
                                  init_swap_data.return_chain_id, std::to_string(swap_timepoint.sec_since_epoch()) });

      string swap_id = sha256::hash(swap_payload);
      asset before_init_balance = get_balance(N(rem.swap));

      init_swap(N(rem.swap), init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp);

      // 0 if a swap status initialized
      auto data = get_singtable(N(rem.swap), N(swaps), "swap_data");
      BOOST_REQUIRE_EQUAL("0", data["status"].as_string());

      // 21 approvals, tokens will be issued after 2/3 + 1 approvals
      for (const auto &producer : _producer_candidates) {
         init_swap(producer, init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                   init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp);
      }

      asset after_init_balance = get_balance(N(rem.swap));

      data = get_singtable(N(rem.swap), N(swaps), "swap_data");
      BOOST_REQUIRE_EQUAL(init_swap_data.txid, data["txid"].as_string());

      BOOST_REQUIRE_EQUAL(swap_id, data["swap_id"].as_string());
      BOOST_REQUIRE_EQUAL(string(swap_timepoint), data["swap_timestamp"].as_string());
      // 1 if a swap status issued
      BOOST_REQUIRE_EQUAL("1", data["status"].as_string());
      // after issue tokens, balance rem.swap should be a before issue balance + amount of tokens to be a swapped
      BOOST_REQUIRE_EQUAL(before_init_balance + init_swap_data.quantity, after_init_balance);
      // default producers reward
//      BOOST_REQUIRE_EQUAL(get_swap_fee(), _core_from_string("50.0000"));
      auto supported_chains_data = get_supported_chain(N(ethropsten));
      BOOST_REQUIRE_EQUAL(supported_chains_data["chain"].as_string(), "ethropsten");
      BOOST_REQUIRE_EQUAL(supported_chains_data["input"].as_string(), "1");
      BOOST_REQUIRE_EQUAL(supported_chains_data["output"].as_string(), "1");

      // action's authorizing actor 'fail' does not exist
      BOOST_REQUIRE_THROW(
         init_swap(N(fail), init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                   init_swap_data.return_address, init_swap_data.return_chain_id,
                   init_swap_data.swap_timestamp), transaction_exception);
      // approval already exists
      BOOST_REQUIRE_THROW(init_swap(init_swap_data.rampayer, init_swap_data.txid, init_swap_data.swap_pubkey,
                                    init_swap_data.quantity,
                                    init_swap_data.return_address, init_swap_data.return_chain_id,
                                    init_swap_data.swap_timestamp), eosio_assert_message_exception);
      // symbol precision mismatch
      BOOST_REQUIRE_THROW(init_swap(init_swap_data.rampayer, init_swap_data.txid, init_swap_data.swap_pubkey,
                                    asset::from_string("201.0000 SYS"), init_swap_data.return_address,
                                    init_swap_data.return_chain_id, init_swap_data.swap_timestamp),
                                    eosio_assert_message_exception);
      // the quantity must be greater than the swap fee
      BOOST_REQUIRE_THROW(init_swap(init_swap_data.rampayer, init_swap_data.txid, init_swap_data.swap_pubkey,
                                    _core_from_string("25.0000"), init_swap_data.return_address,
                                    init_swap_data.return_chain_id, init_swap_data.swap_timestamp),
                                    eosio_assert_message_exception);
      // swap lifetime expired
      BOOST_REQUIRE_THROW(init_swap(init_swap_data.rampayer, init_swap_data.txid, init_swap_data.swap_pubkey,
                                    init_swap_data.quantity,
                                    init_swap_data.return_address, init_swap_data.return_chain_id,
                                    time_point_sec::from_iso_string("2019-01-13T18:09:16.000")),
                                    eosio_assert_message_exception);
      // swap cannot be initialized with a future timestamp
      BOOST_REQUIRE_THROW(init_swap(init_swap_data.rampayer, init_swap_data.txid, init_swap_data.swap_pubkey,
                                    init_swap_data.quantity, init_swap_data.return_address,
                                    init_swap_data.return_chain_id,
                                    time_point_sec::from_iso_string("2020-01-13T18:09:16.000")),
                                    eosio_assert_message_exception);
      // block producer authorization required
      BOOST_REQUIRE_THROW(
         init_swap(N(rem.swap), init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                   init_swap_data.return_address, init_swap_data.return_chain_id,
                   init_swap_data.swap_timestamp), eosio_assert_message_exception);
         // TODO: uncomment this when rem.utils will be merged
//      // not supported chain id
//      BOOST_REQUIRE_THROW(
//         init_swap(N(rem.swap), init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
//                   init_swap_data.return_address, " ",
//                   init_swap_data.swap_timestamp), eosio_assert_message_exception);
//      // not supported address
//      BOOST_REQUIRE_THROW(
//         init_swap(N(rem.swap), init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
//                   " ", init_swap_data.return_chain_id,
//                   init_swap_data.swap_timestamp), eosio_assert_message_exception);
//      // not supported address
//      BOOST_REQUIRE_THROW(
//         init_swap(N(rem.swap), init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
//                   "/&%$#!@)(|<>_-+=", init_swap_data.return_chain_id,
//                   init_swap_data.swap_timestamp), eosio_assert_message_exception);
//      // not supported address
//      BOOST_REQUIRE_THROW(
//         init_swap(N(rem.swap), init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
//                   init_swap_data.return_address, "/&%$#!@)(|<>_-+=",
//                   init_swap_data.swap_timestamp), eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(init_swap_after_cancel_test, swap_tester) {
   try {
      init_data init_swap_data = {
         .swap_pubkey = get_pubkey_str(crypto::private_key::generate()),
         // swap can be canceled after expiration (1 week)
         .swap_timestamp = time_point_sec::from_iso_string("2019-12-05T00:00:43.000")
      };
      for (const auto &producer : _producer_candidates) {
         init_swap(producer, init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                   init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp);
      }
      cancel_swap(N(rem.swap), init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                  init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp);

      // swap already canceled
      BOOST_REQUIRE_THROW(init_swap(N(proda), init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                                   init_swap_data.return_address, init_swap_data.return_chain_id,
                                   init_swap_data.swap_timestamp), eosio_assert_message_exception);

   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(finish_swap_test, swap_tester) {
   try {
      name receiver = N(prodc);
      crypto::private_key swap_key_priv = crypto::private_key::generate();
      init_data init_swap_data = {
         .swap_pubkey = get_pubkey_str(swap_key_priv),
         .swap_timestamp = time_point_sec(control->head_block_time())
      };
      time_point swap_timepoint = init_swap_data.swap_timestamp.to_time_point();
      string swap_payload = join({init_swap_data.swap_pubkey.substr(3), init_swap_data.txid, REMCHAIN_ID,
                                  init_swap_data.quantity.to_string(), init_swap_data.return_address,
                                  init_swap_data.return_chain_id, std::to_string(swap_timepoint.sec_since_epoch())});

      string swap_id = sha256::hash(swap_payload);

      for (const auto &producer : _producer_candidates) {
         init_swap(producer, init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                   init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp);
      }

      string sign_payload = join(
         { receiver.to_string(), init_swap_data.txid, REMCHAIN_ID, init_swap_data.quantity.to_string(),
           init_swap_data.return_address, init_swap_data.return_chain_id,
           std::to_string(swap_timepoint.sec_since_epoch()) });

      auto swap_digest = sha256::hash(sign_payload);
      auto sign = swap_key_priv.sign(swap_digest);

      auto receiver_before_balance = get_balance(receiver);
      auto remswap_before_balance = get_balance(N(rem.swap));
      asset producers_reward = get_swap_fee();

      finish_swap(init_swap_data.rampayer, receiver, init_swap_data.txid, init_swap_data.swap_pubkey,
                  init_swap_data.quantity, init_swap_data.return_address, init_swap_data.return_chain_id,
                  init_swap_data.swap_timestamp, sign);

      auto receiver_after_balance = get_balance(receiver);
      auto remswap_after_balance = get_balance(N(rem.swap));

      auto data = get_singtable(N(rem.swap), N(swaps), "swap_data");

      // table equal
      BOOST_REQUIRE_EQUAL(swap_id, data["swap_id"].as_string());
      // 2 if swap status finish
      BOOST_REQUIRE_EQUAL("2", data["status"].as_string());
      BOOST_REQUIRE_EQUAL(string(swap_timepoint), data["swap_timestamp"].as_string());
      // amount of provided approvals must be a 2/3 + 1 of active producers
      uint32_t majority = (_producer_candidates.size() * 2 / 3) + 1;
      BOOST_TEST(majority <= data["provided_approvals"].get_array().size());
      // balance equal : receiver balance += swapped quantity - producers_reward
      BOOST_REQUIRE_EQUAL(receiver_before_balance + init_swap_data.quantity - producers_reward,
                          receiver_after_balance);
      // balance equal : swap contract -= swapped quantity
      BOOST_REQUIRE_EQUAL(remswap_before_balance - init_swap_data.quantity, remswap_after_balance);

      // swap already finished
      BOOST_REQUIRE_THROW(
         finish_swap(init_swap_data.rampayer, receiver, init_swap_data.txid, init_swap_data.swap_pubkey,
                     init_swap_data.quantity, init_swap_data.return_address, init_swap_data.return_chain_id,
                     init_swap_data.swap_timestamp, sign),
                     eosio_assert_message_exception);
      // swap doesn't exist
      BOOST_REQUIRE_THROW(
         finish_swap(init_swap_data.rampayer, receiver, init_swap_data.txid, init_swap_data.swap_pubkey,
                     _core_from_string("500.0000"), init_swap_data.return_address,
                     init_swap_data.return_chain_id, init_swap_data.swap_timestamp, sign),
                     eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(finish_expired_swap_test, swap_tester) {
   try {
      name receiver = N(prodc);
      crypto::private_key swap_key_priv = crypto::private_key::generate();
      init_data init_swap_data = {
         .swap_pubkey = get_pubkey_str(swap_key_priv),
         .swap_timestamp = time_point_sec::from_iso_string("2019-12-01T00:00:00.000")
      };
      time_point swap_timepoint = init_swap_data.swap_timestamp.to_time_point();

      for (const auto &producer : _producer_candidates) {
         init_swap(producer, init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                   init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp);
      }

      string sign_payload = join(
         { receiver.to_string(), init_swap_data.txid, REMCHAIN_ID, init_swap_data.quantity.to_string(),
           init_swap_data.return_address, init_swap_data.return_chain_id,
           std::to_string(swap_timepoint.sec_since_epoch()) });

      auto swap_digest = sha256::hash(sign_payload);
      auto sign = swap_key_priv.sign(swap_digest);

      // swap has to be canceled after expiration
      BOOST_REQUIRE_THROW(
         finish_swap(init_swap_data.rampayer, receiver, init_swap_data.txid, init_swap_data.swap_pubkey,
                     init_swap_data.quantity, init_swap_data.return_address, init_swap_data.return_chain_id,
                     init_swap_data.swap_timestamp, sign),
                     eosio_assert_message_exception);

      // swap lifetime expired if a swap_timeswamp > 180 days
      block_timestamp_type swap_expired_timestamp = time_point_sec::from_iso_string("2019-12-25T00:00:44.500");
      for (const auto &producer : _producer_candidates) {
         init_swap(producer, init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                   init_swap_data.return_address, init_swap_data.return_chain_id, swap_expired_timestamp);
      }
      swap_timepoint = swap_expired_timestamp.to_time_point();

      sign_payload = join(
         { receiver.to_string(), init_swap_data.txid, REMCHAIN_ID, init_swap_data.quantity.to_string(),
           init_swap_data.return_address, init_swap_data.return_chain_id,
           std::to_string(swap_timepoint.sec_since_epoch()) });

      swap_digest = sha256::hash(sign_payload);
      sign = swap_key_priv.sign(swap_digest);

      // swap lifetime expired
      produce_blocks(10);
      BOOST_REQUIRE_THROW(
         finish_swap(init_swap_data.rampayer, receiver, init_swap_data.txid, init_swap_data.swap_pubkey,
                     init_swap_data.quantity, init_swap_data.return_address, init_swap_data.return_chain_id,
                     swap_expired_timestamp, sign),
                     eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(finish_not_confirmed_swap_test, swap_tester) {
   try {
      name receiver = N(prodc);
      crypto::private_key swap_key_priv = crypto::private_key::generate();
      init_data init_swap_data = {
         .swap_pubkey = get_pubkey_str(swap_key_priv),
         // swap can be canceled after expiration (1 week)
         .swap_timestamp = time_point_sec::from_iso_string("2019-12-01T00:00:00.000")
      };
      time_point swap_timepoint = init_swap_data.swap_timestamp.to_time_point();

      init_swap(N(proda), init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp);

      string sign_payload = join(
         { receiver.to_string(), init_swap_data.txid, REMCHAIN_ID, init_swap_data.quantity.to_string(),
           init_swap_data.return_address, init_swap_data.return_chain_id,
           std::to_string(swap_timepoint.sec_since_epoch()) });

      auto swap_digest = sha256::hash(sign_payload);
      auto sign = swap_key_priv.sign(swap_digest);
      // not enough active producers approvals
      BOOST_REQUIRE_THROW(
         finish_swap(init_swap_data.rampayer, receiver, init_swap_data.txid, init_swap_data.swap_pubkey,
                     init_swap_data.quantity, init_swap_data.return_address, init_swap_data.return_chain_id,
                     init_swap_data.swap_timestamp, sign),
                     eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(finish_swap_key_assert_test, swap_tester) {
   try {
      name receiver = N(prodc);
      crypto::private_key swap_key_priv = crypto::private_key::generate();
      init_data init_swap_data = {
         .swap_pubkey = get_pubkey_str(swap_key_priv),
         .swap_timestamp = time_point_sec(control->head_block_time())
      };
      time_point swap_timepoint = init_swap_data.swap_timestamp.to_time_point();

      for (const auto &producer : _producer_candidates) {
         init_swap(producer, init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                   init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp);
      }

      string sign_payload = join(
         { receiver.to_string(), init_swap_data.txid, REMCHAIN_ID, init_swap_data.quantity.to_string(),
           init_swap_data.return_address, init_swap_data.return_chain_id,
           std::to_string(swap_timepoint.sec_since_epoch()) });

      // sign digest by another key
      auto swap_digest = sha256::hash(sign_payload);
      auto n_swap_key_priv = crypto::private_key::generate();
      auto sign = n_swap_key_priv.sign(swap_digest);
      // Error expected key different than recovered key
      BOOST_REQUIRE_THROW(
         finish_swap(init_swap_data.rampayer, receiver, init_swap_data.txid, init_swap_data.swap_pubkey,
                     init_swap_data.quantity, init_swap_data.return_address, init_swap_data.return_chain_id,
                     init_swap_data.swap_timestamp, sign),
                     crypto_api_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(finish_after_cancel_swap_test, swap_tester) {
   try {
      name receiver = N(prodc);
      crypto::private_key swap_key_priv = crypto::private_key::generate();
      init_data init_swap_data = {
         .swap_pubkey = get_pubkey_str(swap_key_priv),
         // swap can be canceled after expiration (1 week)
         .swap_timestamp = time_point_sec::from_iso_string("2019-12-01T00:00:00.000")
      };
      time_point swap_timepoint = init_swap_data.swap_timestamp.to_time_point();

      for (const auto &producer : _producer_candidates) {
         init_swap(producer, init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                   init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp);
      }
      cancel_swap(N(rem.swap), init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                  init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp);

      string sign_payload = join(
         { receiver.to_string(), init_swap_data.txid, REMCHAIN_ID, init_swap_data.quantity.to_string(),
           init_swap_data.return_address, init_swap_data.return_chain_id,
           std::to_string(swap_timepoint.sec_since_epoch()) });

      auto swap_digest = sha256::hash(sign_payload);
      auto sign = swap_key_priv.sign(swap_digest);
      // swap already canceled
      BOOST_REQUIRE_THROW(
         finish_swap(init_swap_data.rampayer, receiver, init_swap_data.txid, init_swap_data.swap_pubkey,
                     init_swap_data.quantity, init_swap_data.return_address, init_swap_data.return_chain_id,
                     init_swap_data.swap_timestamp, sign),
                     eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(finishnewacc_swap_test, swap_tester) {
   try {
      name receiver = N(testnewacc11);
      crypto::private_key swap_key_priv = crypto::private_key::generate();
      string owner_acc_pubkey = get_pubkey_str(crypto::private_key::generate());
      string active_acc_pubkey = get_pubkey_str(crypto::private_key::generate());
      init_data init_swap_data = {
         .swap_pubkey = get_pubkey_str(swap_key_priv),
         .swap_timestamp = time_point_sec(control->head_block_time())
      };

      time_point swap_timepoint = init_swap_data.swap_timestamp.to_time_point();
      string swap_payload = join({init_swap_data.swap_pubkey.substr(3), init_swap_data.txid, REMCHAIN_ID,
                                  init_swap_data.quantity.to_string(), init_swap_data.return_address,
                                  init_swap_data.return_chain_id, std::to_string(swap_timepoint.sec_since_epoch())});

      string swap_id = sha256::hash(swap_payload);

      for (const auto &producer : _producer_candidates) {
         init_swap(producer, init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                   init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp);
      }

      string sign_payload = join(
         { receiver.to_string(), owner_acc_pubkey, active_acc_pubkey, init_swap_data.txid, REMCHAIN_ID,
           init_swap_data.quantity.to_string(), init_swap_data.return_address, init_swap_data.return_chain_id,
           std::to_string(swap_timepoint.sec_since_epoch()) });

      auto swap_digest = sha256::hash(sign_payload);
      auto sign = swap_key_priv.sign(swap_digest);

      auto remswap_before_balance = get_balance(N(rem.swap));
      asset producers_reward = get_swap_fee();

      finish_swap_new_account(init_swap_data.rampayer, receiver, owner_acc_pubkey, active_acc_pubkey,
                              init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                              init_swap_data.return_address, init_swap_data.return_chain_id,
                              init_swap_data.swap_timestamp, sign);

      auto receiver_after_balance = get_balance(receiver);
      auto remswap_after_balance = get_balance(N(rem.swap));

      auto data = get_singtable(N(rem.swap), N(swaps), "swap_data");

      // table equal
      BOOST_REQUIRE_EQUAL(swap_id, data["swap_id"].as_string());
      // 2 if swap status finish
      BOOST_REQUIRE_EQUAL("2", data["status"].as_string());
      BOOST_REQUIRE_EQUAL(string(swap_timepoint), data["swap_timestamp"].as_string());
      // amount of provided approvals must be a 2/3 + 1 of active producers
      uint32_t majority = (_producer_candidates.size() * 2 / 3) + 1;
      BOOST_TEST(majority <= data["provided_approvals"].get_array().size());
//      BOOST_TEST(is_account(receiver));
      // balance equal : receiver balance = swapped quantity + min account stake - producers_reward
      BOOST_REQUIRE_EQUAL(init_swap_data.quantity - producers_reward,
                          receiver_after_balance + _core_from_string("100.0000"));
      // balance equal : swap contract -= swapped quantity
      BOOST_REQUIRE_EQUAL(remswap_before_balance - init_swap_data.quantity, remswap_after_balance);

      // swap already finished
      BOOST_REQUIRE_THROW(
         finish_swap_new_account(init_swap_data.rampayer, receiver, owner_acc_pubkey, active_acc_pubkey,
                                 init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                                 init_swap_data.return_address, init_swap_data.return_chain_id,
                                 init_swap_data.swap_timestamp, sign),
         eosio_assert_message_exception);
      // swap doesn't exist
      BOOST_REQUIRE_THROW(
         finish_swap_new_account(init_swap_data.rampayer, receiver, owner_acc_pubkey, active_acc_pubkey,
                                 init_swap_data.txid, init_swap_data.swap_pubkey, _core_from_string("500.0000"),
                                 init_swap_data.return_address, init_swap_data.return_chain_id,
                                 init_swap_data.swap_timestamp, sign),
         eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(finishnewacc_not_confirmed_swap_test, swap_tester) {
   try {
      name receiver = N(testnewacc11);
      crypto::private_key swap_key_priv = crypto::private_key::generate();
      string owner_acc_pubkey = get_pubkey_str(crypto::private_key::generate());
      string active_acc_pubkey = get_pubkey_str(crypto::private_key::generate());
      init_data init_swap_data = {
         .swap_pubkey = get_pubkey_str(swap_key_priv),
         .swap_timestamp = time_point_sec(control->head_block_time())
      };
      time_point swap_timepoint = init_swap_data.swap_timestamp.to_time_point();

      init_swap(N(proda), init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp);

      string sign_payload = join(
         { receiver.to_string(), owner_acc_pubkey, active_acc_pubkey, init_swap_data.txid, REMCHAIN_ID,
           init_swap_data.quantity.to_string(), init_swap_data.return_address, init_swap_data.return_chain_id,
           std::to_string(swap_timepoint.sec_since_epoch()) });

      auto swap_digest = sha256::hash(sign_payload);
      auto sign = swap_key_priv.sign(swap_digest);
      // not enough active producers approvals
      BOOST_REQUIRE_THROW(
         finish_swap(init_swap_data.rampayer, receiver, init_swap_data.txid, init_swap_data.swap_pubkey,
                     init_swap_data.quantity, init_swap_data.return_address, init_swap_data.return_chain_id,
                     init_swap_data.swap_timestamp, sign),
                     eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(finishnewacc_expired_swap_test, swap_tester) {
   try {
      name receiver = N(testnewacc11);
      crypto::private_key swap_key_priv = crypto::private_key::generate();
      string owner_acc_pubkey = get_pubkey_str(crypto::private_key::generate());
      string active_acc_pubkey = get_pubkey_str(crypto::private_key::generate());
      init_data init_swap_data = {
         .swap_pubkey = get_pubkey_str(swap_key_priv),
         // swap can be canceled after expiration (1 week)
         .swap_timestamp = time_point_sec::from_iso_string("2019-12-01T00:00:00.000")
      };
      time_point swap_timepoint = init_swap_data.swap_timestamp.to_time_point();

      for (const auto &producer : _producer_candidates) {
         init_swap(producer, init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                   init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp);
      }

      string sign_payload = join(
         { receiver.to_string(), owner_acc_pubkey, active_acc_pubkey, init_swap_data.txid, REMCHAIN_ID,
           init_swap_data.quantity.to_string(), init_swap_data.return_address, init_swap_data.return_chain_id,
           std::to_string(swap_timepoint.sec_since_epoch()) });

      auto swap_digest = sha256::hash(sign_payload);
      auto sign = swap_key_priv.sign(swap_digest);
      // swap has to be canceled after expiration
      BOOST_REQUIRE_THROW(
         finish_swap_new_account(init_swap_data.rampayer, receiver, owner_acc_pubkey, active_acc_pubkey,
                                 init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                                 init_swap_data.return_address, init_swap_data.return_chain_id,
                                 init_swap_data.swap_timestamp, sign),
                                 eosio_assert_message_exception);

      // swap lifetime expired if a swap_timeswamp > 180 days
      block_timestamp_type swap_expired_timestamp = time_point_sec::from_iso_string("2019-12-25T00:00:44.500");
      for (const auto &producer : _producer_candidates) {
         init_swap(producer, init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                   init_swap_data.return_address, init_swap_data.return_chain_id, swap_expired_timestamp);
      }
      swap_timepoint = swap_expired_timestamp.to_time_point();

      sign_payload = join(
         { receiver.to_string(), owner_acc_pubkey, active_acc_pubkey, init_swap_data.txid, REMCHAIN_ID,
           init_swap_data.quantity.to_string(), init_swap_data.return_address, init_swap_data.return_chain_id,
           std::to_string(swap_timepoint.sec_since_epoch()) });

      swap_digest = sha256::hash(sign_payload);
      sign = swap_key_priv.sign(swap_digest);

      // swap lifetime expired
      produce_blocks(10);
      BOOST_REQUIRE_THROW(
         finish_swap_new_account(init_swap_data.rampayer, receiver, owner_acc_pubkey, active_acc_pubkey,
                                 init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                                 init_swap_data.return_address, init_swap_data.return_chain_id,
                                 swap_expired_timestamp, sign),
         eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(finishnewacc_swap_key_assert_test, swap_tester) {
   try {
      name receiver = N(testnewacc11);
      crypto::private_key swap_key_priv = crypto::private_key::generate();
      string owner_acc_pubkey = get_pubkey_str(crypto::private_key::generate());
      string active_acc_pubkey = get_pubkey_str(crypto::private_key::generate());
      init_data init_swap_data = {
         .swap_pubkey = get_pubkey_str(swap_key_priv),
         .swap_timestamp = time_point_sec(control->head_block_time())
      };
      time_point swap_timepoint = init_swap_data.swap_timestamp.to_time_point();

      for (const auto &producer : _producer_candidates) {
         init_swap(producer, init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                   init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp);
      }

      string sign_payload = join(
         { receiver.to_string(), owner_acc_pubkey, active_acc_pubkey, init_swap_data.txid, REMCHAIN_ID,
           init_swap_data.quantity.to_string(), init_swap_data.return_address, init_swap_data.return_chain_id,
           std::to_string(swap_timepoint.sec_since_epoch()) });

      // sign digest by another key
      auto swap_digest = sha256::hash(sign_payload);
      auto n_swap_key_priv = crypto::private_key::generate();
      auto sign = n_swap_key_priv.sign(swap_digest);
      // Error expected key different than recovered key
      BOOST_REQUIRE_THROW(
         finish_swap_new_account(init_swap_data.rampayer, receiver, owner_acc_pubkey, active_acc_pubkey,
                                 init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                                 init_swap_data.return_address, init_swap_data.return_chain_id,
                                 init_swap_data.swap_timestamp, sign),
                                 crypto_api_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(finishnewacc_after_cancel_swap_test, swap_tester) {
   try {
      name receiver = N(testnewacc11);
      crypto::private_key swap_key_priv = crypto::private_key::generate();
      string owner_acc_pubkey = get_pubkey_str(crypto::private_key::generate());
      string active_acc_pubkey = get_pubkey_str(crypto::private_key::generate());
      init_data init_swap_data = {
         .swap_pubkey = get_pubkey_str(swap_key_priv),
         // swap can be canceled after expiration (1 week)
         .swap_timestamp = time_point_sec::from_iso_string("2019-12-01T00:00:00.000")
      };
      time_point swap_timepoint = init_swap_data.swap_timestamp.to_time_point();

      for (const auto &producer : _producer_candidates) {
         init_swap(producer, init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                   init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp);
      }
      cancel_swap(N(rem.swap), init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                  init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp);

      string sign_payload = join(
         { receiver.to_string(), owner_acc_pubkey, active_acc_pubkey, init_swap_data.txid, REMCHAIN_ID,
           init_swap_data.quantity.to_string(), init_swap_data.return_address, init_swap_data.return_chain_id,
           std::to_string(swap_timepoint.sec_since_epoch()) });

      auto swap_digest = sha256::hash(sign_payload);
      auto sign = swap_key_priv.sign(swap_digest);
      // swap already canceled
      BOOST_REQUIRE_THROW(
         finish_swap_new_account(init_swap_data.rampayer, receiver, owner_acc_pubkey, active_acc_pubkey,
                                 init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                                 init_swap_data.return_address, init_swap_data.return_chain_id,
                                 init_swap_data.swap_timestamp, sign),
                                 eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(cancel_swap_test, swap_tester) {
   try {
      name receiver = N(prodc);
      crypto::private_key swap_key_priv = crypto::private_key::generate();
      init_data init_swap_data = {
         .swap_pubkey = get_pubkey_str(swap_key_priv),
         // swap can be canceled after expiration (1 week)
         .swap_timestamp = time_point_sec::from_iso_string("2019-12-01T00:00:00.000")
      };
      time_point swap_timepoint = init_swap_data.swap_timestamp.to_time_point();

      string swap_payload = join({init_swap_data.swap_pubkey.substr(3), init_swap_data.txid, REMCHAIN_ID,
                                  init_swap_data.quantity.to_string(), init_swap_data.return_address,
                                  init_swap_data.return_chain_id, std::to_string(swap_timepoint.sec_since_epoch())});

      string swap_id = sha256::hash(swap_payload);

      auto remswap_before_init_balance = get_balance(N(rem.swap));
      for (const auto &producer : _producer_candidates) {
         init_swap(producer, init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                   init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp);
      }

      auto remswap_before_cancel_balance = get_balance(N(rem.swap));
      cancel_swap(N(rem.swap), init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                  init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp);

      auto remswap_after_cancel_balance = get_balance(N(rem.swap));
      auto data = get_singtable(N(rem.swap), N(swaps), "swap_data");

      // table equal
      BOOST_REQUIRE_EQUAL(swap_id, data["swap_id"].as_string());
      BOOST_REQUIRE_EQUAL(string(swap_timepoint), data["swap_timestamp"].as_string());
      // -1 if a swap status canceled
      BOOST_REQUIRE_EQUAL("-1", data["status"].as_string());
      //  after init issues quantity amount of tokens, after cancel retires quantity amount of tokens
      BOOST_REQUIRE_EQUAL(remswap_before_init_balance, remswap_after_cancel_balance);
      // balance equal: after issue tokens balance before cancel should be increased on the 'quantity' amount of tokens
      BOOST_REQUIRE_EQUAL(remswap_before_init_balance + init_swap_data.quantity, remswap_before_cancel_balance);
      // amount of provided approvals must be a 2/3 + 1 of active producers
      uint32_t majority = (_producer_candidates.size() * 2 / 3) + 1;
      BOOST_TEST(majority <= data["provided_approvals"].get_array().size());

      block_timestamp_type swap_not_expired_timestamp = time_point_sec(control->head_block_time());
      // swap lifetime expired if a swap_timeswamp > 180 days
      block_timestamp_type swap_expired_timestamp = time_point_sec::from_iso_string("2019-07-05T00:01:10.000");
      for (const auto &producer : _producer_candidates) {
         init_swap(producer, init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                   init_swap_data.return_address, init_swap_data.return_chain_id, swap_not_expired_timestamp);
         init_swap(producer, init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                   init_swap_data.return_address, init_swap_data.return_chain_id, swap_expired_timestamp);
      }
      init_swap(N(proda), init_swap_data.txid, init_swap_data.swap_pubkey, _core_from_string("300.0000"),
                init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp);

      // swap already canceled
      BOOST_REQUIRE_THROW(
         cancel_swap(N(rem.swap), init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                     init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp),
                     eosio_assert_message_exception);
      // swap doesn't exist
      BOOST_REQUIRE_THROW(
         cancel_swap(N(rem.swap), init_swap_data.txid, init_swap_data.swap_pubkey, _core_from_string("100.0000"),
                     init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp);,
                     eosio_assert_message_exception);
      // swap has to be canceled after expiration
      BOOST_REQUIRE_THROW(
         cancel_swap(N(rem.swap), init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                     init_swap_data.return_address, init_swap_data.return_chain_id, swap_not_expired_timestamp),
                     eosio_assert_message_exception);

      // swap lifetime expired
      produce_blocks(10);
      BOOST_REQUIRE_THROW(
         cancel_swap(N(rem.swap), init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                     init_swap_data.return_address, init_swap_data.return_chain_id, swap_expired_timestamp),
                     eosio_assert_message_exception);

      // not enough active producers approvals
      BOOST_REQUIRE_THROW(
         cancel_swap(N(rem.swap), init_swap_data.txid, init_swap_data.swap_pubkey, _core_from_string("300.0000"),
                     init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp),
                     eosio_assert_message_exception);

   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(cancel_after_finish_swap_test, swap_tester) {
   try {
      name receiver = N(prodc);
      crypto::private_key swap_key_priv = crypto::private_key::generate();
      init_data init_swap_data = {
         .swap_pubkey = get_pubkey_str(swap_key_priv),
         .swap_timestamp = time_point_sec(control->head_block_time())
      };
      time_point swap_timepoint = init_swap_data.swap_timestamp.to_time_point();

      for (const auto &producer : _producer_candidates) {
         init_swap(producer, init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                   init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp);
      }

      string sign_payload = join(
         { receiver.to_string(), init_swap_data.txid, REMCHAIN_ID, init_swap_data.quantity.to_string(),
           init_swap_data.return_address, init_swap_data.return_chain_id,
           std::to_string(swap_timepoint.sec_since_epoch()) });

      auto swap_digest = sha256::hash(sign_payload);
      auto sign = swap_key_priv.sign(swap_digest);

      finish_swap(init_swap_data.rampayer, receiver, init_swap_data.txid, init_swap_data.swap_pubkey,
                  init_swap_data.quantity, init_swap_data.return_address, init_swap_data.return_chain_id,
                  init_swap_data.swap_timestamp, sign);

      // swap already finished
      BOOST_REQUIRE_THROW(
         cancel_swap(N(rem.swap), init_swap_data.txid, init_swap_data.swap_pubkey, init_swap_data.quantity,
                     init_swap_data.return_address, init_swap_data.return_chain_id, init_swap_data.swap_timestamp),
                     eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

//BOOST_FIXTURE_TEST_CASE(init_return_swap_test, swap_tester) {
//   try {
//      name sender = N(whale3);
//      string return_address = "9f21f19180c8692ebaa061fd231cd1b029ff2326";
//      string return_chain_id = "ethropsten";
//      string memo = return_chain_id + ' ' + return_address;
//      asset quantity = _core_from_string("500.0000");
//
//      // add to sender unstacked tokens by transfer from rem.swap
//      transfer(N(rem.swap), sender, quantity, "initial tansfer");
//      auto remswap_before_transfer_balance = get_balance(N(rem.swap));
//      auto sender_before_transfer_balance = get_balance(sender);
//
//      transfer(sender, N(rem.swap), quantity, memo);
//
//      auto remswap_after_transfer_balance = get_balance(N(rem.swap));
//      auto sender_after_transfer_balance = get_balance(sender);
//
//      BOOST_REQUIRE_EQUAL(remswap_before_transfer_balance, remswap_after_transfer_balance);
//      BOOST_REQUIRE_EQUAL(sender_before_transfer_balance - quantity, sender_after_transfer_balance);
//
//      // invalid memo
//      BOOST_REQUIRE_THROW(transfer(sender, N(rem.swap), quantity, ""), eosio_assert_message_exception);
//      BOOST_REQUIRE_THROW(transfer(sender, N(rem.swap), quantity, return_chain_id + return_address),
//                          eosio_assert_message_exception);
//      // wrong chain id
//      BOOST_REQUIRE_THROW(transfer(sender, N(rem.swap), quantity, ' ' + return_address),
//                          eosio_assert_message_exception);
//      // wrong address
//      BOOST_REQUIRE_THROW(transfer(sender, N(rem.swap), quantity, return_chain_id + ' '),
//                          eosio_assert_message_exception);
//      // symbol precision mismatch
//      BOOST_REQUIRE_THROW(transfer(sender, N(rem.swap), asset::from_string("500.0000 SYS"), memo),
//                          eosio_assert_message_exception);
//      // the quantity must be greater than the swap minimum amount
//      BOOST_REQUIRE_THROW(transfer(sender, N(rem.swap), asset::from_string("250.0000 REM"), memo),
//                          eosio_assert_message_exception);
//   } FC_LOG_AND_RETHROW()
//};

BOOST_FIXTURE_TEST_CASE(set_swap_fee_test, swap_tester) {
   try {
      setswapfee(2000000);
      auto swap_fee = get_singtable(N(rem.swap), N(swapparams), "swapparams")["in_swap_fee"];
      BOOST_REQUIRE_EQUAL(swap_fee.as_int64(), 2000000);
      // amount must be a positive
      BOOST_REQUIRE_THROW(setswapfee(-1000000), eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(set_min_swap_out_test, swap_tester) {
   try {
      setminswpout(5000000);
      auto min_amount = get_singtable(N(rem.swap), N(swapparams), "swapparams")["out_swap_min_amount"];
      BOOST_REQUIRE_EQUAL(min_amount.as_int64(), 5000000);
      // amount must be a positive
      BOOST_REQUIRE_THROW(setminswpout(-1000000), eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(set_chain_id_test, swap_tester) {
   try {
      string expected_chain_id("0x", 32);
      setchainid(expected_chain_id);
      auto chain_id = get_singtable(N(rem.swap), N(swapparams), "swapparams")["chain_id"];
      BOOST_REQUIRE_EQUAL(chain_id.as_string(), expected_chain_id);
      // invalid chain-id
      BOOST_REQUIRE_THROW(setchainid(""), eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(add_chain_test, swap_tester) {
   try {

      addchain(N(rem.swap), N(ethropsten), true, true);

      auto data = get_supported_chain(N(ethropsten));

      BOOST_REQUIRE_EQUAL(data["chain"].as_string(), "ethropsten");
      BOOST_REQUIRE_EQUAL(data["input"].as_string(), "1");
      BOOST_REQUIRE_EQUAL(data["output"].as_string(), "1");

      addchain(N(rem.swap), N(ethropsten), true, false);
      data = get_supported_chain(N(ethropsten));

      BOOST_REQUIRE_EQUAL(data["output"].as_string(), "0");

      // missing required authority
      BOOST_REQUIRE_THROW(addchain(N(proda), N(ethropsten), true, false), missing_auth_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
