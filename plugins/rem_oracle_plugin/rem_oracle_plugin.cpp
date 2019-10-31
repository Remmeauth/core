/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosio/rem_oracle_plugin/rem_oracle_plugin.hpp>
#include <eosio/rem_oracle_plugin/http_client.hpp>
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

#include <boost/optional/optional.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ssl.hpp>

using boost::asio::ip::tcp;

namespace eosio {
   static appbase::abstract_plugin& _rem_oracle_plugin = app().register_plugin<rem_oracle_plugin>();

   using namespace eosio::chain;

class rem_oracle_plugin_impl {
   public:
     std::vector<fc::crypto::private_key>    _oracle_signing_key;
     std::vector<name>                       _oracle_signing_account;
     std::vector<std::string>                _oracle_signing_permission;

     std::string _cryptocompare_apikey;

     void start_monitor() {
       while(true) {
         try {
           ilog("price monitor started");
           std::map<std::string, double> coingecko_prices;
           std::map<std::string, double> cryptocompare_prices;
           std::map<name, double> average_prices;
           std::string currencies[] = {"USD", "BTC", "ETH"};

           for(size_t i = 0; i < 3; i++) {
             coingecko_prices[ currencies[i] ] = get_coingecko_rem_price(coingecko_host, coingecko_endpoint, (currencies[i] == "USD" ? "USDT" : currencies[i].c_str()) );
             ilog("avg ${c} coingecko: ${p}", ("c", currencies[i])("p", coingecko_prices[ currencies[i] ]));

             cryptocompare_prices[ currencies[i] ] = 0;

             if(_cryptocompare_apikey != "0") {
               cryptocompare_prices[ currencies[i] ] = get_cryptocompare_rem_price(cryptocompare_host,
                                                                       (string(cryptocompare_endpoint)+string(cryptocompare_params)+_cryptocompare_apikey).c_str(),
                                                                        currencies[i].c_str());
               ilog("avg ${c} cryptocompare: ${p}", ("c", currencies[i])("p", cryptocompare_prices[ currencies[i] ]));
             } else {
                wlog("cryptocompare-apikey is not set");
             }
             int count = (coingecko_prices[ currencies[i] ] == 0 ? 0 : 1) + (cryptocompare_prices[ currencies[i] ] == 0 ? 0 : 1);
             if( count == 0 ) {
               elog("Can't retrieve REM token price data neither from https://www.cryptocompare.com/ not from https://www.coingecko.com/en");
               continue;
             }
             double price_sum = (coingecko_prices[ currencies[i] ] == 0 ? 0 : coingecko_prices[ currencies[i] ]) +
                                (cryptocompare_prices[ currencies[i] ] == 0 ? 0 : cryptocompare_prices[ currencies[i] ]);
             average_prices[ eosio::chain::string_to_name(boost::algorithm::to_lower_copy("REM." + currencies[i]).c_str()) ] = price_sum / count;
           }

           this->push_set_price_transaction(average_prices);

           sleep(update_price_period);
         } FC_LOG_WAIT_AND_CONTINUE()
       }
     }

     std::string make_request(const char* host, const char* endpoint) {
         boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
         ctx.set_default_verify_paths();

         boost::asio::io_service io_service;
         http_client c(io_service, ctx, host, endpoint);
         io_service.run();

         std::string response = c.get_response_body();
         return response;
     }

     double get_coingecko_rem_price(const char* coingecko_host, const char* remme_endpoint, const char* to_currency) {
         try {
           std::string response = make_request(coingecko_host, remme_endpoint);

           boost::iostreams::stream<boost::iostreams::array_source> stream(response.c_str(), response.size());
           namespace pt = boost::property_tree;
           pt::ptree root;
           pt::read_json(stream, root);
           double sum = 0;
           int counter = 0;
           for (pt::ptree::value_type &log : root.get_child("tickers")) {
               //const std::string& key = log.first; // key
               const boost::property_tree::ptree& subtree = log.second;

               boost::optional<std::string> target_opt = subtree.get_optional<std::string>("target");
               boost::optional<std::string> last_opt = subtree.get_optional<std::string>("last");
               //boost::optional<std::string> txid_opt = subtree.get_optional<std::string>("transactionHash");
               //cout << "t2: " << data_opt.get() << endl;

               if(target_opt && target_opt.get() == to_currency) {
                   sum += boost::lexical_cast<double>(last_opt.get());
                   counter++;
               }
               else
                   continue;
           }
           return sum/counter;
         } catch (...) {
            return 0;
         }
     }

     double get_cryptocompare_rem_price(const char* cryptocompare_host, const char* cryptocompare_endpoint, const char* to_currency) {
         try {
           std::string response = make_request(cryptocompare_host, cryptocompare_endpoint);

           boost::iostreams::stream<boost::iostreams::array_source> stream(response.c_str(), response.size());
           namespace pt = boost::property_tree;
           pt::ptree root;
           pt::read_json(stream, root);
           boost::optional<double> price_opt = root.get_optional<double>(to_currency);
           if (price_opt)
               return price_opt.get();
           else
               return 0;
         } catch (...) {
            return 0;
         }

     }

     void push_set_price_transaction(const std::map<name, double>& pairs_data) {
         for(size_t i = 0; i < this->_oracle_signing_key.size(); i++) {
           std::vector<signed_transaction> trxs;
           trxs.reserve(2);

           controller& cc = app().get_plugin<chain_plugin>().chain();
           auto chainid = app().get_plugin<chain_plugin>().get_chain_id();

           signed_transaction trx;
           trx.actions.emplace_back(vector<chain::permission_level>{{this->_oracle_signing_account[i],name(this->_oracle_signing_permission[i])}},
             setprice{this->_oracle_signing_account[i],
               pairs_data});

           trx.expiration = cc.head_block_time() + fc::seconds(30);
           trx.set_reference_block(cc.head_block_id());
           trx.max_net_usage_words = 5000;
           trx.sign(this->_oracle_signing_key[i], chainid);
           trxs.emplace_back(std::move(trx));
           try {
              auto trxs_copy = std::make_shared<std::decay_t<decltype(trxs)>>(std::move(trxs));
              app().post(priority::low, [trxs_copy]() {
                for (size_t i = 0; i < trxs_copy->size(); ++i) {
                    app().get_plugin<chain_plugin>().accept_transaction( std::make_shared<packed_transaction>(trxs_copy->at(i)),
                    [](const fc::static_variant<fc::exception_ptr, transaction_trace_ptr>& result){
                      if (result.contains<fc::exception_ptr>()) {
                         elog("Failed to push set price transaction: ${res}", ( "res", result.get<fc::exception_ptr>()->to_string() ));
                      } else {
                         if (result.contains<transaction_trace_ptr>() && result.get<transaction_trace_ptr>()->receipt) {
                             auto trx_id = result.get<transaction_trace_ptr>()->id;
                             ilog("Pushed set price transaction: ${id}", ( "id", trx_id ));
                         }
                      }
                   });
                }
              });
           } FC_LOG_AND_DROP()
         }
     }

};

rem_oracle_plugin::rem_oracle_plugin():my(new rem_oracle_plugin_impl()){}
rem_oracle_plugin::~rem_oracle_plugin(){}

void rem_oracle_plugin::set_program_options(options_description&, options_description& cfg) {
  cfg.add_options()
        ("cryptocompare-apikey", bpo::value<std::string>()->default_value(std::string("0")),  // doesn't accept empty strings
         "cryptocompare api key for reading REM token price")
        ("oracle-authority", bpo::value<std::vector<std::string>>(),
         "Account name and permission to authorize set rem token price actions. For example blockproducer1@active")
        ("oracle-signing-key", bpo::value<std::vector<std::string>>(),
         "A private key to sign set price actions")

        ("update_price_period", bpo::value<uint32_t>()->default_value(update_price_period), "")
        ;
}

void rem_oracle_plugin::plugin_initialize(const variables_map& options) {
  try {
    std::vector<std::string> oracle_auth = options.at( "oracle-authority" ).as<std::vector<std::string>>();
    std::vector<std::string> oracle_signing_key = options.at( "oracle-signing-key" ).as<std::vector<std::string>>();
    for(size_t i = 0; i < oracle_auth.size(); i++) {
      auto space_pos = oracle_auth[i].find('@');
      EOS_ASSERT( (space_pos != std::string::npos), chain::plugin_config_exception,
                  "invalid authority" );
      std::string permission = oracle_auth[i].substr( space_pos + 1 );
      std::string account = oracle_auth[i].substr( 0, space_pos );

      struct name oracle_signing_account(account);
      my->_oracle_signing_account.push_back(oracle_signing_account);
      my->_oracle_signing_permission.push_back(permission);
      my->_oracle_signing_key.push_back(fc::crypto::private_key( oracle_signing_key[std::min(i, oracle_signing_key.size()-1)] ));
    }

    std::string cryptocompare_apikey = options.at( "cryptocompare-apikey" ).as<std::string>();
    my->_cryptocompare_apikey = cryptocompare_apikey;

    update_price_period = options.at( "update_price_period" ).as<uint32_t>();

  } FC_LOG_AND_RETHROW()

}

void rem_oracle_plugin::plugin_startup() {
    std::thread t2([=](){
        try {
          my->start_monitor();
        } FC_LOG_AND_RETHROW()
    });
    t2.detach();
}

void rem_oracle_plugin::plugin_shutdown() {

}

}
