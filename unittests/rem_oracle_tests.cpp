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

class oracle_tester : public TESTER {
public:
   oracle_tester();

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

   variant get_remprice_tbl( const name& pair ) {
      vector<char> data = get_row_by_account( N(rem.oracle), N(rem.oracle), N(remprice), pair );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "remprice", data, abi_serializer_max_time );
   }

   variant get_pricedata_tbl( const name& producer ) {
      vector<char> data = get_row_by_account( N(rem.oracle), N(rem.oracle), N(pricedata), producer );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "pricedata", data, abi_serializer_max_time );
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

   asset get_balance( const account_name& act ) {
      return get_currency_balance(N(rem.token), symbol(CORE_SYMBOL), act);
   }

   void set_code_abi(const account_name& account, const vector<uint8_t>& wasm, const char* abi, const private_key_type* signer = nullptr) {
      wdump((account));
      set_code(account, wasm, signer);
      set_abi(account, abi, signer);
      if (account == N(rem.oracle)) {
         const auto& accnt = control->db().get<account_object,by_name>( account );
         abi_def abi_definition;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi_definition), true);
         abi_ser.set_abi(abi_definition, abi_serializer_max_time);
      }
      produce_blocks();
   }

   abi_serializer abi_ser;
};

oracle_tester::oracle_tester() {
   // Create rem.msig and rem.token, rem.oracle
   create_accounts({N(rem.msig), N(rem.token), N(rem.rex), N(rem.ram), N(rem.ramfee), N(rem.stake), N(rem.bpay), N(rem.spay), N(rem.vpay), N(rem.saving), N(rem.oracle) });

   // Register producers
   const auto producer_candidates = {
      N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
      N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
      N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ)
   };

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
   // Set code for the following accounts:
   //  - rem (code: rem.bios) (already set by tester constructor)
   //  - rem.msig (code: rem.msig)
   //  - rem.token (code: rem.token)
   //  - rem.oracle (code: rem.oracle)
   set_code_abi(N(rem.msig),
                contracts::rem_msig_wasm(),
                contracts::rem_msig_abi().data());//, &rem_active_pk);
   set_code_abi(N(rem.token),
                contracts::rem_token_wasm(),
                contracts::rem_token_abi().data()); //, &rem_active_pk);
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

   // Create SYS tokens in rem.token, set its manager as rem
   const auto max_supply     = core_from_string("1000000000.0000"); /// 10x larger than 1B initial tokens
   const auto initial_supply = core_from_string("100000000.0000");  /// 10x larger than 1B initial tokens

   create_currency(N(rem.token), config::system_account_name, max_supply);
   // Issue the genesis supply of 1 billion SYS tokens to rem.system
   issue(N(rem.token), config::system_account_name, config::system_account_name, initial_supply);

   auto actual = get_balance(config::system_account_name);
   BOOST_REQUIRE_EQUAL(initial_supply, actual);

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
   // set permission @rem.code to rem.oracle
   updateauth(N(rem.oracle), N(rem.oracle));

   // add new supported pairs to the rem.oracle
   vector<name> supported_pairs = {
      N(rem.usd), N(rem.eth), N(rem.btc),
   };
   for (const auto &pair : supported_pairs) {
      addpair(pair, { {N(rem.oracle), config::active_name} });
   }
}

BOOST_AUTO_TEST_SUITE(rem_oracle_tests)

BOOST_FIXTURE_TEST_CASE( setprice_test, oracle_tester ) {
   try {
      const auto _producers = control->head_block_state()->active_schedule.producers;
      uint32_t majority_amount = (_producers.size() * 2 / 3) + 1;
      vector<name> supported_pairs = {N(rem.usd), N(rem.btc), N(rem.eth)};

      map<name, double> pair_price {
         {N(rem.usd), 0.003210},
         {N(rem.btc), 0.0000003957},
         {N(rem.eth), 0.0000176688}
      };

      for (size_t i = 0; i < _producers.size(); ++i) {
         setprice(_producers[i].producer_name, pair_price);

         for (const auto &pair : supported_pairs) {
            auto pair_data = get_remprice_tbl(pair);

            // if the amount of producers points in the pricedata < majority amount, at the first time
            // remprice should be an empty
            if (i < majority_amount) {
               BOOST_TEST_REQUIRE(pair_data.is_null());
            } else {
               BOOST_TEST_REQUIRE(!pair_data.is_null());
            }
         }
      }

      {
         time_point ct;
         produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(3600)); // +1 hour
         for (const auto &producer : _producers) {
            setprice(producer.producer_name, pair_price);
            ct = control->head_block_time();

            auto pricedata = get_pricedata_tbl(producer.producer_name);

            BOOST_TEST_REQUIRE(pricedata["producer"].as_string() == producer.producer_name.to_string());
            BOOST_TEST_REQUIRE(pricedata["last_update"].as_string() == string(ct));
         }
         ct = control->head_block_time();

         // compromised price by non-active producers
         vector<name> non_active_prods = {N(b1), N(whale1), N(whale2)};
         map<name, double> compromised_pair_price{
            {N(rem.usd), 3210},
            {N(rem.btc), 0.3957},
            {N(rem.eth), 176688}
         };
         for (const auto &prod: non_active_prods) {
            setprice(prod, compromised_pair_price);
         }

         for (const auto &pair : supported_pairs) {

            auto pair_data = get_remprice_tbl(pair);
            auto supported_pairs = get_singtable(N(rem.oracle), N(pairstable), "pairstable");

            vector<variant> pair_points(_producers.size(), pair_price[pair]);

            BOOST_TEST_REQUIRE(pair_data["price"].as_double() == pair_price[pair]);
            BOOST_TEST_REQUIRE(pair_data["pair"].as_string() == pair.to_string());
            BOOST_TEST_REQUIRE(pair_data["price_points"].get_array() == pair_points);
            BOOST_TEST_REQUIRE(pair_data["last_update"].as_string() == string(ct));
         }
      }

      // block producer authorization required
      BOOST_REQUIRE_THROW(setprice(N(runnerup3), pair_price), eosio_assert_message_exception );
      // the frequency of price changes should not exceed 1 time per hour
      BOOST_REQUIRE_THROW(setprice(N(proda), pair_price), eosio_assert_message_exception );
      // incorrect pairs
      pair_price = { {N(rem.usd), 0.003210},
                     {N(rem.btc), 0.0000003957} };
      BOOST_REQUIRE_THROW(setprice(N(proda), pair_price), eosio_assert_message_exception );
      // unsupported pairs
      pair_price[N(remxrp)] = 0.0000003957;
      BOOST_REQUIRE_THROW(setprice(N(proda), pair_price), eosio_assert_message_exception );

   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( setprice_median_test, oracle_tester ) {
   try {
      const auto _producers = control->head_block_state()->active_schedule.producers;
      vector<name> supported_pairs = {N(rem.usd), N(rem.btc), N(rem.eth)};
      uint32_t majority_amount = (_producers.size() * 2 / 3) + 1;
      map<name, double> pair_price {
         {N(rem.usd), 0.003210},
         {N(rem.btc), 0.0000003957},
         {N(rem.eth), 0.00000176688}
      };

      vector<variant> remusd_points;
      vector<variant> rembtc_points;
      vector<variant> remeth_points;
      produce_blocks_for_n_rounds(10); // shift time to test the frequency of price changes

      for (const auto &producer: _producers) {
         remusd_points.emplace_back(++pair_price[N(rem.usd)]);
         rembtc_points.emplace_back(++pair_price[N(rem.btc)]);
         remeth_points.emplace_back(++pair_price[N(rem.eth)]);
         setprice(producer.producer_name, pair_price);
      }

      auto remusd_data = get_remprice_tbl(N(rem.usd));
      auto rembtc_data = get_remprice_tbl(N(rem.btc));
      auto remeth_data = get_remprice_tbl(N(rem.eth));

      BOOST_TEST_REQUIRE(remusd_data["price"].as_double() == remusd_points[majority_amount / 2]);
      BOOST_TEST_REQUIRE(rembtc_data["price"].as_double() == rembtc_points[majority_amount / 2]);
      BOOST_TEST_REQUIRE(remeth_data["price"].as_double() == remeth_points[majority_amount / 2]);
      BOOST_TEST_REQUIRE(remusd_data["price_points"].get_array() == remusd_points);
      BOOST_TEST_REQUIRE(rembtc_data["price_points"].get_array() == rembtc_points);
      BOOST_TEST_REQUIRE(remeth_data["price_points"].get_array() == remeth_points);

      // shift median remusd on nth position
      // the range of points remusd is a [ 1 .. 21 ], to shift median need to reduce delta of the current subset
      for(size_t i = 0; i < (_producers.size() - majority_amount); ++i) {
         produce_blocks_for_n_rounds(29); // +1 hour

         // increase the prev delta for the current subset
         pair_price[N(rem.usd)] = i + 1;
         setprice(_producers.at(i).producer_name, pair_price);
         // decrease by 0.5 the current delta
         pair_price[N(rem.usd)] = i + 2.5;
         setprice(_producers.at(i + 1).producer_name, pair_price);

         remusd_data = get_remprice_tbl(N(rem.usd));

         BOOST_TEST_REQUIRE(remusd_data["price"].as_double() == remusd_points[(majority_amount / 2) + i + 1]);
      }

   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( addpair_test, oracle_tester ) {
   try {
      const auto _producers = control->head_block_state()->active_schedule.producers;
      vector<name> supported_pairs = {N(rem.usd), N(rem.btc), N(rem.eth)};

      map<name, double> pair_price {
         {N(rem.usd), 0.003210},
         {N(rem.btc), 0.0000003957},
         {N(rem.eth), 0.0000176688}
      };

      for (const auto &producer : _producers) {
         setprice(producer.producer_name, pair_price);
      }

      auto remusd_data = get_remprice_tbl(N(rem.usd));
      auto rembtc_data = get_remprice_tbl(N(rem.btc));
      auto remeth_data = get_remprice_tbl(N(rem.eth));

      supported_pairs.emplace_back(N(rem.bnb));
      addpair(N(rem.bnb), { {N(rem.oracle), config::active_name} });

      produce_blocks_for_n_rounds(29); // +1 hour
      pair_price[N(rem.bnb)] = 1;
      setprice(N(proda), pair_price);

      auto rembnb_data = get_remprice_tbl(N(rem.bnb));
      // rembnb must be empty, because there is only 1 point of the active producer
      BOOST_TEST_REQUIRE(rembnb_data.is_null());

      auto remusd_data_after = get_remprice_tbl(N(rem.usd));
      auto rembtc_data_after = get_remprice_tbl(N(rem.btc));
      auto remeth_data_after = get_remprice_tbl(N(rem.eth));

      BOOST_TEST_REQUIRE(remusd_data["price"] == remusd_data_after["price"]);
      BOOST_TEST_REQUIRE(remusd_data["pair"] == remusd_data_after["pair"]);
      BOOST_TEST_REQUIRE(remusd_data["last_update"] == remusd_data_after["last_update"]);

      BOOST_TEST_REQUIRE(rembtc_data["price"] == rembtc_data_after["price"]);
      BOOST_TEST_REQUIRE(rembtc_data["pair"] == rembtc_data_after["pair"]);
      BOOST_TEST_REQUIRE(rembtc_data["last_update"] == rembtc_data_after["last_update"]);

      BOOST_TEST_REQUIRE(remeth_data["price"] == remeth_data_after["price"]);
      BOOST_TEST_REQUIRE(remeth_data["pair"] == remeth_data_after["pair"]);
      BOOST_TEST_REQUIRE(remeth_data["last_update"] == remeth_data_after["last_update"]);

      produce_blocks_for_n_rounds(29); // +1 hour
      // add new pair data (rembnb)
      for (const auto &producer : _producers) {
         setprice(producer.producer_name, pair_price);
      }
      vector<variant> rembnb_points(_producers.size(), pair_price[N(rem.bnb)]);
      time_point ct = control->head_block_time();
      rembnb_data = get_remprice_tbl(N(rem.bnb));

      BOOST_TEST_REQUIRE(rembnb_data["price"].as_double() == pair_price[N(rem.bnb)]);
      BOOST_TEST_REQUIRE(rembnb_data["pair"].as_string() == "rem.bnb");
      BOOST_TEST_REQUIRE(rembnb_data["price_points"].get_array() == rembnb_points);
      BOOST_TEST_REQUIRE(rembnb_data["last_update"].as_string() == string(ct));

      // the pair is already supported
      BOOST_REQUIRE_THROW(addpair(N(rem.bnb), { {N(rem.oracle), config::active_name} }),
                          eosio_assert_message_exception);
      // missing required authority rem.oracle
      BOOST_REQUIRE_THROW(addpair(N(btcrem), { {N(proda), config::active_name} }), missing_auth_exception);

   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
