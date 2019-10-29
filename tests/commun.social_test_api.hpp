#pragma once
#include "test_api_helper.hpp"
#include "../commun.social/include/commun.social/types.h"
#include <commun/config.hpp>

namespace cfg = commun::config;

namespace eosio { namespace testing {

struct commun_social_api: base_contract_api {
    commun_social_api(golos_tester* tester, name code)
        :   base_contract_api(tester, code) {}

    action_result push_maybe_msig(account_name act, account_name actor, mvo a, account_name client) {
        return client ?
           push_msig(act, {{actor, config::active_name}, {_code, cfg::client_permission_name}},
               {actor, client}, a) :
           push(act, actor, a);
    }

    action_result pin(account_name pinner, account_name pinning, account_name client) {
        return push_maybe_msig(N(pin), pinner, args()
            ("pinner", pinner)
            ("pinning", pinning),
            client
        );
    }

    action_result unpin(account_name pinner, account_name pinning, account_name client) {
        return push_maybe_msig(N(unpin), pinner, args()
            ("pinner", pinner)
            ("pinning", pinning),
            client
        );
    }

    action_result block(account_name blocker, account_name blocking, account_name client) {
        return push_maybe_msig(N(block), blocker, args()
            ("blocker", blocker)
            ("blocking", blocking),
            client
        );
    }

    action_result unblock(account_name blocker, account_name blocking, account_name client) {
        return push_maybe_msig(N(unblock), blocker, args()
            ("blocker", blocker)
            ("blocking", blocking),
            client
        );
    }

    action_result updatemeta(account_name account, accountmeta meta, account_name client) {
        return push_maybe_msig(N(updatemeta), account, args()
            ("account", account)
            ("meta", meta),
            client
        );
    }

    action_result deletemeta(account_name account, account_name client) {
        return push_maybe_msig(N(deletemeta), account, args()
            ("account", account),
            client
        );
    }
};

}} // eosio::testing
