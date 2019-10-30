/**
 *  @copyright defined in eos/LICENSE.txt
 */

#include <rem.oracle/rem.oracle.hpp>
#include <rem.system/rem.system.hpp>

namespace eosio {

   oracle::oracle(name receiver, name code, datastream<const char*> ds)
   :contract(receiver, code, ds),
    remprice_tbl(_self, _self.value),
    pricedata_tbl(_self, _self.value),
    pairs_tbl(_self, _self.value)
    {
       pairstable_data = pairs_tbl.exists() ? pairs_tbl.get() : pairstable{};
    }

   void oracle::setprice(const name &producer, std::map<name, double> &pairs_data) {
      require_auth(producer);
      check(is_producer(producer), "block producer authorization required");

      vector<name> _producers = eosio::get_active_producers();
      bool is_active_producer = std::find(_producers.begin(), _producers.end(), producer) != _producers.end();
      check_pairs(pairs_data);

      auto data_it = pricedata_tbl.find(producer.value);
      time_point ct = current_time_point();

      if (data_it != pricedata_tbl.end()) {
         uint64_t ct_amount_hours = ct.sec_since_epoch() / 3600;
         uint64_t last_amount_hours = data_it->last_update.to_time_point().sec_since_epoch() / 3600;
         check(ct_amount_hours > last_amount_hours, "the frequency of price changes should not exceed 1 time during the current hour");

         pricedata_tbl.modify(*data_it, producer, [&](auto &p) {
            p.pairs_data = pairs_data;
            p.last_update = ct;
         });
      } else {
         pricedata_tbl.emplace(producer, [&](auto &p) {
            p.producer = producer;
            p.pairs_data = pairs_data;
            p.last_update = ct;
         });
      }

      if (is_active_producer) {
         std::map<name, vector<double>> pairs_points = get_relevant_prices();
         auto majority_amount = get_majority_amount();
         uint32_t amount_prod_points = pairs_points.begin()->second.size();

         if (amount_prod_points > majority_amount) {
            for (const auto &points: pairs_points) {
               double median = get_subset_median(points.second);

               auto price_it = remprice_tbl.find(points.first.value);
               if (price_it != remprice_tbl.end()) {
                  remprice_tbl.modify(*price_it, producer, [&](auto &p) {
                     p.price        = median;
                     p.price_points = points.second;
                     p.last_update  = ct;
                  });
               } else {
                  remprice_tbl.emplace(producer, [&](auto &p) {
                     p.pair         = points.first;
                     p.price        = median;
                     p.price_points = points.second;
                     p.last_update  = ct;
                  });
               }
            }
         }
      }
   }

   void oracle::addpair(const name &pair) {
      require_auth(_self);

      check(pairstable_data.pairs.find(pair) == pairstable_data.pairs.end(), "the pair is already supported");
      pairstable_data.pairs.insert(pair);
      pairs_tbl.set(pairstable_data, _self);
   }

   std::map<name, vector<double>> oracle::get_relevant_prices() const {
      vector <name> _producers = eosio::get_active_producers();
      std::map<name, vector<double>> prices_data;

      for (const auto &producer: _producers) {
         auto it = pricedata_tbl.find(producer.value);
         if (it != pricedata_tbl.end()) {

            for (const auto &pair: pairstable_data.pairs) {
               // if a new pair is added, but the producer doesn't add the rate for new pair, its data is skipped
               if (it->pairs_data.count(pair) == 0) {
                  break;
               }
               prices_data[pair].push_back(it->pairs_data.at(pair));
            }
         }
      }
      return prices_data;
   }

   double oracle::get_subset_median(vector<double> points) const {
      std::sort(points.begin(), points.end());
      uint8_t majority = get_majority_amount();
      // the last of majority producer in the array will have an index `majority -1`
      // the subset index of the majority producers in the set [..(majority)....], indicates which index the set of the
      // majority is shifted from begin in sorted array points, wheare point - producer price rate
      uint8_t subset = 0;
      double min_delta = points.at(majority - 1) - points.at(0);
      double crt_delta;

      for (uint8_t i = majority; i < points.size(); ++i) {
         crt_delta = points.at(i) - points.at(i - majority + 1);
         if (min_delta > crt_delta) {
            min_delta = crt_delta;
            subset = i - majority + 1;
         }
      }
      double median = get_median(vector<double>(points.begin() + subset, points.begin() + subset + majority));

      return median;
   }

   double oracle::get_median(const vector<double>& sorted_points) const {
      if (sorted_points.size() % 2 == 0) {
         return (sorted_points.at(sorted_points.size() / 2) + sorted_points.at(sorted_points.size() / 2 - 1)) / 2;
      }
      return sorted_points.at(sorted_points.size() / 2);
   }

   uint8_t oracle::get_majority_amount() const {
      uint8_t prods_size = eosio::get_active_producers().size();
      return (prods_size * 2 / 3) + 1;
   }

   bool oracle::is_producer( const name& user ) const {
      eosiosystem::producers_table _producers_table( system_account, system_account.value );
      return _producers_table.find( user.value ) != _producers_table.end();
   }

   void oracle::check_pairs(const std::map<name, double> &pairs) {
      check(pairs.size() == pairstable_data.pairs.size(), "incorrect pairs");
      for(const auto &pair: pairs) {
         auto it = pairstable_data.pairs.find(pair.first);
         check(it != pairstable_data.pairs.end(), "unsupported pairs");
      }
   }
} /// namespace eosio

EOSIO_DISPATCH( eosio::oracle, (setprice)(addpair) )
