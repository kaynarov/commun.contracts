#pragma once
#include "test_api_helper.hpp"
#include "contracts.hpp"
#include "../include/commun/config.hpp"

namespace eosio { namespace testing {

namespace cfg = commun::config;

struct commun_ctrl_api: base_contract_api {
    commun_ctrl_api(golos_tester* tester, account_name code, symbol_code point_, account_name owner_)
    :   base_contract_api(tester, code)
    ,   point(point_)
    ,   owner(owner_) {}
    
    symbol_code point;
    account_name owner;

    using base_contract_api::base_contract_api;

    const name _minority_name = N(witn.minor);
    const name _majority_name = N(witn.major);
    const name _supermajority_name = N(witn.smajor);

    //// control actions
    action_result set_params(const std::string& json_params) {
        return push(N(setparams), owner, args()
            ("point", point)
            ("params", json_str_to_obj(json_params))
        );
    }
    action_result set_param(const std::string& json_param) {
        return push(N(setparams), owner, args()
            ("point", point)
            ("params", json_str_to_obj(std::string() + "[" + json_param + "]"))
        );
    }
    action_result validate_params(const std::string& json_params) {
        return push(N(validateprms), owner, args()
            ("point", point)
            ("params", json_str_to_obj(json_params))
        );
    }

    action_result reg_witness(name witness, string url) {
        return push(N(regwitness), witness, args()
            ("point", point)
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
            ("point", point)
            ("witness", witness)
        );
    }

    action_result vote_witness(name voter, name witness) {
        return push(N(votewitness), voter, args()
            ("point", point)
            ("voter", voter)
            ("witness", witness)
        );
    }
    action_result unvote_witness(name voter, name witness) {
        return push(N(unvotewitn), voter, args()
            ("point", point)
            ("voter", voter)
            ("witness", witness)
        );
    }

    action_result change_vests(name who, asset diff) {
        return push(N(changevest), who, args()
            ("point", point)
            ("who", who)
            ("diff", diff));
    }

    variant get_params() const {
        return get_struct(_code, N(ctrlparams), N(ctrlparams), "ctrl_state");  // TODO: get_singleton instead of get_struct
    }

    variant get_witness(name witness) const {
        return get_struct(_code, N(witness), witness, "witness_info");
    }

    std::vector<variant> get_all_witnesses() {
        return _tester->get_all_chaindb_rows(_code, point.value, N(witness), false);
    }

    variant get_msig_auths() const {
        return get_struct(_code, N(msigauths), N(msigauths), "msig_auths");
    }

    //// helpers
    static std::string multisig_param(name acc) {
        return std::string() + "['multisig_acc',{'name':'"+acc.to_string()+"'}]";
    }
    static std::string max_witnesses_param(uint16_t max = 21) {
        return std::string() + "['max_witnesses',{'max':"+std::to_string(max)+"}]";
    }
    static std::string max_witness_votes_param(uint16_t max = 30) {
        return std::string() + "['max_witness_votes',{'max':"+std::to_string(max)+"}]";
    }
    static std::string msig_perms_param(uint16_t smajor = 0, uint16_t major = 0, uint16_t minor = 0) {
        return std::string() + "['multisig_perms',{"
            "'super_majority':"+std::to_string(smajor) +
            ",'majority':"+std::to_string(major) +
            ",'minority':"+std::to_string(minor) + "}]";
    }
    static std::string update_auth_param(uint32_t delay_sec = 30*60) {
        return std::string() + "['update_auth',{'period':"+std::to_string(delay_sec)+"}]";
    }
    static std::string default_params(name owner, uint16_t witnesses = 21, uint16_t witness_votes = 30,
            uint16_t smajor = 0, uint16_t major = 0, uint16_t minor = 0) {
        return std::string() + "[" +
            multisig_param(owner) + "," +
            max_witnesses_param(witnesses) + "," +
            msig_perms_param(smajor, major, minor) + "," +
            max_witness_votes_param(witness_votes) + "," +"]";
    }

    // sets permissions for "multisig" account
    void prepare_multisig(name msig) {
        // witn.major/minor
        auto auth = authority(1, {}, {
            {.permission = {msig, config::active_name}, .weight = 1}
        });
        _tester->set_authority(msig, _majority_name, auth, "active");
        _tester->set_authority(msig, _minority_name, auth, "active");

        // eosio.code
        auto code_auth = authority(1, {
            {_tester->get_public_key(msig, "active"), 1}
        }, {
            {.permission = {_code, config::eosio_code_name}, .weight = 1}
        });
        _tester->set_authority(msig, config::active_name, code_auth, "owner");
    }

};


}} // eosio::testing
