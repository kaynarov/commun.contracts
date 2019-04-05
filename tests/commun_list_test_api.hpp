#pragma once
#include "test_api_helper.hpp"

namespace eosio { namespace testing {

struct community_contracts {
    name ctrl;
    name emit;
    name charge;
    name social;
    name publish;
};

struct commun_list_api: base_contract_api {
    commun_list_api(golos_tester* tester, name code)
        :   base_contract_api(tester, code) {}

    action_result create_record(name sender, std::string community_name, symbol_code token_name, community_contracts contracts) {
        return push(N(create), sender, args()
                    ("community_name", community_name)
                    ("token_name", token_name)
                    ("contracts", contracts)
                    );
    }
};

}} // eosio::testing

FC_REFLECT(eosio::testing::community_contracts,
           (ctrl)(emit)(charge)(social)(publish))
