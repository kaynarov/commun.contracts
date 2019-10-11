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

    const name _minority_name = N(lead.minor);
    const name _majority_name = N(lead.major);
    const name _supermajority_name = N(lead.smajor);

    //// control actions

    action_result reg_leader(name leader, string url) {
        return push(N(regleader), leader, args()
            ("commun_code", commun_code)
            ("leader", leader)
            ("url", url)
        );
    }

    action_result unreg_leader(name leader) {
        return push(N(unregleader), leader, args()
            ("leader", leader)
        );
    }

    action_result start_leader(name leader) {
        return push(N(startleader), leader, args()
            ("leader", leader)
        );
    }

    action_result stop_leader(name leader) {
        return push(N(stopleader), leader, args()
            ("commun_code", commun_code)
            ("leader", leader)
        );
    }

    action_result vote_leader(name voter, name leader) {
        return push(N(voteleader), voter, args()
            ("commun_code", commun_code)
            ("voter", voter)
            ("leader", leader)
        );
    }
    action_result unvote_leader(name voter, name leader) {
        return push(N(unvotelead), voter, args()
            ("commun_code", commun_code)
            ("voter", voter)
            ("leader", leader)
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

    variant get_leader(name leader) const {
        return get_struct(_code, N(leader), leader, "leader_info");
    }

    std::vector<variant> get_all_leaders() {
        return _tester->get_all_chaindb_rows(_code, commun_code.value, N(leader), false);
    }
    
    void prepare(const std::vector<name>& leaders, name voter) {
        for (int i = 0; i < leaders.size(); i++) {
            BOOST_CHECK_EQUAL(base_tester::success(), reg_leader(leaders[i], "localhost"));
            BOOST_CHECK_EQUAL(base_tester::success(), vote_leader(voter, leaders[i]));
        }
    }

};


}} // eosio::testing
