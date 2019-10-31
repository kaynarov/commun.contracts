#pragma once
#include "test_api_helper.hpp"

namespace eosio { namespace testing {

struct commun_emit_api: base_contract_api {
    commun_emit_api(golos_tester* tester, name code)
        :   base_contract_api(tester, code) {}

    action_result init(symbol_code commun_code) {
        return push(N(init), _code, args()
            ("commun_code", commun_code)
        );
    }

    action_result issuereward(symbol_code commun_code, name to_contract) {
        return push(N(issuereward), _code, args()
            ("commun_code", commun_code)
            ("to_contract", to_contract)
        );
    }

    variant get_stat(symbol_code commun_code) {
        return get_struct(commun_code.value, N(stat), commun_code.value, "stat");
    }
};

}} // eosio::testing
