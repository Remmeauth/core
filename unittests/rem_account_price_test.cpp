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

BOOST_AUTO_TEST_SUITE(rem_account_price_tests)
BOOST_FIXTURE_TEST_CASE(rem_account_price_test, rem_system::eosio_system_tester) {
    try {
        cross_15_percent_threshold();
        produce_blocks(10);

        // everyone should be able to create account with given price
        {
            const int64_t account_price = 50'0000;
            push_action( config::system_account_name, N(setminstake), mvo()("min_account_stake", account_price) );
            
            const auto min_account_stake = get_global_state()["min_account_stake"].as<int64_t>();
            BOOST_REQUIRE_EQUAL(min_account_stake, account_price);

            create_account_with_resources(
                N(testuser111),
                config::system_account_name,
                false,
                asset{ account_price }
            );

            produce_blocks(1);
        }

        // everyone should be able to create account with given price
        {
            const int64_t account_price = 10'0000;
            push_action( config::system_account_name, N(setminstake), mvo()("min_account_stake", account_price) );
            
            const auto min_account_stake = get_global_state()["min_account_stake"].as<int64_t>();
            BOOST_REQUIRE_EQUAL(min_account_stake, account_price);

            create_account_with_resources(
                N(testuser222),
                config::system_account_name,
                false,
                asset{ account_price }
            );

            produce_blocks(1);
        }
    } FC_LOG_AND_RETHROW()
}
BOOST_AUTO_TEST_SUITE_END()