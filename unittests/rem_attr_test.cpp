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

#include <functional>
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

using mvo = fc::mutable_variant_object;

struct genesis_account {
   account_name aname;
   uint64_t     initial_balance;
};


struct create_attribute_t {
   name     attr_name;
   int32_t  type;
   int32_t  privacy_type;
};

struct set_attr_t {
   name         issuer;
   name         receiver;
   name         attribute_name;
   std::string  value;

   set_attr_t(name issuer_, name receiver_, name attribute_name_, std::string value_)
      : issuer(issuer_), receiver(receiver_), attribute_name(attribute_name_), value(value_) {}

   virtual bool isValueOk(std::string actualValue) const = 0;
};

template<typename T>
struct set_attr_expected_t : public set_attr_t {
   T expectedValue;

   set_attr_expected_t(name issuer_, name receiver_, name attribute_name_, std::string value_, T expectedValue_)
      : set_attr_t(issuer_, receiver_, attribute_name_, value_), expectedValue(expectedValue_) {}

   virtual bool isValueOk(std::string actualValue) const {
      bytes b(actualValue.size() / 2);
      const auto size = fc::from_hex(actualValue, b.data(), b.size());

      const auto v = fc::raw::unpack<T>(b);
      return v == expectedValue;
   }
};


class attribute_tester : public TESTER {
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


    auto confirm_attr( name owner, name attribute_name ) {
        auto r = base_tester::push_action(N(rem.attr), N(confirm), owner, mvo()
            ("owner", owner)
            ("attribute_name", attribute_name)
        );
        produce_block();
        return r;
    }

    auto create_attr( name attr, int32_t type, int32_t ptype ) {
        auto r = base_tester::push_action(N(rem.attr), N(create), N(rem.attr), mvo()
            ("attribute_name", attr)
            ("type", type)
            ("ptype", ptype)
        );
        produce_block();
        return r;
    }

    auto set_attr( name issuer, name receiver, name attribute_name, std::string value ) {
        auto r = base_tester::push_action(N(rem.attr), N(setattr), issuer, mvo()
            ("issuer", issuer)
            ("receiver", receiver)
            ("attribute_name", attribute_name)
            ("value", value)
        );
        produce_block();
        return r;
    }

    auto unset_attr( name issuer, name receiver, name attribute_name ) {
        auto r = base_tester::push_action(N(rem.attr), N(unsetattr), issuer, mvo()
            ("issuer", issuer)
            ("receiver", receiver)
            ("attribute_name", attribute_name)
        );
        produce_block();
        return r;
    }

    fc::variant get_attribute_info( const account_name& attribute ) {
        vector<char> data = get_row_by_account( N(rem.attr), attribute, N(attrinfo), attribute );
        return abi_ser.binary_to_variant( "attribute_info", data, abi_serializer_max_time );
    }

    fc::variant get_account_attribute( const account_name& account, const account_name& attribute ) {
        vector<char> data = get_row_by_account( N(rem.attr), account, N(attributes), attribute );
        return abi_ser.binary_to_variant( "attribute_data", data, abi_serializer_max_time );
    }

    asset get_balance( const account_name& act ) {
         return get_currency_balance(N(rem.token), symbol(CORE_SYMBOL), act);
    }

    void set_code_abi(const account_name& account, const vector<uint8_t>& wasm, const char* abi, const private_key_type* signer = nullptr) {
       wdump((account));
        set_code(account, wasm, signer);
        set_abi(account, abi, signer);
        if (account == N(rem.attr)) {
           const auto& accnt = control->db().get<account_object,by_name>( account );
           abi_def abi_definition;
           BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi_definition), true);
           abi_ser.set_abi(abi_definition, abi_serializer_max_time);
        }
        produce_blocks();
    }

    abi_serializer abi_ser;
};

BOOST_AUTO_TEST_SUITE(attribute_tests)

BOOST_FIXTURE_TEST_CASE( attribute_test, attribute_tester ) {
    try {

        // Create rem.msig and rem.token
        create_accounts({N(rem.msig), N(rem.token), N(rem.ram), N(rem.ramfee), N(rem.stake), N(rem.vpay), N(rem.spay), N(rem.saving), N(rem.attr) });
        // Set code for the following accounts:
        //  - rem (code: rem.bios) (already set by tester constructor)
        //  - rem.msig (code: rem.msig)
        //  - rem.token (code: rem.token)
        // set_code_abi(N(rem.msig), contracts::rem_msig_wasm(), contracts::rem_msig_abi().data());//, &rem_active_pk);
        // set_code_abi(N(rem.token), contracts::rem_token_wasm(), contracts::rem_token_abi().data()); //, &rem_active_pk);

        set_code_abi(N(rem.msig),
                     contracts::rem_msig_wasm(),
                     contracts::rem_msig_abi().data());//, &rem_active_pk);
        set_code_abi(N(rem.token),
                     contracts::rem_token_wasm(),
                     contracts::rem_token_abi().data()); //, &rem_active_pk);
        set_code_abi(N(rem.attr),
                     contracts::rem_attr_wasm(),
                     contracts::rem_attr_abi().data()); //, &rem_active_pk);

        // Set privileged for rem.msig and rem.token
        set_privileged(N(rem.msig));
        set_privileged(N(rem.token));

        // Verify rem.msig and rem.token is privileged
        const auto& rem_msig_acc = get<account_metadata_object, by_name>(N(rem.msig));
        BOOST_TEST(rem_msig_acc.is_privileged() == true);
        const auto& rem_token_acc = get<account_metadata_object, by_name>(N(rem.token));
        BOOST_TEST(rem_token_acc.is_privileged() == true);


        // Create SYS tokens in rem.token, set its manager as rem
        auto max_supply = core_from_string("1000000000.0000");
        auto initial_supply = core_from_string("900000000.0000");
        create_currency(N(rem.token), config::system_account_name, max_supply);
        // Issue the genesis supply of 1 billion SYS tokens to rem.system
        issue(N(rem.token), config::system_account_name, config::system_account_name, initial_supply);

        auto actual = get_balance(config::system_account_name);
        BOOST_REQUIRE_EQUAL(initial_supply, actual);

        // Create genesis accounts

        std::vector<genesis_account> test_genesis( {
            {N(b1),       100'000'000'0000ll},
            {N(whale4),    40'000'000'0000ll},
            {N(whale3),    30'000'000'0000ll},
            {N(whale2),    20'000'000'0000ll},
            {N(proda),      1'000'000'0000ll},
            {N(prodb),      1'000'000'0000ll},
            {N(prodc),      1'000'000'0000ll}
        });
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

        std::vector<create_attribute_t> attributes = {
            { .attr_name=N(crosschain),  .type=3,  .privacy_type=1 },  // type: ChainAccount, privacy type: PublicPointer
            { .attr_name=N(tags),        .type=9,  .privacy_type=4 },  // type: Set,          privacy type: PrivateConfirmedPointer
            { .attr_name=N(creator),     .type=0,  .privacy_type=3 },  // type: Bool,         privacy type: PrivatePointer
            { .attr_name=N(name),        .type=4,  .privacy_type=0 },  // type: UTFString,    privacy type: SelfAssigned
            { .attr_name=N(largeint),    .type=2,  .privacy_type=2 },  // type: LargeInt,     privacy type: PublicConfirmedPointer
        };
        for (const auto& a: attributes) {
            create_attr(a.attr_name, a.type, a.privacy_type);
            const auto attr_info = get_attribute_info(a.attr_name);
            BOOST_TEST( a.attr_name.to_string() == attr_info["attribute_name"].as_string() );
            BOOST_TEST( a.type == attr_info["type"].as_int64() );
            BOOST_TEST( a.privacy_type == attr_info["ptype"].as_int64() );
        }
        BOOST_REQUIRE_THROW(base_tester::push_action(N(rem.attr), N(create), N(b1), mvo()("attribute_name", name(N(fail)))("type", 0)("ptype", 0)),
            missing_auth_exception);
        BOOST_REQUIRE_THROW(create_attr(N(fail), -1, 0), eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(create_attr(N(fail), 10, 0), eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(create_attr(N(fail), 0, -1), eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(create_attr(N(fail), 0, 5), eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(create_attr(N(tags), 0, 0), eosio_assert_message_exception);

        set_attr_expected_t<std::pair<fc::sha256, name>> attr1(N(proda), N(proda), N(crosschain),
           "00000000000000000000000000000000000000000000000000000000000000ff000000000000A4BA",
           std::pair<chain_id_type, name>("00000000000000000000000000000000000000000000000000000000000000ff", N(rem)));
        set_attr_expected_t<std::pair<fc::sha256, name>> attr2(N(prodb), N(proda), N(crosschain),
            "000000000000000000000000000000000000000000000000000000000000ffff0000000000EA3055",
            std::pair<chain_id_type, name>("000000000000000000000000000000000000000000000000000000000000ffff", N(eosio)));
        set_attr_expected_t<std::set<std::pair<std::string, std::string>>> attr3(N(rem.attr), N(proda), N(tags),
            "03013101320133013401350136",
            std::set<std::pair<std::string, std::string>>{ std::pair("1", "2"), std::pair("3", "4"), std::pair("5", "6") });
        set_attr_expected_t<std::set<std::pair<std::string, std::string>>> attr4(N(rem.attr), N(rem.attr), N(tags),
            "02013103323334023536023738",
            std::set<std::pair<std::string, std::string>>{ std::pair("1", "234"), std::pair("56", "78") });
        set_attr_expected_t<bool> attr5(N(rem.attr), N(prodb), N(creator),
            "00",
            false);
        set_attr_expected_t<bool> attr6(N(rem.attr), N(rem.attr), N(creator),
            "01",
            true);
        set_attr_expected_t<std::string> attr7(N(prodb), N(prodb), N(name),
            "06426c61697a65",
            "Blaize");
        set_attr_expected_t<int64_t> attr8(N(prodb), N(prodb), N(largeint),
            "0000000000000080",
            -9223372036854775808);
        set_attr_expected_t<int64_t> attr9(N(proda), N(prodb), N(largeint),
           "ffffffffffffffff",
           -1);
        for (auto& attr: std::vector<std::reference_wrapper<set_attr_t>>{ attr1, attr2, attr3, attr4, attr5, attr6, attr7, attr8, attr9 }) {
            auto& attr_ref = attr.get();
            set_attr(attr_ref.issuer, attr_ref.receiver, attr_ref.attribute_name, attr_ref.value);

            const auto attribute_obj = get_account_attribute(attr_ref.receiver, attr_ref.attribute_name);
            wdump((attribute_obj));
            BOOST_TEST(attr_ref.isValueOk(attribute_obj["data"].as_string()));
        }

        //issuer must have authorization
        BOOST_REQUIRE_THROW(base_tester::push_action(N(rem.attr), N(setattr), N(proda), mvo()("issuer", name(N(b1)))("receiver", name(N(prodb)))("attribute_name", name(N(crosschain)))("value", "00000000000000000000000000000000000000000000000000000000000000ff000000000000A4BA")),
            missing_auth_exception);
        //empty value
        BOOST_REQUIRE_THROW(set_attr(N(proda), N(prodb), N(crosschain), ""), eosio_assert_message_exception);
        //privacy checks
        BOOST_REQUIRE_THROW(set_attr(N(prodb), N(proda), N(tags), "03013101320133013401350136"), eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(set_attr(N(proda), N(proda), N(creator), "01"), eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(set_attr(N(rem.attr), N(proda), N(name), "06426c61697a65"), eosio_assert_message_exception);

        BOOST_TEST(get_account_attribute(N(proda),        N(crosschain)) ["confirmed"].as_bool() == true);
        BOOST_TEST(get_account_attribute(N(proda),              N(tags)) ["confirmed"].as_bool() == false);
        BOOST_TEST(get_account_attribute(N(rem.attr),           N(tags)) ["confirmed"].as_bool() == false);
        BOOST_TEST(get_account_attribute(N(prodb),           N(creator)) ["confirmed"].as_bool() == true);
        BOOST_TEST(get_account_attribute(N(rem.attr),        N(creator)) ["confirmed"].as_bool() == true);
        BOOST_TEST(get_account_attribute(N(prodb),              N(name)) ["confirmed"].as_bool() == true);
        BOOST_TEST(get_account_attribute(N(prodb),          N(largeint)) ["confirmed"].as_bool() == false);

        BOOST_REQUIRE_THROW(base_tester::push_action(N(rem.attr), N(confirm), N(rem.attr), mvo()("owner", name(N(proda)))("attribute_name", name(N(tags)))),
            missing_auth_exception);
        BOOST_REQUIRE_THROW(confirm_attr(N(proda), N(nonexisting)), eosio_assert_message_exception);

        confirm_attr(N(proda),     N(tags));
        confirm_attr(N(rem.attr),  N(tags));
        confirm_attr(N(prodb),     N(largeint));
        BOOST_TEST(get_account_attribute(N(proda),              N(tags)) ["confirmed"].as_bool() == true);
        BOOST_TEST(get_account_attribute(N(rem.attr),           N(tags)) ["confirmed"].as_bool() == true);
        BOOST_TEST(get_account_attribute(N(prodb),          N(largeint)) ["confirmed"].as_bool() == true);

        BOOST_REQUIRE_THROW(unset_attr(N(proda),      N(proda),      N(nonexisting)),  eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(unset_attr(N(prodc),      N(prodc),      N(crosschain)),   eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(unset_attr(N(proda),      N(proda),      N(tags)),         eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(unset_attr(N(prodb),      N(rem.attr),   N(tags)),         eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(unset_attr(N(proda),      N(proda),      N(creator)),      eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(unset_attr(N(rem.attr),   N(proda),      N(name)),         eosio_assert_message_exception);

        unset_attr(N(prodb),      N(proda),      N(crosschain));
        unset_attr(N(rem.attr),   N(proda),      N(tags));
        unset_attr(N(rem.attr),   N(rem.attr),   N(tags));
        unset_attr(N(rem.attr),   N(prodb),      N(creator));
        unset_attr(N(rem.attr),   N(rem.attr),   N(creator));
        unset_attr(N(prodb),      N(prodb),      N(name));
        unset_attr(N(proda),      N(prodb),      N(largeint));
        BOOST_REQUIRE_THROW(get_account_attribute(N(proda),      N(crosschain)),   unpack_exception);
        BOOST_REQUIRE_THROW(get_account_attribute(N(proda),      N(tags)),         unpack_exception);
        BOOST_REQUIRE_THROW(get_account_attribute(N(rem.attr),   N(tags)),         unpack_exception);
        BOOST_REQUIRE_THROW(get_account_attribute(N(prodb),      N(creator)),      unpack_exception);
        BOOST_REQUIRE_THROW(get_account_attribute(N(rem.attr),   N(creator)),      unpack_exception);
        BOOST_REQUIRE_THROW(get_account_attribute(N(prodb),      N(name)),         unpack_exception);
        BOOST_REQUIRE_THROW(get_account_attribute(N(prodb),      N(largeint)),     unpack_exception);

    } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
