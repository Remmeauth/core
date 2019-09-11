#include <rem.system/rem.system.hpp>


namespace eosiosystem {

   std::vector<eosio::producer_key> system_contract::get_rotated_schedule() {
      const auto ct = eosio::current_time_point();
      const auto next_rotation_time = _grotation.last_rotation_time + _grotation.rotation_period;

      //get top active and standby producers
      auto sortedprods = _producers.get_index<"prototalvote"_n>();
      std::vector<eosio::producer_key> top_prods;
      std::vector<eosio::producer_key> rotating_standby;
      top_prods.reserve(max_block_producers);
      auto prod_it = sortedprods.begin();
      for (; top_prods.size() < max_block_producers &&
               prod_it != sortedprods.end() && 0 < prod_it->total_votes && prod_it->active(); prod_it++) {
         top_prods.push_back(eosio::producer_key{ prod_it->owner, prod_it->producer_key });
      }
      //return top_prods;
      if (top_prods.size() == max_block_producers) {
         rotating_standby.reserve(_grotation.standby_prods_to_rotate);
         for (; rotating_standby.size() < _grotation.standby_prods_to_rotate &&
                  prod_it != sortedprods.end() && 0 < prod_it->total_votes && prod_it->active(); prod_it++) {
            rotating_standby.push_back(eosio::producer_key{ prod_it->owner, prod_it->producer_key });
         }
      }


      if (next_rotation_time <= ct) {
         //update rotation pair
         _grotation.bp_out = name(0);
         _grotation.sbp_in = name(0);
         if (!_grotation.prods_by_rotation_time.empty() && !_grotation.standby_prods_by_rotation_time.empty()) {
            //check(false, "addsdaasada");
            _grotation.bp_out = _grotation.prods_by_rotation_time.front();
            _grotation.sbp_in = _grotation.standby_prods_by_rotation_time.front();
            // move bp_out and sbp_in to the end of respective lists
            _grotation.prods_by_rotation_time.pop_front();
            _grotation.prods_by_rotation_time.push_back(_grotation.bp_out);
            _grotation.standby_prods_by_rotation_time.pop_front();
            _grotation.standby_prods_by_rotation_time.push_back(_grotation.sbp_in);
            _grotation.last_rotation_time = ct;
         }
      }
      else {
         //reset rotation pair if bp_out is not in active producers or if sbp_in is not in top standby producers
         if (std::find_if(std::begin(top_prods), std::end(top_prods),
            [this](const auto& prod_key) { return prod_key.producer_name == _grotation.bp_out; }) == std::end(top_prods) ||
               std::find_if(std::begin(rotating_standby), std::end(rotating_standby),
                  [this](const auto& prod_key) { return prod_key.producer_name == _grotation.sbp_in; }) == std::end(rotating_standby)) {
            _grotation.bp_out = name(0);
            _grotation.sbp_in = name(0);
         }
      }

      auto schedule = std::move(top_prods);
      if (_grotation.bp_out != name(0) && _grotation.sbp_in != name(0)) {
         const auto bp_it = std::find_if(std::begin(schedule), std::end(schedule),
            [this](const auto& prod_key) { return prod_key.producer_name == _grotation.bp_out; });
         if (bp_it != std::end(schedule)) {
            const auto& prod = _producers.get(_grotation.sbp_in.value);
            *bp_it = eosio::producer_key{ prod.owner, prod.producer_key };
         }
      }
      return schedule;
   }
} /// namespace eosiosystem