#pragma once
#include "test_api_helper.hpp"

namespace eosio { namespace testing {

struct commun_list_api: base_contract_api {
    commun_list_api(golos_tester* tester, name code)
        :   base_contract_api(tester, code) {}

    action_result create_record(name signer, symbol_code commun_code, std::string community_name) {
        return push(N(create), signer, args()
                    ("commun_code", commun_code)
                    ("community_name", community_name)
                    );
    }

    action_result setparams(name signer, symbol_code commun_code, mvo params) {
        return push(N(setparams), signer, params
            ("commun_code", commun_code)
        );
    }

    action_result setinfo(symbol_code commun_code, std::string description = "description",
            std::string language = "language", std::string rules = "rules",
            std::string avatar_image = "avatar_image", std::string cover_image = "cover_image") {
        return push(N(setinfo), _code, args()
            ("commun_code", commun_code)
            ("description", description)
            ("language", language)
            ("rules", rules)
            ("avatar_image", avatar_image)
            ("cover_image", cover_image)
        );
    }
};

}} // eosio::testing

