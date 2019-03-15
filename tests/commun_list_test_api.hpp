#pragma once
#include "test_api_helper.hpp"


namespace eosio { namespace testing {

struct commun_list_api: base_contract_api {
    commun_list_api(golos_tester* tester, name code)
        :   base_contract_api(tester, code) {}

    action_result create_record(symbol symbol, name token, name ctrl = N(), name vesting = N(), name emit = N(),
                                name charge = N(), name posting = N(), name social = N(), name referral = N()) {
        return push(N(create), token, args()
                    ("symbol", symbol)
                    ("token", token)
                    ("ctrl", ctrl)
                    ("vesting", vesting)
                    ("emit", emit)
                    ("charge", charge)
                    ("posting", posting)
                    ("social", social)
                    ("referral", referral)
                    );
    }

    action_result update_record(symbol symbol, name token, name ctrl = N(), name vesting = N(), name emit = N(),
                                name charge = N(), name posting = N(), name social = N(), name referral = N()) {
        return push(N(update), token, args()
                    ("symbol", symbol)
                    ("token", token)
                    ("ctrl", ctrl)
                    ("vesting", vesting)
                    ("emit", emit)
                    ("charge", charge)
                    ("posting", posting)
                    ("social", social)
                    ("referral", referral)
                    );
    }

    variant get_record(symbol symbol, name token) {
        variant obj = _tester->get_chaindb_lower_bound_struct(_code, token, N(community), N(secondary),
                                                              std::pair<uint64_t, uint64_t>(symbol.value(), token.value),
                                                              "community");
        if (!obj.is_null())
            return variant();
        return obj;
    }
};

}} // eosio::testing
