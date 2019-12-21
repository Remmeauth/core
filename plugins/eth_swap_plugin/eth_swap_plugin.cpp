/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosio/eth_swap_plugin/eth_swap_plugin.hpp>
#include <eosio/eth_swap_plugin/my_web3.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/wast_to_wasm.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/exception/exception.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/io/json.hpp>

#include <Inline/BasicTypes.h>
#include <IR/Module.h>
#include <IR/Validate.h>
#include <WAST/WAST.h>
#include <WASM/WASM.h>
#include <Runtime/Runtime.h>

#include <contracts.hpp>

#include <sstream>
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/algorithm/clamp.hpp>
#include <boost/optional/optional.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>

#include <iostream>
#include <fstream>


namespace eosio {
   static appbase::abstract_plugin& _eth_swap_plugin = app().register_plugin<eth_swap_plugin>();

   using namespace eosio::chain;

class eth_swap_plugin_impl {
  public:
    std::vector<fc::crypto::private_key>    _swap_signing_key;
    std::vector<name>                       _swap_signing_account;
    std::vector<std::string>                _swap_signing_permission;
    std::string                _eth_wss_provider;

    void start_monitor() {

      while(eth_swap_contract_address.empty() && return_chain_id.empty()) {
        //uint32_t i = 0;
        try {
          chain_apis::read_only::get_table_rows_params params = {};
          params.json = true;
          params.code = N(rem.swap);
          params.scope = std::string("rem.swap");
          params.table = N(swapparams);

          chain_apis::read_only::get_table_rows_result result = app().get_plugin<chain_plugin>().get_read_only_api().get_table_rows(params);
          for( const auto& item : result.rows ) {
            eth_swap_contract_address = item["eth_swap_contract_address"].as<std::string>();
            return_chain_id = item["eth_return_chainid"].as<std::string>();
            //try {ilog("in_swap_fee: ${i}", ("i", item["in_swap_fee"]));} FC_LOG_AND_DROP()
            //try {ilog("out_swap_min_amount: ${i}", ("i", item["out_swap_min_amount"]));} FC_LOG_AND_DROP()
            //try {ilog("chain_id: ${i}", ("i", item["chain_id"]));} FC_LOG_AND_DROP()
            //ilog("counter: ${i}", ("i", i++));
          }
        } FC_LOG_AND_DROP()
        if( eth_swap_contract_address.empty() && return_chain_id.empty() )
          sleep(wait_for_swapparams);
      }
      ilog("eth swap contract address: ${i}", ("i", eth_swap_contract_address));
      ilog("eth return chain id: ${i}", ("i", return_chain_id));

      std::thread t([=](){
          init_prev_swap_requests();
      });
      t.detach();

      uint64_t last_processed_block = 0;
      while(true) {
        try {
            my_web3 my_w3(this->_eth_wss_provider);
            uint64_t last_block_dec = my_w3.get_last_block_num();
            uint64_t from_block_dec = std::max(last_block_dec - long_polling_blocks_per_filter, last_processed_block);

            std::stringstream stream;
            stream << std::hex << from_block_dec;
            std::string from_block( "0x" + stream.str() );
            stream.str("");
            stream.clear();

            stream << std::hex << (last_block_dec - min_tx_confirmations);
            std::string to_block( "0x" + stream.str() );
            stream.str("");
            stream.clear();

            std::string request_swap_filter_id = my_w3.new_filter(eth_swap_contract_address, from_block, to_block, "[\""+string(eth_swap_request_event)+"\"]");
            std::string filter_logs = my_w3.get_filter_logs(request_swap_filter_id);
            my_w3.uninstall_filter(request_swap_filter_id);
            std::vector<swap_event_data> prev_swap_requests = get_prev_swap_events(filter_logs);

            push_txs(prev_swap_requests, &last_processed_block);

            sleep(long_polling_period);
        } FC_LOG_WAIT_AND_CONTINUE()
      }
    }

    void push_txs(const std::vector<swap_event_data>& swap_requests, uint64_t* eth_block_number_ptr) {
        try {
          for (std::vector<swap_event_data>::const_iterator it = swap_requests.begin() ; it != swap_requests.end(); ++it) {
              swap_event_data data = *it;
              std::string txid = data.txid;
              push_init_swap_transaction(data, eth_block_number_ptr);
          }
        } FC_LOG_AND_RETHROW()
    }

    void init_prev_swap_requests() {
        uint64_t last_block_num = 0, min_block_dec, to_block_dec;
        while(last_block_num == 0) {
          try {
            my_web3 my_w3(this->_eth_wss_provider);
            last_block_num = my_w3.get_last_block_num();
            min_block_dec = last_block_num - eth_events_window_length;
            to_block_dec = last_block_num - min_tx_confirmations;
          } FC_LOG_WAIT_AND_CONTINUE()
        }

        while(to_block_dec > min_block_dec) {
          try {
            my_web3 my_w3(this->_eth_wss_provider);

            while (to_block_dec > min_block_dec) {

              std::stringstream stream;
              stream << std::hex << min(min_block_dec, to_block_dec - blocks_per_filter);
              std::string from_block( "0x" + stream.str() );
              stream.str("");
              stream.clear();

              stream << std::hex << to_block_dec;
              std::string to_block = "0x" + stream.str();

              std::string request_swap_filter_id = my_w3.new_filter(eth_swap_contract_address, from_block, to_block, "[\""+string(eth_swap_request_event)+"\"]");
              std::string filter_logs = my_w3.get_filter_logs(request_swap_filter_id);
              my_w3.uninstall_filter(request_swap_filter_id);
              std::vector<swap_event_data> prev_swap_requests = get_prev_swap_events(filter_logs);
              uint64_t eth_block_number;
              push_txs(prev_swap_requests, &eth_block_number);
              to_block_dec -= (blocks_per_filter-1);
            }

          } FC_LOG_WAIT_AND_CONTINUE()
        }
    }

    void push_init_swap_transaction(const swap_event_data& data, uint64_t* eth_block_number_ptr) {
        bool is_tx_sent = false;
        uint32_t push_tx_attempt = 0;
        while(!is_tx_sent) {
            uint32_t slot = (data.timestamp * 1000 - block_timestamp_epoch) / block_interval_ms;
            if(push_tx_attempt) {
              wlog("Retrying to push init swap transaction(${txid}, ${pubkey}, ${amount}, ${ret_addr}, ${ret_chainid}, ${timestamp})",
              ("txid", data.txid)("pubkey", data.swap_pubkey)("amount", data.amount)
              ("ret_addr", data.return_address)("ret_chainid", data.return_chain_id)("timestamp", epoch_block_timestamp(slot)));
            }
            std::vector<signed_transaction> trxs;
            trxs.reserve(2);

            controller& cc = app().get_plugin<chain_plugin>().chain();
            auto chainid = app().get_plugin<chain_plugin>().get_chain_id();

            if( data.chain_id != std::string(chainid) ) {
                elog("Invalid chain identifier in init swap transaction(${chain_id}, ${txid}, ${pubkey}, ${amount}, ${ret_addr}, ${ret_chainid}, ${timestamp})",
                ("chain_id", data.chain_id)("txid", data.txid)("pubkey", data.swap_pubkey)("amount", data.amount)
                ("ret_addr", data.return_address)("ret_chainid", data.return_chain_id)("timestamp", epoch_block_timestamp(slot)));
                return;
            }
            signed_transaction trx;
            for(size_t i = 0; i < this->_swap_signing_key.size(); i++) {
              trx.actions.emplace_back(vector<chain::permission_level>{{this->_swap_signing_account[i],name(this->_swap_signing_permission[i])}},
                init{this->_swap_signing_account[i],
                  data.txid,
                  data.swap_pubkey,
                  uint64_to_rem_asset(data.amount),
                  data.return_address,
                  data.return_chain_id,
                  epoch_block_timestamp(slot)});

              trx.expiration = cc.head_block_time() + fc::seconds(init_swap_expiration_time);
              trx.set_reference_block(cc.head_block_id());
              trx.max_net_usage_words = 5000;
              trx.sign(this->_swap_signing_key[i], chainid);
              trxs.emplace_back(std::move(trx));
            }
            try {
               auto trxs_copy = std::make_shared<std::decay_t<decltype(trxs)>>(std::move(trxs));
               app().post(priority::low, [trxs_copy, &is_tx_sent, data, slot, eth_block_number_ptr]() {
                 for (size_t i = 0; i < trxs_copy->size(); ++i) {
                     name account = trxs_copy->at(i).first_authorizer();
                     app().get_plugin<chain_plugin>().accept_transaction( std::make_shared<packed_transaction>(trxs_copy->at(i)),
                     [&is_tx_sent, data, slot, account, eth_block_number_ptr](const fc::static_variant<fc::exception_ptr, transaction_trace_ptr>& result){
                       is_tx_sent = true;
                       *eth_block_number_ptr = data.block_number;
                       if (result.contains<fc::exception_ptr>()) {
                          std::string err_str = result.get<fc::exception_ptr>()->to_string();
                          //if ( err_str.find("swap already canceled") == string::npos && err_str.find("swap already finished") == string::npos &&
                            //   err_str.find("approval already exists") == string::npos )
                              elog("${prod} failed to push init swap transaction(${txid}, ${pubkey}, ${amount}, ${ret_addr}, ${ret_chainid}, ${timestamp}): ${res}",
                              ("prod", account)( "res", result.get<fc::exception_ptr>()->to_string() )("txid", data.txid)("pubkey", data.swap_pubkey)("amount", data.amount)
                              ("ret_addr", data.return_address)("ret_chainid", data.return_chain_id)("timestamp", epoch_block_timestamp(slot)));
                       } else {
                          if (result.contains<transaction_trace_ptr>() && result.get<transaction_trace_ptr>()->receipt) {
                              auto trx_id = result.get<transaction_trace_ptr>()->id;
                              ilog("${prod} pushed init swap transaction(${txid}, ${pubkey}, ${amount}, ${ret_addr}, ${ret_chainid}, ${timestamp}): ${id}",
                              ("prod", account)( "id", trx_id )("txid", data.txid)("pubkey", data.swap_pubkey)("amount", data.amount)
                              ("ret_addr", data.return_address)("ret_chainid", data.return_chain_id)("timestamp", epoch_block_timestamp(slot)));
                          }
                       }
                    });
                 }
               });
            } FC_LOG_AND_DROP()
            sleep(wait_for_accept_tx);
            if(!is_tx_sent) {
              sleep(retry_push_tx_time);
            }
            push_tx_attempt++;
        }
    }
};

eth_swap_plugin::eth_swap_plugin():my(new eth_swap_plugin_impl()){}
eth_swap_plugin::~eth_swap_plugin(){}

void eth_swap_plugin::set_program_options(options_description&, options_description& cfg) {
  cfg.add_options()
        ("eth-wss-provider", bpo::value<std::string>(),
         "Ethereum websocket provider. For example wss://mainnet.infura.io/ws/v3/<infura_id>")
        ("swap-authority", bpo::value<std::vector<std::string>>(),
         "Account name and permission to authorize init swap actions. For example blockproducer1@active")
        ("swap-signing-key", bpo::value<std::vector<std::string>>(),
         "A private key to sign init swap actions")

         //("eth_swap_contract_address", bpo::value<std::string>(), "")
         ("eth_swap_request_event", bpo::value<std::string>()->default_value(eth_swap_request_event), "")
         //("return_chain_id", bpo::value<std::string>(), "")

         ("eth_events_window_length", bpo::value<uint32_t>()->default_value(eth_events_window_length), "")
         ("blocks_per_filter", bpo::value<uint32_t>()->default_value(blocks_per_filter), "")
         ("check_tx_confirmations_times", bpo::value<uint32_t>()->default_value(check_tx_confirmations_times), "")
         ("min_tx_confirmations", bpo::value<uint32_t>()->default_value(min_tx_confirmations), "")

         ("long_polling_blocks_per_filter", bpo::value<uint32_t>()->default_value(long_polling_blocks_per_filter), "")
         ("long_polling_period", bpo::value<uint32_t>()->default_value(long_polling_period), "")

         ("init_swap_expiration_time", bpo::value<uint32_t>()->default_value(init_swap_expiration_time), "")
         ("retry_push_tx_time", bpo::value<uint32_t>()->default_value(retry_push_tx_time), "")
         ("start_monitor_delay", bpo::value<uint32_t>()->default_value(start_monitor_delay), "")
        ;
}

void eth_swap_plugin::plugin_initialize(const variables_map& options) {
    try {
      std::vector<std::string> swap_auth = options.at( "swap-authority" ).as<std::vector<std::string>>();
      std::vector<std::string> swap_signing_key = options.at( "swap-signing-key" ).as<std::vector<std::string>>();
      for(size_t i = 0; i < swap_auth.size(); i++) {
        auto space_pos = swap_auth[i].find('@');
        EOS_ASSERT( (space_pos != std::string::npos), chain::plugin_config_exception,
                    "invalid authority" );
        std::string permission = swap_auth[i].substr( space_pos + 1 );
        std::string account = swap_auth[i].substr( 0, space_pos );

        struct name swap_signing_account(account);
        my->_swap_signing_account.push_back(swap_signing_account);
        my->_swap_signing_permission.push_back(permission);
        my->_swap_signing_key.push_back(fc::crypto::private_key( swap_signing_key[std::min(i, swap_signing_key.size()-1)] ));
      }

      //std::string prefix = "wss://";
      my->_eth_wss_provider = options.at( "eth-wss-provider" ).as<std::string>();
      //if(my->_eth_wss_provider.substr(0, prefix.size()) != prefix)
        //throw InvalidWssLinkException("Invalid ethereum node connection link. Should start with " + prefix);

      //eth_swap_contract_address = options.at( "eth_swap_contract_address" ).as<std::string>();
      eth_swap_request_event    = options.at( "eth_swap_request_event" ).as<std::string>();
      //return_chain_id           = options.at( "return_chain_id" ).as<std::string>();

      eth_events_window_length = options.at( "eth_events_window_length" ).as<uint32_t>();
      blocks_per_filter = options.at( "blocks_per_filter" ).as<uint32_t>();

      check_tx_confirmations_times = options.at( "check_tx_confirmations_times" ).as<uint32_t>();
      min_tx_confirmations = options.at( "min_tx_confirmations" ).as<uint32_t>();

      long_polling_blocks_per_filter = options.at( "long_polling_blocks_per_filter" ).as<uint32_t>();
      long_polling_period = options.at( "long_polling_period" ).as<uint32_t>();

      init_swap_expiration_time = options.at( "init_swap_expiration_time" ).as<uint32_t>();
      retry_push_tx_time = options.at( "retry_push_tx_time" ).as<uint32_t>();

      start_monitor_delay = options.at( "start_monitor_delay" ).as<uint32_t>();
    } FC_LOG_AND_RETHROW()
}

void eth_swap_plugin::plugin_startup() {
  ilog("Ethereum swap plugin started");
  // struct get_table_rows_params {
  //    bool        json = false;
  //    name        code;
  //    string      scope;
  //    name        table;
  //    string      table_key;
  //    string      lower_bound;
  //    string      upper_bound;
  //    uint32_t    limit = 10;
  //    string      key_type;  // type of key specified by index_position
  //    string      index_position; // 1 - primary (first), 2 - secondary index (in order defined by multi_index), 3 - third index, etc
  //    string      encode_type{"dec"}; //dec, hex , default=dec
  //    optional<bool>  reverse;
  //    optional<bool>  show_payer; // show RAM pyer
  //  };
  //
  // struct get_table_rows_result {
  //    vector<fc::variant> rows; ///< one row per item, either encoded as hex String or JSON object
  //    bool                more = false; ///< true if last element in data is not the end and sizeof data() < limit
  //    string              next_key; ///< fill lower_bound with this value to fetch more rows
  // };
  //
  // get_table_rows_result get_table_rows( const get_table_rows_params& params )const;

  try {
    my_web3 my_w3(my->_eth_wss_provider);
    ilog("last eth block: " + to_string(my_w3.get_last_block_num()));
  } FC_LOG_AND_RETHROW()

  std::thread t2([=](){
      sleep(start_monitor_delay);
      try {
        my->start_monitor();
      } FC_LOG_AND_RETHROW()
  });
  t2.detach();
}

void eth_swap_plugin::plugin_shutdown() {
}

std::string hex_to_string(const std::string& input) {
  static const char* const lut = "0123456789abcdef";
  size_t len = input.length();
  if (len & 1) throw;
  std::string output;
  output.reserve(len / 2);
  for (size_t i = 0; i < len; i += 2) {
    char a = input[i];
    const char* p = std::lower_bound(lut, lut + 16, a);
    if (*p != a) throw;
    char b = input[i + 1];
    const char* q = std::lower_bound(lut, lut + 16, b);
    if (*q != b) throw;
    output.push_back(((p - lut) << 4) | (q - lut));
  }
  return output;
}

swap_event_data* parse_swap_event_hex(const std::string& hex_data, swap_event_data* data) {

    if( hex_data.length() != request_swap_hex_data_length )
        return nullptr;

    std::string chain_id = hex_data.substr(0, 64);
    std::string amount_hex = hex_data.substr(64*2, 64);
    std::string return_address = hex_data.substr(64*3 + 24, 40);
    std::string timestamp = hex_data.substr(64*4, 64);
    std::string swap_pubkey = hex_to_string(hex_data.substr(64*6, 106));

    std::stringstream ss;

    ss << std::hex << amount_hex;
    unsigned long long amount_dec;
    ss >> amount_dec;

    ss.str(std::string());
    ss.clear();

    ss << std::hex << timestamp;
    time_t timestamp_dec;
    ss >> timestamp_dec;

    data->chain_id = chain_id;
    data->swap_pubkey = swap_pubkey;
    data->amount = amount_dec;
    data->return_address = return_address;
    data->timestamp = timestamp_dec;

    return data;
}

swap_event_data* get_swap_event_data( boost::property_tree::ptree root,
                                      swap_event_data* data,
                                      const char* data_key,
                                      const char* txid_key,
                                      const char* block_number_key ) {
    boost::optional<string> hex_data_opt = root.get_optional<string>(data_key);
    boost::optional<string> txid = root.get_optional<string>(txid_key);
    boost::optional<string> block_number = root.get_optional<string>(block_number_key);
    if( !hex_data_opt )
        return nullptr;
    string hex_data = hex_data_opt.get();
    parse_swap_event_hex(hex_data.substr(2), data);

    data->return_chain_id = return_chain_id;
    data->txid = txid.get().substr(2);

    std::stringstream ss;
    ss << std::hex << block_number.get().substr(2);
    uint64_t block_number_dec;
    ss >> block_number_dec;
    data->block_number = block_number_dec;

    return data;
}

swap_event_data* get_swap_event_data( const std::string& event_str,
                                      swap_event_data* data,
                                      const char* data_key,
                                      const char* txid_key,
                                      const char* block_number_key ) {
    boost::iostreams::stream<boost::iostreams::array_source> stream(event_str.c_str(), event_str.size());
    namespace pt = boost::property_tree;
    pt::ptree root;
    pt::read_json(stream, root);
    return get_swap_event_data(root, data, data_key, txid_key, block_number_key);
}

asset uint64_to_rem_asset(unsigned long long amount) {
    std::string amount_dec_rem(to_string(amount));
    amount_dec_rem.insert (amount_dec_rem.end()-4,'.');
    amount_dec_rem += " ";
    amount_dec_rem += rem_token_id;
    return asset::from_string(amount_dec_rem);
}

std::vector<swap_event_data> get_prev_swap_events(const std::string& logs) {
    vector<swap_event_data> swap_events;

    boost::iostreams::stream<boost::iostreams::array_source> stream(logs.c_str(), logs.size());
    namespace pt = boost::property_tree;
    pt::ptree root;
    pt::read_json(stream, root);

    for (pt::ptree::value_type &log : root.get_child("result")) {
        const std::string& key = log.first; // key
        const boost::property_tree::ptree& subtree = log.second;

        boost::optional<std::string> data_opt = subtree.get_optional<std::string>("data");
        boost::optional<std::string> txid_opt = subtree.get_optional<std::string>("transactionHash");

        if(data_opt && txid_opt) {
            swap_event_data event_data;
            get_swap_event_data(subtree, &event_data, "data", "transactionHash", "blockNumber");
            swap_events.push_back(event_data);
        }
        else
            continue;
    }
    return swap_events;
}

}
