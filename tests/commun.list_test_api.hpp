#pragma once
#include "test_api_helper.hpp"
#include <commun.list/include/commun.list/config.hpp>

namespace eosio { namespace testing {

using commun::structures::opus_info;

struct commun_list_api: base_contract_api {
    commun_list_api(golos_tester* tester, name code)
        :   base_contract_api(tester, code) {}

    action_result create(name sender, symbol_code commun_code, std::string community_name) {
        return push(N(create), sender, args()
            ("commun_code", commun_code)
            ("community_name", community_name)
        );
    }

    mvo sysparams() {
        return args()
            ("opuses", std::set<opus_info>())
            ("remove_opuses", std::set<name>());
    }

    mvo info() {
        return args();
    }
    
    action_result setappparams(mvo params) {
        return push(N(setappparams), _code, params);
    }

    action_result setsysparams(symbol_code commun_code, mvo params) {
        return push(N(setsysparams), _code, params
            ("commun_code", commun_code)
        );
    }

    action_result setparams(name signer, symbol_code commun_code, mvo params) {
        return push(N(setparams), signer, params
            ("commun_code", commun_code)
        );
    }

    action_result setinfo(name signer, symbol_code commun_code, mvo params) {
        return push(N(setinfo), signer, params
            ("commun_code", commun_code)
        );
    }

    action_result follow(symbol_code commun_code, name follower) {
        return push(N(follow), follower, args()
            ("commun_code", commun_code)
            ("follower", follower)
        );
    }

    action_result unfollow(symbol_code commun_code, name follower) {
        return push(N(unfollow), follower, args()
            ("commun_code", commun_code)
            ("follower", follower)
        );
    }

    action_result hide(symbol_code commun_code, name follower) {
        return push(N(hide), follower, args()
            ("commun_code", commun_code)
            ("follower", follower)
        );
    }

    action_result unhide(symbol_code commun_code, name follower) {
        return push(N(unhide), follower, args()
            ("commun_code", commun_code)
            ("follower", follower)
        );
    }
};

}} // eosio::testing

