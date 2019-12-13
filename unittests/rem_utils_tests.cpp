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

   auto addchain(const name &chain_id, const bool &input, const bool& output, const int64_t &in_swap_min_amount,
                 const int64_t &out_swap_min_amount, const vector<permission_level>& level) {

      auto r = base_tester::push_action(N(rem.swap), N(addchain), level, mvo()
            ("chain_id", chain_id)
            ("input", input)
            ("output", output)
            ("in_swap_min_amount", in_swap_min_amount)
            ("out_swap_min_amount", out_swap_min_amount)
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
   addchain(N(ethropsten), true, true, 5000000, 5000000, auths_level);
}

BOOST_AUTO_TEST_SUITE(rem_utils_tests)

BOOST_FIXTURE_TEST_CASE( validate_eth_address_test, utils_tester ) {
   try {
      name ethchainid = N(ethropsten);
      vector<string> valid_addresses = {
         "0xd18a02cafC6715c2e096636aB3349E4B79FAeCE7", "0xeB5F897477362945af744EbB244be03FbA0248F6",
         "0x81b7E08F65Bdf5648606c89998A9CC8164397647", "0xCeAdcdA44010fe724Ff92Efc2cbCE9B5cf01842C",
         "0xed092A687C65D12abd98420c57dE86694D7B682C", "0xc18De3aC4E50f9090435112866564dcFFEa7E2Fb",
         "0x2a67090E67BcD5c1cc580f43a54DF5030797f1Bf", "0x61664145ae31775A634B237e527EB1472028B6B2",
         "0xc9BCeB47b9795f6bbd190C30959Cd5E73792f2D3", "0x6B770956515f615A21E395cb97010100bAB5d1E6",
         "0x48827A804e170A855cdf6Fa902728B517b982cDd", "0x5e904e42A2E5ff3Bae010c32b02CdcD988920c71",
         "0xc18De3aC4E50f9090435112866564dcFFEa7E2Fb", "0xed092A687C65D12abd98420c57dE86694D7B682C",
         "0x81b7E08F65Bdf5648606c89998A9CC8164397647", "0x48827A804e170A855cdf6Fa902728B517b982cDd",
         "0x759dC16D1a8ab2D95F90cCd456774b3dF0c97CB8", "0x45cb76afdb1e30b7f1eca0c3faf0ea2619c0ea33",
      };
      for (const auto &address: valid_addresses) {
         validate_address(N(proda), ethchainid, address);
      }
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE( validate_eth_address_test_without_hexpre, utils_tester ) {
   try {
      name ethchainid = N(ethropsten);
      vector<string> valid_addresses = {
         "d18a02cafC6715c2e096636aB3349E4B79FAeCE7", "eB5F897477362945af744EbB244be03FbA0248F6",
         "81b7E08F65Bdf5648606c89998A9CC8164397647", "CeAdcdA44010fe724Ff92Efc2cbCE9B5cf01842C",
         "ed092A687C65D12abd98420c57dE86694D7B682C", "c18De3aC4E50f9090435112866564dcFFEa7E2Fb",
         "2a67090E67BcD5c1cc580f43a54DF5030797f1Bf", "61664145ae31775A634B237e527EB1472028B6B2",
         "c9BCeB47b9795f6bbd190C30959Cd5E73792f2D3", "6B770956515f615A21E395cb97010100bAB5d1E6",
         "48827A804e170A855cdf6Fa902728B517b982cDd", "5e904e42A2E5ff3Bae010c32b02CdcD988920c71",
         "c18De3aC4E50f9090435112866564dcFFEa7E2Fb", "ed092A687C65D12abd98420c57dE86694D7B682C",
         "81b7E08F65Bdf5648606c89998A9CC8164397647", "48827A804e170A855cdf6Fa902728B517b982cDd",
         "759dC16D1a8ab2D95F90cCd456774b3dF0c97CB8", "45cb76afdb1e30b7f1eca0c3faf0ea2619c0ea33",
      };
      for (const auto &address: valid_addresses) {
         validate_address(N(proda), ethchainid, address);
      }
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

BOOST_FIXTURE_TEST_CASE( validate_eth_address_all_lower, utils_tester ) {
   try {
      name ethchainid = N(ethropsten);
      // valid address all upper
      string address = "0x8617E340B3D01FA5F11F306F4090FD50E238070D";
      validate_address(N(proda), ethchainid, address);

      address = "0x9fB8A18fF402680b47387AE0F4e38229EC64f097";
      // invalid ethereum address checksum
      BOOST_REQUIRE_THROW(validate_address(N(proda), ethchainid, address), eosio_assert_message_exception);

      // valid addresses all lower
      vector<string> valid_addresses = {
         "0xd18a02cafc6715c2e096636ab3349e4b79faece7", "0xeb5f897477362945af744ebb244be03fba0248f6",
         "0x81b7e08f65bdf5648606c89998a9cc8164397647", "0xceadcda44010fe724ff92efc2cbce9b5cf01842c",
         "0xed092a687c65d12abd98420c57de86694d7b682c", "0xc18de3ac4e50f9090435112866564dcffea7e2fb",
         "0x2a67090e67bcd5c1cc580f43a54df5030797f1bf", "0x61664145ae31775a634b237e527eb1472028b6b2",
         "0xc9bceb47b9795f6bbd190c30959cd5e73792f2d3", "0x6b770956515f615a21e395cb97010100bab5d1e6",
         "0x48827a804e170a855cdf6fa902728b517b982cdd", "0x5e904e42a2e5ff3bae010c32b02cdcd988920c71",
         "0xc18de3ac4e50f9090435112866564dcffea7e2fb", "0xed092a687c65d12abd98420c57de86694d7b682c",
         "0x81b7e08f65bdf5648606c89998a9cc8164397647", "0x48827a804e170a855cdf6fa902728b517b982cdd",
         "0x759dc16d1a8ab2d95f90ccd456774b3df0c97cb8", "0x45cb76afdb1e30b7f1eca0c3faf0ea2619c0ea33",
      };
      for (const auto &address: valid_addresses) {
         validate_address(N(proda), ethchainid, address);
      }
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
