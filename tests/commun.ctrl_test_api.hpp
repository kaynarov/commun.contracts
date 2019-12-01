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
    
    action_result clear_votes(name leader, std::optional<uint16_t> count = std::optional<uint16_t>()) {
        auto a = args()
            ("commun_code", commun_code)
            ("leader", leader);
        if (count.has_value()) {
            a("count", *count);
        }
        return push(N(clearvotes), leader, a);
    }

    action_result unreg_leader(name leader) {
        return push(N(unregleader), leader, args()
            ("commun_code", commun_code)
            ("leader", leader)
        );
    }

    action_result start_leader(name leader) {
        return push(N(startleader), leader, args()
            ("commun_code", commun_code)
            ("leader", leader)
        );
    }

    action_result stop_leader(name leader) {
        return push(N(stopleader), leader, args()
            ("commun_code", commun_code)
            ("leader", leader)
        );
    }

    action_result vote_leader(name voter, name leader, std::optional<uint16_t> pct = std::optional<uint16_t>()) {
        auto a = args()
            ("commun_code", commun_code)
            ("voter", voter)
            ("leader", leader);
        if (pct.has_value()) {
            a("pct", *pct);
        }
        return push(N(voteleader), voter, a);
    }
    action_result unvote_leader(name voter, name leader) {
        return push(N(unvotelead), voter, args()
            ("commun_code", commun_code)
            ("voter", voter)
            ("leader", leader)
        );
    }

    action_result change_points(name who, asset diff) {
        return push(N(changepoints), who, args()
            ("who", who)
            ("diff", diff)
        );
    }

    action_result emit(name actor, permission_level permission) {
        return push_msig(N(emit), {permission}, {actor}, args()
            ("commun_code", commun_code)
        );
    }
    
    action_result claim(name leader) {
        return push(N(claim), leader, args()
            ("commun_code", commun_code)
            ("leader", leader)
        );
    }
    
    action_result propose(name proposer, name proposal_name, name permission, transaction trx) {
        return push(N(propose), proposer, args()
            ("commun_code", commun_code)
            ("proposer", proposer)
            ("proposal_name", proposal_name)
            ("permission", permission)
            ("trx", trx)
        );
    }
    
    action_result approve(name proposer, name proposal_name, name approver, std::optional<checksum256_type> proposal_hash = std::optional<checksum256_type>()) {
        auto a = args()
            ("proposer", proposer)
            ("proposal_name", proposal_name)
            ("approver", approver);
        if (proposal_hash.has_value()) {
            a("proposal_hash", *proposal_hash);
        }
        return push(N(approve), approver, a);
    }
    
    action_result unapprove(name proposer, name proposal_name, name approver) {
        return push(N(unapprove), approver, args()
            ("proposer", proposer)
            ("proposal_name", proposal_name)
            ("approver", approver)
        );
    }
    
    action_result cancel(name proposer, name proposal_name, name canceler) {
        return push(N(cancel), canceler, args()
            ("proposer", proposer)
            ("proposal_name", proposal_name)
            ("canceler", canceler)
        );
    }
    
    action_result exec(name proposer, name proposal_name, name executer) {
        return push(N(exec), executer, args()
            ("proposer", proposer)
            ("proposal_name", proposal_name)
            ("executer", executer)
        );
    }
    
    action_result invalidate(name account) {
        return push(N(invalidate), account, args()
            ("account", account)
        );
    }

    variant get_leader(name leader) const {
        return get_struct(commun_code.value, N(leader), leader, "leader_info");
    }

    std::vector<variant> get_all_leaders() {
        return _tester->get_all_chaindb_rows(_code, commun_code.value, N(leader), false);
    }
    
    int64_t get_unclaimed(name leader) {
        return get_leader(leader)["unclaimed_points"].as<int64_t>();
    }
    
    int64_t get_retained() {
        return get_struct(commun_code.value, N(stat), commun_code.value, "stat")["retained"].as<int64_t>();
    }
    
    void prepare(const std::vector<name>& leaders, name voter, uint16_t pct_sum = cfg::_100percent) {
        for (int i = 0; i < leaders.size(); i++) {
            BOOST_CHECK_EQUAL(base_tester::success(), reg_leader(leaders[i], "localhost"));
            uint16_t pct = pct_sum / leaders.size();
            pct = (pct / (10 * cfg::_1percent)) * (10 * cfg::_1percent);
            BOOST_CHECK_EQUAL(base_tester::success(), vote_leader(voter, leaders[i], pct));
        }
    }

};


}} // eosio::testing
