#pragma once
#include "test_api_helper.hpp"
#include "../commun.social/include/commun.social/types.h"

namespace eosio { namespace testing {

struct commun_social_api: base_contract_api {
    commun_social_api(golos_tester* tester, name code)
        :   base_contract_api(tester, code) {}

    action_result pin(account_name pinner, account_name pinning) {
        return push(N(pin), pinner, args()
            ("pinner", pinner)
            ("pinning", pinning)
        );
    }

    action_result unpin(account_name pinner, account_name pinning) {
        return push(N(unpin), pinner, args()
            ("pinner", pinner)
            ("pinning", pinning)
        );
    }

    action_result block(account_name blocker, account_name blocking) {
        return push(N(block), blocker, args()
            ("blocker", blocker)
            ("blocking", blocking)
        );
    }

    action_result unblock(account_name blocker, account_name blocking) {
        return push(N(unblock), blocker, args()
            ("blocker", blocker)
            ("blocking", blocking)
        );
    }

    action_result updatemeta(account_name account, accountmeta meta) {
        return push(N(updatemeta), account, args()
            ("account", account)
            ("meta", meta)
        );
    }

    action_result deletemeta(account_name account) {
        return push(N(deletemeta), account, args()
            ("account", account)
        );
    }
};

}} // eosio::testing
