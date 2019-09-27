#pragma once
#include "test_api_helper.hpp"

namespace eosio { namespace testing {

struct commun_list_api: base_contract_api {
    commun_list_api(golos_tester* tester, name code)
        :   base_contract_api(tester, code) {}

    action_result create_record(name sender, symbol_code token_name, std::string community_name) {
        return push(N(create), sender, args()
                    ("token_name", token_name)
                    ("community_name", community_name)
                    );
    }

    action_result add_info(name sender, symbol_code token_name, std::string community_name, std::string ticker = "ticker",
                           std::string avatar = "avatar", std::string cover_img_link = "cover_img_link",
                           std::string description = "description", std::string rules = "rules") {
        return push(N(addinfo), sender, args()
                    ("token_name", token_name)
                    ("community_name", community_name)
                    ("ticker", ticker)
                    ("avatar", avatar)
                    ("cover_img_link", cover_img_link)
                    ("description", description)
                    ("rules", rules)
                    );
    }
};

}} // eosio::testing

