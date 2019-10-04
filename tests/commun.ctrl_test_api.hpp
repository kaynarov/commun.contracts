#pragma once
#include "test_api_helper.hpp"
#include "contracts.hpp"
#include "../include/commun/config.hpp"

namespace eosio { namespace testing {

namespace cfg = commun::config;

struct commun_ctrl_api: base_contract_api {
    commun_ctrl_api(golos_tester* tester, account_name code, symbol_code commun_code_, account_name owner_)
    :   base_contract_api(tester, code)
    ,   commun_code(commun_code_)
    ,   owner(owner_) {}
    
    symbol_code commun_code;
    account_name owner;

    using base_contract_api::base_contract_api;

    const name _minority_name = N(witn.minor);
    const name _majority_name = N(witn.major);
    const name _supermajority_name = N(witn.smajor);

    //// control actions
    
    action_result set_default_params() {
        return push(N(setparams), _code, args()("commun_code", commun_code));
    }

    action_result reg_witness(name witness, string url) {
        return push(N(regwitness), witness, args()
            ("commun_code", commun_code)
            ("witness", witness)
            ("url", url)
        );
    }

    action_result unreg_witness(name witness) {
        return push(N(unregwitness), witness, args()
            ("witness", witness)
        );
    }

    action_result start_witness(name witness) {
        return push(N(startwitness), witness, args()
            ("witness", witness)
        );
    }

    action_result stop_witness(name witness) {
        return push(N(stopwitness), witness, args()
            ("commun_code", commun_code)
            ("witness", witness)
        );
    }

    action_result vote_witness(name voter, name witness) {
        return push(N(votewitness), voter, args()
            ("commun_code", commun_code)
            ("voter", voter)
            ("witness", witness)
        );
    }
    action_result unvote_witness(name voter, name witness) {
        return push(N(unvotewitn), voter, args()
            ("commun_code", commun_code)
            ("voter", voter)
            ("witness", witness)
        );
    }

    action_result change_vests(name who, asset diff) {
        return push(N(changevest), who, args()
            ("commun_code", commun_code)
            ("who", who)
            ("diff", diff));
    }
    
    action_result propose(name proposer, name proposal_name, name permission, transaction trx) {
        return push(N(propose), proposer, args()
            ("commun_code", commun_code)
            ("proposer", proposer)
            ("proposal_name", proposal_name)
            ("permission", permission)
            ("trx", trx));
    }
    
    action_result approve(name proposer, name proposal_name, name approver) { // TODO: proposal_hash
        return push(N(approve), approver, args()
            ("proposer", proposer)
            ("proposal_name", proposal_name)
            ("approver", approver));
    }
    
    action_result unapprove(name proposer, name proposal_name, name approver) {
        return push(N(unapprove), approver, args()
            ("proposer", proposer)
            ("proposal_name", proposal_name)
            ("approver", approver));
    }
    
    action_result cancel(name proposer, name proposal_name, name canceler) {
        return push(N(cancel), canceler, args()
            ("proposer", proposer)
            ("proposal_name", proposal_name)
            ("canceler", canceler));
    }
    
    action_result exec(name proposer, name proposal_name, name executer) {
        return push(N(exec), executer, args()
            ("proposer", proposer)
            ("proposal_name", proposal_name)
            ("executer", executer));
    }
    
    action_result invalidate(name account) {
        return push(N(invalidate), account, args()
            ("account", account));
    }

    variant get_witness(name witness) const {
        return get_struct(_code, N(witness), witness, "witness_info");
    }

    std::vector<variant> get_all_witnesses() {
        return _tester->get_all_chaindb_rows(_code, commun_code.value, N(witness), false);
    }
    
    void prepare(const std::vector<name>& leaders, name voter) {
        
        BOOST_CHECK_EQUAL(base_tester::success(), set_default_params());
        for (int i = 0; i < leaders.size(); i++) {
            BOOST_CHECK_EQUAL(base_tester::success(), reg_witness(leaders[i], "localhost"));
            BOOST_CHECK_EQUAL(base_tester::success(), vote_witness(voter, leaders[i]));
        }
    }

};


}} // eosio::testing
