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

class utils_tester : public TESTER {
public:
   utils_tester();

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

   auto addchain(const name &chain_id, const bool &input, const bool& output, const vector<permission_level>& level) {

      auto r = base_tester::push_action(N(rem.swap), N(addchain), level, mvo()
            ("chain_id", chain_id)
            ("input", input)
            ("output", output)
      );
      produce_block();
      return r;
   }

   auto validate_address(const name& user, const name& chain_id, const string& address) {
      auto r = base_tester::push_action(N(rem.utils), N(validateaddr), user, mvo()
              ("chain_id",  chain_id)
              ("address", address )
      );
      produce_block();
      return r;
   }

   auto set_swap_fee(const name& chain_id, const asset& fee) {
      auto r = base_tester::push_action(N(rem.utils), N(setswapfee), N(rem.utils), mvo()
              ("chain_id",  chain_id)
              ("fee", fee )
      );
      produce_block();
      return r;
   }

   fc::variant get_swap_fee_info( const name& chain_id ) {
      vector<char> data = get_row_by_account( N(rem.utils), N(rem.utils), N(swapfee), chain_id );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "swap_fee", data, abi_serializer_max_time );
   }

   asset get_balance( const account_name& act ) {
      return get_currency_balance(N(rem.token), symbol(CORE_SYMBOL), act);
   }

   void set_code_abi(const account_name& account, const vector<uint8_t>& wasm, const char* abi, const private_key_type* signer = nullptr) {
      wdump((account));
      set_code(account, wasm, signer);
      set_abi(account, abi, signer);
      if (account == N(rem.utils)) {
         const auto& accnt = control->db().get<account_object,by_name>( account );
         abi_def abi_definition;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi_definition), true);
         abi_ser.set_abi(abi_definition, abi_serializer_max_time);
      }
      produce_blocks();
   }

   abi_serializer abi_ser;
};

utils_tester::utils_tester() {
   // Create rem.msig and rem.token, rem.utils
   create_accounts({N(rem.msig), N(rem.token), N(rem.rex), N(rem.ram),
                    N(rem.ramfee), N(rem.stake), N(rem.swap), N(rem.bpay), N(rem.spay), N(rem.vpay), N(rem.saving), N(rem.utils) });

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
   //  - rem.utils (code: rem.utils)
   set_code_abi(N(rem.msig),
                contracts::rem_msig_wasm(),
                contracts::rem_msig_abi().data());//, &rem_active_pk);
   set_code_abi(N(rem.token),
                contracts::rem_token_wasm(),
                contracts::rem_token_abi().data()); //, &rem_active_pk);
   set_code_abi(N(rem.utils),
                contracts::rem_utils_wasm(),
                contracts::rem_utils_abi().data()); //, &rem_active_pk);
   set_code_abi(N(rem.swap),
                contracts::rem_swap_wasm(),
                contracts::rem_swap_abi().data()); //, &rem_active_pk);

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

   vector<permission_level> auths_level = { permission_level{config::system_account_name, config::active_name},
                                            permission_level{N(rem.swap), config::active_name}};
   addchain(N(ethropsten), true, true, auths_level);
}

BOOST_AUTO_TEST_SUITE(utils_tests)

BOOST_FIXTURE_TEST_CASE( validate_eth_address_test, utils_tester ) {
   name ethchainid = N(ethropsten);
   string ethaddress = "0x9fB8A18fF402680b47387AE0F4e38229EC64f098";
   try {
      validate_address(N(proda), ethchainid, ethaddress);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( validate_eth_address_test_without_hexpre, utils_tester ) {
   try {
      name ethchainid = N(ethropsten);
      string address = "9fB8A18fF402680b47387AE0F4e38229EC64f098";

      validate_address(N(proda), ethchainid, address);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( validate_eth_address_test_with_non_existed_chain_id, utils_tester ) {
   try {
      name ethchainid = N(nonexchain);
      string ethaddress = "0x9fB8A18fF402680b47387AE0F4e38229EC64f098";
      // not supported chain id
      BOOST_REQUIRE_THROW(validate_address(N(proda), ethchainid, ethaddress), eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( validate_eth_address_test_with_non_existed_account, utils_tester ) {
   try {
      name ethchainid = N(ethropsten);
      string ethaddress = "0x9fB8A18fF402680b47387AE0F4e38229EC64f098";
      // auth error
      BOOST_REQUIRE_THROW(validate_address(N(fail), ethchainid, ethaddress), transaction_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( validate_eth_address_test_with_address_invalid_length, utils_tester ) {
   try {
      name ethchainid = N(ethropsten);
      string address = "0x9f21f19180c8692eb";

      // invalid address length
      BOOST_REQUIRE_THROW(validate_address(N(proda), ethchainid, address), eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( validate_eth_address_test_with_address_invalid_symbol, utils_tester ) {
   try {
      name ethchainid = N(ethropsten);
      string address = "0x9fB8A18fF402680b47387AE0F4e38229EC64f09%";

      // invalid hex symbol in ethereum address
      BOOST_REQUIRE_THROW(validate_address(N(proda), ethchainid, address), eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( validate_eth_address_invalid_checksum, utils_tester ) {
   try {
      name ethchainid = N(ethropsten);
      // valid address all upper
      string address = "0x8617E340B3D01FA5F11F306F4090FD50E238070D";
      validate_address(N(proda), ethchainid, address);

      address = "0x9fB8A18fF402680b47387AE0F4e38229EC64f097";
      // invalid ethereum address checksum
      BOOST_REQUIRE_THROW(validate_address(N(proda), ethchainid, address), eosio_assert_message_exception);

      // valid address all lower
      address = "0xd423ae43105a0185c18f968cd8be0fa276689c04";
      // invalid ethereum address checksum
      BOOST_REQUIRE_THROW(validate_address(N(proda), ethchainid, address), eosio_assert_message_exception);
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
