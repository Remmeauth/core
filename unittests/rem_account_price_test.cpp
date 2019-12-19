#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <boost/test/unit_test.hpp>
#pragma GCC diagnostic pop

#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/testing/tester.hpp>

#include <fc/exception/exception.hpp>
#include <fc/variant_object.hpp>

#include <contracts.hpp>

#include "eosio_system_tester.hpp"


#define PRINT_USAGE(account) { \
   auto rlm = control->get_resource_limits_manager(); \
   auto ram_usage = rlm.get_account_ram_usage(N(account)); \
   int64_t ram_bytes; \
   int64_t net_weight; \
   int64_t cpu_weight; \
   rlm.get_account_limits( N(account), ram_bytes, net_weight, cpu_weight); \
   const auto free_bytes = ram_bytes - ram_usage; \
   wdump((#account)(ram_usage)(free_bytes)(ram_bytes)(net_weight)(cpu_weight)); \
}

BOOST_AUTO_TEST_SUITE(rem_account_price_tests)
BOOST_FIXTURE_TEST_CASE(rem_account_price_test, rem_system::eosio_system_tester) {
    try {
        const auto min_account_stake = get_global_state()["min_account_stake"].as<int64_t>();
        BOOST_REQUIRE_EQUAL(min_account_stake, 100'0000u);

        const auto account_price = 50'0000;
        push_action( config::system_account_name, N(setminstake), mvo()("min_account_stake", account_price) );

        PRINT_USAGE(rem)
        PRINT_USAGE(rem.stake)

        cross_15_percent_threshold();
        produce_blocks(10);

        // everyone should be able to create account with given price
        {
            create_account_with_resources(
                N(testuser222),
                config::system_account_name,
                true,
                asset{ account_price }
            );

            produce_blocks(1);
        }
    } FC_LOG_AND_RETHROW()
}
BOOST_AUTO_TEST_SUITE_END()