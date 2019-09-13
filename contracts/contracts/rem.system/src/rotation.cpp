#include <rem.system/rem.system.hpp>


namespace eosiosystem {

   std::vector<eosio::producer_key> system_contract::get_rotated_schedule() {
      //update rotation structure
      auto sortedprods = _producers.get_index<"prototalvote"_n>();
      std::vector<eosio::producer_key> top_prods; //top21 producers
      std::vector<name> rotating_standby; //first n standby producers
      std::deque<name> prods_by_rotation_time; //queue of producers to rotate out (from first to last)
      std::deque<name> standby_prods_by_rotation_time; //queue of standby producers to rotate in (from first to last)
      top_prods.reserve(max_block_producers);
      auto prod_it = sortedprods.begin();
      for (; top_prods.size() < max_block_producers &&
             prod_it != sortedprods.end() && 0 < prod_it->total_votes && prod_it->active(); prod_it++) {
         //fill top21 list
         top_prods.push_back(eosio::producer_key{ prod_it->owner, prod_it->producer_key });
         //see if producer has just became top21
         // if so then add it to prods_by_rotation_time
         // this way fresh top21 producers will be rotated out after old ones
         if (std::find(std::begin(_grotation.prods_by_rotation_time), std::end(_grotation.prods_by_rotation_time), prod_it->owner) ==
             std::end(_grotation.prods_by_rotation_time)) {
            //insert new producers
            prods_by_rotation_time.push_back(prod_it->owner);
         }
      }
      if (top_prods.size() == max_block_producers) {
         for (; rotating_standby.size() < _grotation.standby_prods_to_rotate &&
                prod_it != sortedprods.end() && 0 < prod_it->total_votes && prod_it->active(); prod_it++) {
            //fill standby producers list
            rotating_standby.push_back(prod_it->owner);
            //see if producer has just became top n standby producer (by default top22-25)
            // if so then add it to standby_prods_by_rotation_time
            // this way fresh stabdby producers will be rotated in after old ones
            if (std::find(std::begin(_grotation.standby_prods_by_rotation_time), std::end(_grotation.standby_prods_by_rotation_time), prod_it->owner) ==
                std::end(_grotation.standby_prods_by_rotation_time)) {
               //insert new standby producers
               standby_prods_by_rotation_time.push_back(prod_it->owner);
            }
         }
      }
      //add persisted top21 producers at the beginning of prods_by_rotation_time
      // so they will rotate out before fresh ones
      for (auto it = std::rbegin(_grotation.prods_by_rotation_time); it != std::rend(_grotation.prods_by_rotation_time); it++) {
         if (std::find_if(std::begin(top_prods), std::end(top_prods),
                          [&](const auto& prod_key) { return prod_key.producer_name == *it; }) != std::end(top_prods)) {
            prods_by_rotation_time.push_front(*it);
         }
      }
      //add persisted top n standby producer (by default top22-25) at the beginning of standby_prods_by_rotation_time
      // so they will rotate in before fresh ones
      for (auto it = std::rbegin(_grotation.standby_prods_by_rotation_time); it != std::rend(_grotation.standby_prods_by_rotation_time); it++) {
         if (std::find(std::begin(rotating_standby), std::end(rotating_standby), *it) != std::end(rotating_standby)) {
            standby_prods_by_rotation_time.push_front(*it);
         }
      }
      _grotation.prods_by_rotation_time.swap(prods_by_rotation_time);
      _grotation.standby_prods_by_rotation_time.swap(standby_prods_by_rotation_time);


      const auto ct = eosio::current_time_point();
      const auto next_rotation_time = _grotation.last_rotation_time + _grotation.rotation_period;
      if (next_rotation_time <= ct) {
         //if enough time has passed
         // update rotation pair
         _grotation.bp_out = name(0);
         _grotation.sbp_in = name(0);
         if (!_grotation.prods_by_rotation_time.empty() && !_grotation.standby_prods_by_rotation_time.empty()) {
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
      else if (_grotation.bp_out != name(0) && _grotation.sbp_in != name(0)) { //check if we need to drop rotation pair
         //reset rotation pair if bp_out is not in active producers or if sbp_in is not in top standby producers
         if (std::find_if(std::begin(top_prods), std::end(top_prods),
            [this](const auto& prod_key) { return prod_key.producer_name == _grotation.bp_out; }) == std::end(top_prods) ||
               std::find_if(std::begin(rotating_standby), std::end(rotating_standby),
                  [this](const auto& prod) { return prod == _grotation.sbp_in; }) == std::end(rotating_standby)) {
            _grotation.bp_out = name(0);
            _grotation.sbp_in = name(0);
         }
      }

      //get top21 list and replace rotated out producer with standby if rotation pair is set
      if (_grotation.bp_out != name(0) && _grotation.sbp_in != name(0)) {
         const auto bp_it = std::find_if(std::begin(top_prods), std::end(top_prods),
            [this](const auto& prod_key) { return prod_key.producer_name == _grotation.bp_out; });
         if (bp_it != std::end(top_prods)) {
            const auto& prod = _producers.get(_grotation.sbp_in.value);
            *bp_it = eosio::producer_key{ prod.owner, prod.producer_key };
         }
      }
      return top_prods;
   }
} /// namespace eosiosystem