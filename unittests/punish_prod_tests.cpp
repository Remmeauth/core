/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/testing/tester.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

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

using mvo = fc::mutable_variant_object;

namespace
{
struct rem_genesis_account {
   account_name name;
   uint64_t     initial_balance;
};

std::vector<rem_genesis_account> rem_test_genesis( {
  {N(b1),        100'000'000'0000ll}, // TODO investigate why `b1` should be at least this value?
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
  {N(runnerup1),     280'000'0000ll},
  {N(runnerup2),     270'000'0000ll},
  {N(runnerup3),     260'000'0000ll},
  {N(runnerup4),     250'000'0000ll},
  {N(runnerup5),     240'000'0000ll},
  {N(test),          10'000'0000ll },
});

class punish_tester : public TESTER {
public:
   punish_tester();

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
      const auto& accnt = control->db().get<account_object,by_name>( config::system_account_name );
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_ser.set_abi(abi, abi_serializer_max_time);
   }

   fc::variant get_global_state() {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(global), N(global) );
      if (data.empty()) std::cout << "\nData is empty\n" << std::endl;
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "eosio_global_state", data, abi_serializer_max_time );
   }

   fc::variant get_global_rem_state() {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(globalrem), N(globalrem) );
      if (data.empty()) std::cout << "\nData is empty\n" << std::endl;
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "eosio_global_rem_state", data, abi_serializer_max_time );
   }

   uint32_t produce_blocks_until_schedule_is_changed(const uint32_t max_blocks) {
       const auto current_version = control->active_producers().version;
       uint32_t blocks_produced = 0;
       while (control->active_producers().version == current_version && blocks_produced < max_blocks) {
           produce_block();
           blocks_produced++;
       }
       return blocks_produced;
   }

    auto punish_producer( name producer) {
        auto r = base_tester::push_action(config::system_account_name, N(punishprod), producer, mvo()
                     ("producer", producer )
                     );
        produce_block();
        return r;
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

   auto undelegate_bandwidth( name from, name receiver, asset unstake_quantity ) {
       auto r = base_tester::push_action(config::system_account_name, N(undelegatebw), from, mvo()
                    ("from", from )
                    ("receiver", receiver)
                    ("unstake_quantity", unstake_quantity)
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

    auto transfer( name from, name to, asset quantity ) {
       auto r = push_action( N(rem.token), N(transfer), config::system_account_name,
                             mutable_variant_object()
                             ("from", from)
                             ("to", to)
                             ("quantity", quantity)
                             ("memo", "")
       );
       produce_block();

       return r;
    }

    auto torewards( name caller, name payer, asset amount ) {
        auto r = base_tester::push_action(config::system_account_name, N(torewards), caller, mvo()
                ("payer", payer)
                ("amount", amount)
        );
        produce_block();
        return r;
    }

    auto claim_rewards( name owner ) {
       auto r = base_tester::push_action( config::system_account_name, N(claimrewards), owner, mvo()("owner",  owner ));
       produce_block();
       return r;
    }

    auto set_privileged( name account ) {
       auto r = base_tester::push_action(config::system_account_name, N(setpriv), config::system_account_name,  mvo()("account", account)("is_priv", 1));
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

    asset get_balance( const account_name& act ) {
         return get_currency_balance(N(rem.token), symbol(CORE_SYMBOL), act);
    }

    void set_code_abi(const account_name& account, const vector<uint8_t>& wasm, const char* abi, const private_key_type* signer = nullptr) {
       wdump((account));
        set_code(account, wasm, signer);
        set_abi(account, abi, signer);
        if (account == config::system_account_name) {
           const auto& accnt = control->db().get<account_object,by_name>( account );
           abi_def abi_definition;
           BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi_definition), true);
           abi_ser.set_abi(abi_definition, abi_serializer_max_time);
        }
        produce_blocks();
    }

    fc::variant get_producer_info( const account_name& act ) {
       vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(producers), act );
       return abi_ser.binary_to_variant( "producer_info", data, abi_serializer_max_time );
    }

    fc::variant get_voter_info( const account_name& act ) {
       vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(voters), act );
       return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "voter_info", data, abi_serializer_max_time );
    }

    // Vote for producers
    void votepro( account_name voter, vector<account_name> producers ) {
       std::sort( producers.begin(), producers.end() );
       base_tester::push_action(config::system_account_name, N(voteproducer), voter, mvo()
                            ("voter", name(voter))
                            ("proxy", name(0) )
                            ("producers", producers)
                );
       produce_blocks();
    };

   auto unregister_producer(name producer) {
       auto r = base_tester::push_action(config::system_account_name, N(unregprod), producer, mvo()
               ("producer",  name(producer))
               ("producer_key", get_public_key( producer, "active" ) )
               ("url", "" )
               ("location", 0 )
       );
       produce_block();
       return r;
   }

   auto refund_to_stake( const name& to ) {
      auto r = base_tester::push_action(
         config::system_account_name, N(refundtostake), to,
         mvo()("owner", to)
      );

      produce_block();
      return r;
   }

    fc::microseconds microseconds_since_epoch_of_iso_string( const fc::variant& v ) {
        return time_point::from_iso_string( v.as_string() ).time_since_epoch();
    }

    abi_serializer abi_ser;
};

punish_tester::punish_tester() {
   // Create rem.msig and rem.token
   create_accounts({N(rem.msig), N(rem.token), N(rem.rex), N(rem.ram), N(rem.ramfee), N(rem.stake), N(rem.bpay), N(rem.spay), N(rem.vpay), N(rem.saving) });

   // Set code for the following accounts:
   //  - rem (code: rem.bios) (already set by tester constructor)
   //  - rem.msig (code: rem.msig)
   //  - rem.token (code: rem.token)
   set_code_abi(N(rem.msig),
               contracts::rem_msig_wasm(),
               contracts::rem_msig_abi().data());//, &rem_active_pk);
   set_code_abi(N(rem.token),
               contracts::rem_token_wasm(),
               contracts::rem_token_abi().data()); //, &rem_active_pk);

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
   for( const auto& account : rem_test_genesis ) {
      create_account( account.name, config::system_account_name );
   }

   deploy_contract();

   // Buy ram and stake cpu and net for each genesis accounts
   for( const auto& account : rem_test_genesis ) {
      const auto stake_quantity = account.initial_balance - 1000;

      const auto r = delegate_bandwidth(N(rem.stake), account.name, asset(stake_quantity));
      BOOST_REQUIRE( !r->except_ptr );
   }
   auto producer_candidates = std::vector< account_name >{
       N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
       N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
       N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ)
   };
   for( auto pro : producer_candidates ) {
       register_producer(pro);
       votepro( pro, {pro} );
   }
   auto runnerups = std::vector< account_name >{
       N(runnerup1), N(runnerup2), N(runnerup3),  N(runnerup4),  N(runnerup5)
   };
   for( auto pro : runnerups ) {
       register_producer(pro);
       votepro( pro, {pro} );
   }

   // vote for prods to activate 15% of stake
   const auto whales = std::vector< name >{ N(b1), N(whale1), N(whale2) };
   for( const auto& whale : whales ) {
       votepro( whale, producer_candidates );
   }
   produce_blocks_for_n_rounds(3);
}

BOOST_AUTO_TEST_SUITE(punish_prod_tests)
BOOST_FIXTURE_TEST_CASE( punish_prod_not_enough_inactivity_test, punish_tester ) {
    try {
        {
            const auto active_schedule = control->head_block_state()->active_schedule;
            name prod_name_to_punish = active_schedule.producers.at(0).producer_name;

            BOOST_REQUIRE_THROW(
               punish_producer(prod_name_to_punish), eosio_assert_message_exception);

            produce_block();
            BOOST_TEST_REQUIRE(  get_producer_info( prod_name_to_punish )["is_active"].as_bool() );
        }
    } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( punish_prod_enough_inactivity_test, punish_tester ) {
    try {
        produce_empty_block(fc::microseconds(get_global_rem_state()["producer_max_inactivity_time"]["_count"].as_int64()));
        {
            const auto active_schedule = control->head_block_state()->active_schedule;
            name prod_name_to_punish = active_schedule.producers.at(0).producer_name;

            punish_producer(prod_name_to_punish);

            BOOST_TEST_REQUIRE(  !get_producer_info( prod_name_to_punish )["is_active"].as_bool() );
        }
    } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( punish_punished_prod_test, punish_tester ) {
    try {
        produce_empty_block(fc::microseconds(get_global_rem_state()["producer_max_inactivity_time"]["_count"].as_int64()));
        {
            const auto active_schedule = control->head_block_state()->active_schedule;
            name prod_name_to_punish = active_schedule.producers.at(0).producer_name;

            punish_producer(prod_name_to_punish);
            BOOST_REQUIRE_THROW(
               punish_producer(prod_name_to_punish), eosio_assert_message_exception);

        }
    } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( regprod_during_punishment_period_test, punish_tester ) {
    try {
        produce_empty_block(fc::microseconds(get_global_rem_state()["producer_max_inactivity_time"]["_count"].as_int64()));
        {
            const auto active_schedule = control->head_block_state()->active_schedule;
            name prod_name_to_punish = active_schedule.producers.at(0).producer_name;

            punish_producer(prod_name_to_punish);
            BOOST_REQUIRE_THROW(
               register_producer(prod_name_to_punish), eosio_assert_message_exception);
            BOOST_TEST_REQUIRE( !get_producer_info( prod_name_to_punish )["is_active"].as_bool() );
        }
    } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( regprod_after_punishment_period_test, punish_tester ) {
    try {
        produce_empty_block(fc::microseconds(get_global_rem_state()["producer_max_inactivity_time"]["_count"].as_int64()));
        {
            const auto active_schedule = control->head_block_state()->active_schedule;
            name prod_name_to_punish = active_schedule.producers.at(0).producer_name;

            punish_producer(prod_name_to_punish);
            produce_empty_block(fc::days(32));
            register_producer(prod_name_to_punish);
            BOOST_TEST_REQUIRE( get_producer_info( prod_name_to_punish )["is_active"].as_bool() );
        }
    } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( punish_non_existing_producer_test, punish_tester ) {
    try {
        produce_empty_block(fc::microseconds(get_global_rem_state()["producer_max_inactivity_time"]["_count"].as_int64()));
        {
            name prod_name_to_punish = N(test);
            BOOST_REQUIRE_THROW(
               punish_producer(prod_name_to_punish), eosio_assert_message_exception);
        }
    } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( punish_not_active_producer_test, punish_tester ) {
    try {
        produce_empty_block(fc::microseconds(get_global_rem_state()["producer_max_inactivity_time"]["_count"].as_int64()));
        {
          const auto active_schedule = control->head_block_state()->active_schedule;
          name prod_name_to_punish = active_schedule.producers.at(0).producer_name;
          unregister_producer(prod_name_to_punish);

          BOOST_REQUIRE_THROW(
             punish_producer(prod_name_to_punish), eosio_assert_message_exception);
        }
    } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( punish_not_top21_producer_test, punish_tester ) {
    try {
        produce_empty_block(fc::microseconds(get_global_rem_state()["producer_max_inactivity_time"]["_count"].as_int64()));
        {
          const auto active_schedule = control->head_block_state()->active_schedule;
          name prod_name_to_punish = N(runnerup5);

          BOOST_REQUIRE_THROW(
             punish_producer(prod_name_to_punish), eosio_assert_message_exception);
        }
    } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( punish_prod_right_after_top21_test, punish_tester ) {
    try {
        const auto whales = std::vector< name >{ N(b1) };
        auto producer_candidates = std::vector< account_name >{
            N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
            N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
            N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ)
        };
        const name prod_name_to_punish = N(runnerup5);

        for( const auto& whale : whales ) {
            votepro( whale, std::vector<name>{prod_name_to_punish} );
        }
        produce_blocks_for_n_rounds(3);

        for( const auto& whale : whales ) {
            votepro( whale, producer_candidates );
        }

        produce_blocks_for_n_rounds(3);

        produce_empty_block(fc::microseconds(get_global_rem_state()["producer_max_inactivity_time"]["_count"].as_int64()));

        for( const auto& whale : whales ) {
            votepro( whale, std::vector<name>{prod_name_to_punish} );
        }

        produce_blocks_until_schedule_is_changed(1000);

        BOOST_REQUIRE_THROW(
           punish_producer(prod_name_to_punish), eosio_assert_message_exception);

    } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace anonymous
