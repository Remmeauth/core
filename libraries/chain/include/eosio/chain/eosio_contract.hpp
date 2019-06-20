/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/contract_types.hpp>

namespace eosio { namespace chain {

   class apply_context;

   /**
    * @defgroup native_action_handlers Native Action Handlers
    */
   ///@{
   void apply_remme_newaccount(apply_context&);
   void apply_remme_updateauth(apply_context&);
   void apply_remme_deleteauth(apply_context&);
   void apply_remme_linkauth(apply_context&);
   void apply_remme_unlinkauth(apply_context&);

   /*
   void apply_remme_postrecovery(apply_context&);
   void apply_remme_passrecovery(apply_context&);
   void apply_remme_vetorecovery(apply_context&);
   */

   void apply_remme_setcode(apply_context&);
   void apply_remme_setabi(apply_context&);

   void apply_remme_canceldelay(apply_context&);
   ///@}  end action handlers

} } /// namespace eosio::chain
