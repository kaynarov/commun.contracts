#pragma once
#include "test_api_helper.hpp"

namespace eosio { namespace testing {

struct registrar_api: base_contract_api {
    registrar_api(golos_tester* tester, name code)
    :   base_contract_api(tester, code) {}

    action_result checkwin() {
        return push(N(checkwin), _code, args());
    }
    
    action_result claimrefund(name bidder, symbol_code sym_code) {
        return push(N(claimrefund), bidder, args()
            ("bidder", bidder)
            ("sym_code", sym_code)
        );
    }
    
    action_result create(account_name owner, asset maximum_supply, int16_t cw, int16_t fee) {
        return push(N(create), owner, args()
            ("maximum_supply", maximum_supply)
            ("cw", cw)
            ("fee", fee)
        );
    }
};

}} // eosio::testing
