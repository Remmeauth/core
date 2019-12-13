#include <eosio/datastream.hpp>
#include <eosio/eosio.hpp>
#include <eosio/multi_index.hpp>
#include <eosio/privileged.hpp>
#include <eosio/serialize.hpp>
#include <eosio/transaction.hpp>

#include <rem.system/rem.system.hpp>
#include <rem.token/rem.token.hpp>

#include <cmath>
#include <map>
#include <algorithm>

#include "name_bidding.cpp"
// Unfortunately, this is needed until CDT fixes the duplicate symbol error with eosio::send_deferred

namespace eosiosystem {

   using eosio::asset;
   using eosio::const_mem_fun;
   using eosio::current_time_point;
   using eosio::indexed_by;
   using eosio::permission_level;
   using eosio::seconds;
   using eosio::time_point_sec;
   using eosio::token;

   using std::map;
   using std::pair;
   using namespace std::string_literals;


   struct [[eosio::table, eosio::contract("rem.system")]] refund_request {
      name            owner;
      time_point_sec  request_time;
      time_point      last_claim_time;
      time_point      unlock_time;
      eosio::asset    resource_amount;

      bool is_empty()const { return resource_amount.amount == 0; }
      uint64_t  primary_key()const { return owner.value; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( refund_request, (owner)(request_time)(last_claim_time)(unlock_time)(resource_amount) )
   };

   /**
    *  These tables are designed to be constructed in the scope of the relevant user, this
    *  facilitates simpler API for per-user queries
    */
   typedef eosio::multi_index< "refunds"_n, refund_request >      refunds_table;

   void validate_b1_vesting( int64_t stake ) {
      const int64_t base_time = 1527811200; /// 2018-06-01
      const int64_t max_claimable = 100'000'000'0000ll;
      const int64_t claimable = int64_t(max_claimable * double(current_time_point().sec_since_epoch() - base_time) / (10*seconds_per_year) );

      check( max_claimable - claimable <= stake, "b1 can only claim their tokens over 10 years" );
   }

   void system_contract::changebw( name from, const name& receiver,
                                   const asset& stake_delta, bool transfer )
   {
      require_auth( from );
      check( stake_delta.amount != 0, "should stake non-zero amount" );

      name source_stake_from = from;
      if ( transfer ) {
         from = receiver;
      }

      // update stake delegated from "from" to "receiver"
      {
         del_bandwidth_table     del_tbl( get_self(), from.value );
         auto itr = del_tbl.find( receiver.value );
         if( itr == del_tbl.end() ) {
            itr = del_tbl.emplace( from, [&]( auto& dbo ){
                  dbo.from          = from;
                  dbo.to            = receiver;
                  dbo.net_weight    = stake_delta;
                  dbo.cpu_weight    = stake_delta;
               });
         }
         else {
            del_tbl.modify( itr, same_payer, [&]( auto& dbo ){
                  dbo.net_weight    += stake_delta;
                  dbo.cpu_weight    += stake_delta;
               });
         }
         check( 0 <= itr->net_weight.amount, "insufficient staked net bandwidth" );
         check( 0 <= itr->cpu_weight.amount, "insufficient staked cpu bandwidth" );
         if ( itr->is_empty() ) {
            del_tbl.erase( itr );
         }
      } // itr can be invalid, should go out of scope

      // update totals of "receiver"
      {
         user_resources_table   totals_tbl( get_self(), receiver.value );
         auto tot_itr = totals_tbl.find( receiver.value );
         if( tot_itr ==  totals_tbl.end() ) {
            tot_itr = totals_tbl.emplace( from, [&]( auto& tot ) {
                  tot.owner = receiver;
                  tot.net_weight    = stake_delta;
                  tot.cpu_weight    = stake_delta;
                  if (from == receiver) {
                     tot.own_stake_amount = stake_delta.amount;
                  }
               });
         } else {
            totals_tbl.modify( tot_itr, from == receiver ? from : same_payer, [&]( auto& tot ) {
                  tot.net_weight    += stake_delta;
                  tot.cpu_weight    += stake_delta;
                  if (from == receiver) {
                     tot.own_stake_amount += stake_delta.amount;

                     // we have to decrease free bytes in case of own stake
                     const int64_t new_free_stake_amount = std::min( static_cast< int64_t >(_gstate.min_account_stake) - tot.own_stake_amount, tot.free_stake_amount);
                     tot.free_stake_amount = std::max(new_free_stake_amount, 0LL);
                  }
               });
         }
         check( 0 <= tot_itr->net_weight.amount, "insufficient staked total net bandwidth" );
         check( 0 <= tot_itr->cpu_weight.amount, "insufficient staked total cpu bandwidth" );
         check( _gstate.min_account_stake <= tot_itr->own_stake_amount + tot_itr->free_stake_amount, "insufficient minimal account stake for " + receiver.to_string() );

         {
            bool ram_managed = false;
            bool net_managed = false;
            bool cpu_managed = false;

            auto voter_itr = _voters.find( receiver.value );
            if( voter_itr != _voters.end() ) {
               ram_managed = has_field( voter_itr->flags1, voter_info::flags1_fields::ram_managed );
               net_managed = has_field( voter_itr->flags1, voter_info::flags1_fields::net_managed );
               cpu_managed = has_field( voter_itr->flags1, voter_info::flags1_fields::cpu_managed );
            }
            if( !(net_managed && cpu_managed && ram_managed) ) {
               int64_t ram_bytes = 0;
               int64_t net       = 0;
               int64_t cpu       = 0;
               get_resource_limits( receiver, ram_bytes, net, cpu );

               const auto system_token_max_supply = eosio::token::get_max_supply(token_account, system_contract::get_core_symbol().code() );
               const double bytes_per_token = (double)_gstate.max_ram_size / (double)system_token_max_supply.amount;
               int64_t bytes_for_stake = bytes_per_token * (tot_itr->own_stake_amount + tot_itr->free_stake_amount);
               set_resource_limits( receiver,
                                    ram_managed ? ram_bytes : bytes_for_stake,
                                    net_managed ? net : tot_itr->net_weight.amount + tot_itr->free_stake_amount,
                                    cpu_managed ? cpu : tot_itr->cpu_weight.amount + tot_itr->free_stake_amount);
            }
         }
      } // tot_itr can be invalid, should go out of scope

      vote_stake_updater( from );
      update_voting_power( from, stake_delta );
   }

   void system_contract::update_voting_power( const name& voter, const asset& total_update )
   {
      auto voter_itr = _voters.find( voter.value );
      if( voter_itr == _voters.end() ) {
         voter_itr = _voters.emplace( voter, [&]( auto& v ) {
            v.owner  = voter;
            v.staked = total_update.amount;
            v.locked_stake = total_update.amount;
         });
      } else {
         _voters.modify( voter_itr, same_payer, [&]( auto& v ) {
            v.staked += total_update.amount;
            v.locked_stake += total_update.amount;
         });
      }

      check( 0 <= voter_itr->staked, "stake for voting cannot be negative" );

      if( voter == "b1"_n ) {
         validate_b1_vesting( voter_itr->staked );
      }

      if( voter_itr->producers.size() || voter_itr->proxy ) {
         update_votes( voter, voter_itr->proxy, voter_itr->producers, false );
      }
   }

   void system_contract::delegatebw( const name& from, const name& receiver,
                                     const asset& stake_quantity,
                                     bool transfer )
   {
      asset zero_asset( 0, core_symbol() );
      check( stake_quantity >= zero_asset, "must stake a positive amount" );
      check( !transfer || from != receiver, "cannot use transfer flag if delegating to self" );
      changebw( from, receiver, stake_quantity, transfer);

      const auto ct = current_time_point();
      // apply stake-lock to those who received stake
      const auto& voter = _voters.get( transfer ? receiver.value : from.value, "user has no resources");
      _voters.modify( voter, same_payer, [&]( auto& v ) {
         const auto restake_rate = double(stake_quantity.amount) / v.staked;
         const auto prevstake_rate = 1.0 - restake_rate;
         const auto time_to_stake_unlock = std::max( v.stake_lock_time - ct, microseconds{} );

         v.stake_lock_time = ct
               + microseconds{ static_cast< int64_t >( prevstake_rate * time_to_stake_unlock.count() ) }
               + microseconds{ static_cast< int64_t >( restake_rate * _gremstate.stake_lock_period.count() ) };
      });

      // transfer staked tokens to stake_account (eosio.stake)
      // for eosio.stake both transfer and refund make no sense
      if ( stake_account != from ) { 
         token::transfer_action transfer_act{ token_account, { {from, active_permission} } };
         transfer_act.send( from, stake_account, asset(stake_quantity), "stake bandwidth" );
      }
   } // delegatebw

   void system_contract::undelegatebw( const name& from, const name& receiver,
                                       const asset& unstake_quantity)
   {
      asset zero_asset( 0, core_symbol() );
      check( unstake_quantity >= zero_asset, "must unstake a positive amount" );
      check( _gstate.total_activated_stake >= min_activated_stake,
             "cannot undelegate bandwidth until the chain is activated (at least 15% of all tokens participate in voting)" );


      const auto ct = current_time_point();
      const auto& voter = _voters.get(from.value, "user has no resources");
      check(voter.stake_lock_time <= ct, "cannot undelegate during stake lock period");

      // for eosio.stake both transfer and refund make no sense
      if ( stake_account != from ) {
         refunds_table refunds_tbl( get_self(), from.value );
         auto req = refunds_tbl.find( from.value );

         // net and cpu are same sign by assertions in delegatebw and undelegatebw
         // redundant assertion also at start of changebw to protect against misuse of changebw
         if ( req != refunds_tbl.end() ) { //need to update refund
            refunds_tbl.modify( req, same_payer, [&]( refund_request& r ) {
               const auto ct = current_time_point();

               r.request_time     = ct;
               r.last_claim_time  = ct;
               r.resource_amount += unstake_quantity;

               const auto restake_rate = double(unstake_quantity.amount) / r.resource_amount.amount;
               const auto prevstake_rate = 1.0 - restake_rate;
               const auto time_to_stake_unlock = std::max( r.unlock_time - ct, microseconds{} );

               r.unlock_time = ct
                     + microseconds{ static_cast< int64_t >( prevstake_rate * time_to_stake_unlock.count() ) }
                     + microseconds{ static_cast< int64_t >( restake_rate * _gremstate.stake_lock_period.count() ) };
            });

            check( 0 <= req->resource_amount.amount, "negative net refund amount" ); //should never happen
         } else { //need to create refund
            refunds_tbl.emplace( from, [&]( refund_request& r ) {
               r.owner = from;
               r.resource_amount    = unstake_quantity;
               r.request_time       = current_time_point();
               r.last_claim_time    = current_time_point();
               r.unlock_time        = current_time_point() + _gremstate.stake_unlock_period;
            });
         } // else stake increase requested with no existing row in refunds_tbl -> nothing to do with refunds_tbl
      }

      changebw( from, receiver, -unstake_quantity , false);
   } // undelegatebw


   void system_contract::refund( const name& owner ) {
      require_auth( owner );

      refunds_table refunds_tbl( get_self(), owner.value );
      const auto& req = refunds_tbl.get( owner.value, "refund request not found" );
      
      const auto ct = current_time_point();
      check( ct - req.last_claim_time > eosio::days( 1 ), "already claimed refunds within past day" );

      const auto unlock_period_in_days = (req.unlock_time - req.last_claim_time).count() / useconds_per_day;
      const auto unclaimed_days = std::min( (ct - req.last_claim_time).count() / useconds_per_day, unlock_period_in_days ); 
      asset refund_amount = req.resource_amount * unclaimed_days / unlock_period_in_days;

      check( refund_amount > asset{ 0, core_symbol() }, "insufficient unlocked amount" );

      token::transfer_action transfer_act{ token_account, { {stake_account, active_permission}, {req.owner, active_permission} } };
      transfer_act.send( stake_account, req.owner, refund_amount, "unstake" );
      
      refunds_tbl.modify( req, same_payer, [&refund_amount, &ct]( auto& refund_request ){
         refund_request.last_claim_time = ct;
         refund_request.resource_amount -= refund_amount;
      });

      if ( req.is_empty() ) {
         refunds_tbl.erase( req );
      }
   }


   void system_contract::refundtostake( const name& owner ) {
      require_auth( owner );

      refunds_table refunds_tbl( get_self(), owner.value );
      const auto& req = refunds_tbl.get( owner.value, "refund request not found" );

      const auto ct = current_time_point();

      const auto unlock_period_in_days = (req.unlock_time - req.last_claim_time).count() / useconds_per_day;
      const auto days_to_unlock = std::max( unlock_period_in_days - (ct - req.last_claim_time).count() / useconds_per_day, int64_t{0} );
      asset refund_amount = req.resource_amount * days_to_unlock / unlock_period_in_days;

      changebw( owner, owner, refund_amount, false );

      refunds_tbl.modify( req, same_payer, [&refund_amount, &ct]( auto& refund_request ){
         refund_request.unlock_time = ct;
         refund_request.resource_amount -= refund_amount;
      });

      if ( req.is_empty() ) {
         refunds_tbl.erase( req );
      }
   }
} //namespace eosiosystem
