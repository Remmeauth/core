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

struct genesis_account {
   account_name aname;
   uint64_t     initial_balance;
};

 static std::vector<genesis_account> test_genesis( {
                                              {N(b1),       100'000'000'0000ll},
                                              {N(whale4),    40'000'000'0000ll},
                                              {N(whale3),    30'000'000'0000ll},
                                              {N(whale2),    20'000'000'0000ll},
                                              {N(proda),      1'000'000'0000ll},
                                              {N(prodb),      1'000'000'0000ll},
                                              {N(prodc),      1'000'000'0000ll},
                                              {N(prodd),      1'000'000'0000ll},
                                              {N(prode),      1'000'000'0000ll},
                                              {N(prodf),      1'000'000'0000ll},
                                              {N(prodg),      1'000'000'0000ll},
                                              {N(prodh),      1'000'000'0000ll},
                                              {N(prodi),      1'000'000'0000ll},
                                              {N(prodj),      1'000'000'0000ll},
                                              {N(prodk),      1'000'000'0000ll},
                                              {N(prodl),      1'000'000'0000ll},
                                              {N(prodm),      1'000'000'0000ll},
                                              {N(prodn),      1'000'000'0000ll},
                                              {N(prodo),      1'000'000'0000ll},
                                              {N(prodp),      1'000'000'0000ll},
                                              {N(prodq),      1'000'000'0000ll},
                                              {N(prodr),      1'000'000'0000ll},
                                              {N(prods),      1'000'000'0000ll},
                                              {N(prodt),      1'000'000'0000ll},
                                              {N(produ),      1'000'000'0000ll},
                                              {N(runnerup1),  1'000'000'0000ll},
                                              {N(runnerup2),  1'000'000'0000ll},
                                              {N(runnerup3),  1'000'000'0000ll},
                                              {N(minow1),         1'100'0000ll},
                                              {N(minow2),         1'050'0000ll},
                                              {N(minow3),         1'050'0000ll},
                                              {N(masses),   500'000'000'0000ll}
                                           });

class rotation_tester : public TESTER {
public:
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

   std::pair<name, name> get_rotated_producers() {
      auto data = get_row_by_account(config::system_account_name, config::system_account_name, N(rotations), N(rotations));
      if (data.empty()) {
         return std::make_pair(0, 0);
      }

      fc::variant v = abi_ser.binary_to_variant( "rotation_state", data, abi_serializer_max_time );
      name bp_out;
      name sbp_in;
      fc::from_variant(v["bp_out"], bp_out);
      fc::from_variant(v["sbp_in"], sbp_in);
      return std::make_pair(bp_out, sbp_in);
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

   uint32_t produce_blocks_until_schedule_is_changed(const uint32_t max_blocks) {
      const auto current_version = control->active_producers().version;
      uint32_t blocks_produced = 0;
      while (control->active_producers().version == current_version && blocks_produced < max_blocks) {
         produce_block();
         blocks_produced++;
      }
      return blocks_produced;
   }

   void test_schedule(const std::vector<std::string>& expected_schedule) {
       const auto active_schedule = control->head_block_state()->active_schedule;
       BOOST_REQUIRE(active_schedule.producers.size() == expected_schedule.size());
       for (size_t i = 0; i < expected_schedule.size(); i++) {
           BOOST_TEST(active_schedule.producers.at(i).producer_name == expected_schedule.at(i));
       }
   }


   abi_serializer abi_ser;
};

BOOST_AUTO_TEST_SUITE(rotation_tests)

BOOST_FIXTURE_TEST_CASE( rotation_with_less_than_4_standby_test, rotation_tester ) {
    try {

        // Create rem.msig and rem.token
        create_accounts({N(rem.msig), N(rem.token), N(rem.ram), N(rem.ramfee), N(rem.stake), N(rem.vpay), N(rem.spay), N(rem.saving) });
        set_code_abi(N(rem.msig),
                     contracts::rem_msig_wasm(),
                     contracts::rem_msig_abi().data());//, &rem_active_pk);
        set_code_abi(N(rem.token),
                     contracts::rem_token_wasm(),
                     contracts::rem_token_abi().data()); //, &rem_active_pk);

        // Set privileged for rem.msig and rem.token
        set_privileged(N(rem.msig));
        set_privileged(N(rem.token));


        // Create SYS tokens in rem.token, set its manager as rem
        auto max_supply = core_from_string("1000000000.0000");
        auto initial_supply = core_from_string("900000000.0000");
        create_currency(N(rem.token), config::system_account_name, max_supply);
        // Issue the genesis supply of 1 billion SYS tokens to rem.system
        issue(N(rem.token), config::system_account_name, config::system_account_name, initial_supply);

        // Create genesis accounts
        for( const auto& a : test_genesis ) {
            create_account( a.aname, config::system_account_name );
        }

        deploy_contract();

        // Buy ram and stake cpu and net for each genesis accounts
        for( const auto& a : test_genesis ) {
            auto stake_quantity = a.initial_balance - 1000;

            auto r = delegate_bandwidth(N(rem.stake), a.aname, asset(stake_quantity));
            BOOST_REQUIRE( !r->except_ptr );
        }


        // register whales as producers
        const auto whales_as_producers = { N(b1), N(whale4), N(whale3), N(whale2) };
        for( const auto& producer : whales_as_producers ) {
            register_producer(producer);
        }

        auto producer_candidates = {
            N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
            N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
            N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ),
            N(runnerup1), N(runnerup2), N(runnerup3)
        };

        // Register producers
        for( auto pro : producer_candidates ) {
            register_producer(pro);
        }

        // Vote for producers
        auto votepro = [&]( account_name voter, vector<account_name> producers ) {
            std::sort( producers.begin(), producers.end() );
            base_tester::push_action(config::system_account_name, N(voteproducer), voter, mvo()
                ("voter",  name(voter))
                ("proxy", name(0) )
                ("producers", producers)
            );
        };
        votepro( N(b1), { N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                          N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                          N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ)} );
        votepro( N(whale2), {N(runnerup1), N(runnerup2), N(runnerup3)} );
        votepro( N(whale3), {N(proda), N(prodb), N(prodc), N(prodd), N(prode)} );
        votepro( N(whale4), {N(prodq), N(prodr), N(prods), N(prodt), N(produ)} );
        for( auto prod : producer_candidates ) {
            votepro(prod, { prod });
        }
        //After this voting producers table looks like this:
        //Top21:
        // (0-4) prodq, prodr, prods, prodt, produ - 141'000'000'0000 votes each
        // (5-9) proda, prodb, prodc, prodd, prode - 131'000'000'0000 votes each
        // (10-20) prodf, prodg, prodh, prodi, prodj, prodk, prodl, prodm, prodn, prodo, prodp - 101'000'000'0000 votes each
        //Standby:
        // (21-23) runnerup1, runnerup2, runnerup3 - 21'000'000'0000 votes each
        //
        //So the first rotetion will be prodq(first in top21)-runnerup1(first in standby list)

        produce_blocks(4);
        produce_block();
        test_schedule({ "proda", "prodb", "prodc", "prodd", "prode", "prodf", "prodg", "prodh", "prodi", "prodj", "prodk",
                        "prodl", "prodm", "prodn", "prodo", "prodp", "prodr", "prods", "prodt", "produ", "runnerup1" });
        BOOST_TEST(get_rotated_producers().first == N(prodq)); //bp out
        BOOST_TEST(get_rotated_producers().second == N(runnerup1)); //standby in

        // make changes to top 21 producer list
        votepro( N(b1), { N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                          N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                          N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ),
                          N(runnerup2)} );
        produce_blocks_until_schedule_is_changed(2000);
        produce_blocks(2);
        //After this voting producers table looks like this:
        //Top21:
        // (0-4) prodq, prodr, prods, prodt, produ - 141'000'000'0000 votes each
        // (5-9) proda, prodb, prodc, prodd, prode - 131'000'000'0000 votes each
        // (10) runnerup2 - 121'000'000'0000 votes
        // (11-20) prodf, prodg, prodh, prodi, prodj, prodk, prodl, prodm, prodn, prodo - 101'000'000'0000 votes each
        //Standby:
        // (21) prodp - 101'000'000'0000 votes
        // (22-23) runnerup1, runnerup3 - 21'000'000'0000 votes each
        //
        //Rotation is the same
        test_schedule({ "proda", "prodb", "prodc", "prodd", "prode", "prodf", "prodg", "prodh", "prodi", "prodj", "prodk",
                        "prodl", "prodm", "prodn", "prodo", "prodr", "prods", "prodt", "produ", "runnerup1", "runnerup2" });
        BOOST_TEST(get_rotated_producers().first == N(prodq)); //bp out
        BOOST_TEST(get_rotated_producers().second == N(runnerup1)); //standby in

        produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(12 * 3600));
        produce_blocks_until_schedule_is_changed(2000);
        produce_blocks(2);
        //Producers table is the same:
        //Top21:
        // (0-4) prodq, prodr, prods, prodt, produ - 141'000'000'0000 votes each
        // (5-9) proda, prodb, prodc, prodd, prode - 131'000'000'0000 votes each
        // (10) runnerup2 - 121'000'000'0000 votes
        // (11-20) prodf, prodg, prodh, prodi, prodj, prodk, prodl, prodm, prodn, prodo - 101'000'000'0000 votes each
        //Standby:
        // (21) prodp - 101'000'000'0000 votes
        // (22-23) runnerup1, runnerup3 - 21'000'000'0000 votes each
        //
        //Rotation has changed after 12 hours: prodr-runnerup3
        test_schedule({ "proda", "prodb", "prodc", "prodd", "prode", "prodf", "prodg", "prodh", "prodi", "prodj", "prodk",
                        "prodl", "prodm", "prodn", "prodo", "prodq", "prods", "prodt", "produ", "runnerup2", "runnerup3" });
        BOOST_TEST(get_rotated_producers().first == N(prodr)); //bp out
        BOOST_TEST(get_rotated_producers().second == N(runnerup3)); //standby in

    } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( rotation_cancelled_after_bp_has_been_unvoted_test, rotation_tester ) {
    try {

        // Create rem.msig and rem.token
        create_accounts({N(rem.msig), N(rem.token), N(rem.ram), N(rem.ramfee), N(rem.stake), N(rem.vpay), N(rem.spay), N(rem.saving) });
        set_code_abi(N(rem.msig),
                     contracts::rem_msig_wasm(),
                     contracts::rem_msig_abi().data());//, &rem_active_pk);
        set_code_abi(N(rem.token),
                     contracts::rem_token_wasm(),
                     contracts::rem_token_abi().data()); //, &rem_active_pk);

        // Set privileged for rem.msig and rem.token
        set_privileged(N(rem.msig));
        set_privileged(N(rem.token));


        // Create SYS tokens in rem.token, set its manager as rem
        auto max_supply = core_from_string("1000000000.0000");
        auto initial_supply = core_from_string("900000000.0000");
        create_currency(N(rem.token), config::system_account_name, max_supply);
        // Issue the genesis supply of 1 billion SYS tokens to rem.system
        issue(N(rem.token), config::system_account_name, config::system_account_name, initial_supply);

        // Create genesis accounts
        for( const auto& a : test_genesis ) {
            create_account( a.aname, config::system_account_name );
        }
        create_account( N(runnerup4), config::system_account_name );
        create_account( N(runnerup5), config::system_account_name );

        deploy_contract();

        // Buy ram and stake cpu and net for each genesis accounts
        for( const auto& a : test_genesis ) {
            auto stake_quantity = a.initial_balance - 1000;

            auto r = delegate_bandwidth(N(rem.stake), a.aname, asset(stake_quantity));
            BOOST_REQUIRE( !r->except_ptr );
        }
        delegate_bandwidth(N(rem.stake), N(runnerup4), asset(300'000'0000));
        delegate_bandwidth(N(rem.stake), N(runnerup5), asset(300'000'0000));


        // register whales as producers
        const auto whales_as_producers = { N(b1), N(whale4), N(whale3), N(whale2) };
        for( const auto& producer : whales_as_producers ) {
            register_producer(producer);
        }

        auto producer_candidates = {
            N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
            N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
            N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ),
            N(runnerup1), N(runnerup2), N(runnerup3)
        };

        // Register producers
        for( auto pro : producer_candidates ) {
            register_producer(pro);
        }
        register_producer(N(runnerup4));
        register_producer(N(runnerup5));

        // Vote for producers
        auto votepro = [&]( account_name voter, vector<account_name> producers ) {
            std::sort( producers.begin(), producers.end() );
            base_tester::push_action(config::system_account_name, N(voteproducer), voter, mvo()
                ("voter",  name(voter))
                ("proxy", name(0) )
                ("producers", producers)
            );
        };
        votepro( N(b1), { N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                          N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                          N(prodo), N(prodp), N(prodq), N(prodr), N(prods), N(prodt), N(produ)} );
        votepro( N(whale2), {N(runnerup1), N(runnerup2), N(runnerup3)} );
        votepro( N(whale3), {N(proda), N(prodb), N(prodc), N(prodd), N(prode)} );
        votepro( N(whale4), {N(prodq), N(prodr), N(prods), N(prodt), N(produ)} );
        for( auto prod : producer_candidates ) {
            votepro(prod, { prod });
        }

        produce_blocks(4);
        produce_block();
        test_schedule({ "proda", "prodb", "prodc", "prodd", "prode", "prodf", "prodg", "prodh", "prodi", "prodj", "prodk",
                        "prodl", "prodm", "prodn", "prodo", "prodp", "prodr", "prods", "prodt", "produ", "runnerup1" });
        BOOST_TEST(get_rotated_producers().first == N(prodq)); //bp out
        BOOST_TEST(get_rotated_producers().second == N(runnerup1)); //standby in

        //add more standby
        votepro( N(whale2), { N(runnerup1), N(runnerup2), N(runnerup3), N(runnerup4), N(runnerup5) } );

        // make changes to top 21 producer list
        votepro( N(b1), { N(proda), N(prodb), N(prodc), N(prodd), N(prode), N(prodf), N(prodg),
                          N(prodh), N(prodi), N(prodj), N(prodk), N(prodl), N(prodm), N(prodn),
                          N(prodo), N(prodp), N(prodr), N(prods), N(prodt), N(produ),
                          N(runnerup3)} );
        produce_blocks_until_schedule_is_changed(2000);
        produce_blocks(2);
        test_schedule({ "proda", "prodb", "prodc", "prodd", "prode", "prodf", "prodg", "prodh", "prodi", "prodj", "prodk",
                        "prodl", "prodm", "prodn", "prodo", "prodp", "prodr", "prods", "prodt", "produ", "runnerup3" });
        //rotation must be dropped due to prodq loosing his votes
        BOOST_TEST(get_rotated_producers().first == name(0)); //bp out
        BOOST_TEST(get_rotated_producers().second == name(0)); //standby in

        produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(fc::seconds(12 * 3600));
        produce_blocks_until_schedule_is_changed(2000);
        produce_blocks(2);
        return;
        test_schedule({ "proda", "prodb", "prodc", "prodd", "prode", "prodf", "prodg", "prodh", "prodi", "prodj", "prodk",
                        "prodl", "prodm", "prodn", "prodo", "prodp", "prods", "prodt", "produ", "runnerup2", "runnerup3" });
        BOOST_TEST(get_rotated_producers().first == N(prodr)); //bp out
        BOOST_TEST(get_rotated_producers().second == N(runnerup2)); //standby in

    } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
