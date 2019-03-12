#pragma once
#include "test_api_helper.hpp"


namespace eosio { namespace testing {

struct commun_list_api: base_contract_api {
    commun_list_api(golos_tester* tester, name code)
    :   base_contract_api(tester, code) {}

};

}} // eosio::testing
