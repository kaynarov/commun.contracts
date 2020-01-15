#pragma once
#include "test_api_helper.hpp"
#include <commun.recover/include/commun.recover/config.hpp>
#include <commun/config.hpp>
#include <eosio/chain/types.hpp>
#include <fc/io/raw.hpp>

namespace cfg = commun::config;

namespace eosio { namespace testing {

struct commun_recover_api: base_contract_api {
    commun_recover_api(golos_tester* tester, name code)
        :   base_contract_api(tester, code) {}

    action_result setparams(name signer, mvo params) {
        return push(N(setparams), signer, params);
    }

    action_result recover(name account, std::optional<public_key_type> active_key, std::optional<public_key_type> owner_key) {
        auto a = args()("account", account);
        if (active_key.has_value() && active_key.value() != public_key_type()) a["active_key"] = active_key.value();
        if (owner_key.has_value() && owner_key.value() != public_key_type()) a["owner_key"] = owner_key.value();
        return push(N(recover), _code, a);
    }

    action_result applyowner(name account) {
        return push(N(applyowner), account, args()
            ("account", account)
        );
    }

    action_result cancelowner(name account) {
        return push(N(cancelowner), account, args()
            ("account", account)
        );
    };

};

}} // eosio::testing

