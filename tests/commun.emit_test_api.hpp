#pragma once
#include "test_api_helper.hpp"

namespace eosio { namespace testing {

struct commun_emit_api: base_contract_api {
    commun_emit_api(golos_tester* tester, name code)
        :   base_contract_api(tester, code) {}

    action_result create(symbol_code commun_code, uint16_t annual_emission_rate, uint16_t leaders_reward_prop) {
        return push(N(create), _code, args()
            ("commun_code", commun_code)
            ("annual_emission_rate", annual_emission_rate)
            ("leaders_reward_prop", leaders_reward_prop)
        );
    }

    action_result issuereward(symbol_code commun_code, bool for_leaders) {
        return push(N(issuereward), _code, args()
            ("commun_code", commun_code)
            ("for_leaders", for_leaders)
        );
    }
};

}} // eosio::testing
