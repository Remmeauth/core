/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <appbase/application.hpp>

namespace eosio {

using namespace appbase;

const char* coingecko_host = "api.coingecko.com";
const char* coingecko_endpoint = "/api/v3/coins/remme/tickers";

const char* cryptocompare_host = "min-api.cryptocompare.com";
const char* cryptocompare_endpoint = "/data/price";
const char* cryptocompare_params = "?fsym=REM&tsyms=USD,BTC,ETH&apikey=";

uint32_t update_price_period = 3600;  // seconds

#define FC_LOG_WAIT_AND_CONTINUE(...)  \
    catch( const boost::interprocess::bad_alloc& ) {\
       throw;\
    } catch( fc::exception& er ) { \
       wlog( "${details}", ("details",er.to_detail_string()) ); \
       sleep(update_price_period); \
       continue; \
    } catch( const std::exception& e ) {  \
       fc::exception fce( \
                 FC_LOG_MESSAGE( warn, "rethrow ${what}: ",FC_FORMAT_ARG_PARAMS( __VA_ARGS__  )("what",e.what()) ), \
                 fc::std_exception_code,\
                 BOOST_CORE_TYPEID(e).name(), \
                 e.what() ) ; \
       wlog( "${details}", ("details",fce.to_detail_string()) ); \
       sleep(update_price_period); \
       continue; \
    } catch( ... ) {  \
       fc::unhandled_exception e( \
                 FC_LOG_MESSAGE( warn, "rethrow", FC_FORMAT_ARG_PARAMS( __VA_ARGS__) ), \
                 std::current_exception() ); \
       wlog( "${details}", ("details",e.to_detail_string()) ); \
       sleep(update_price_period); \
       continue; \
    }

/**
 *  This is a template plugin, intended to serve as a starting point for making new plugins
 */
class rem_oracle_plugin : public appbase::plugin<rem_oracle_plugin> {
public:
   rem_oracle_plugin();
   virtual ~rem_oracle_plugin();

   APPBASE_PLUGIN_REQUIRES()
   virtual void set_program_options(options_description&, options_description& cfg) override;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr<class rem_oracle_plugin_impl> my;
};

}
