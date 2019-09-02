#pragma once
#include "test_api_helper.hpp"
#include <contracts.hpp>

namespace eosio { namespace testing {

struct mssgid {
    eosio::chain::name author;
    std::string permlink;

    auto get_unique_key() const {
        return permlink;
    }

    bool operator ==(const mssgid& rhs) const {
        return author == rhs.author && permlink == rhs.permlink;
    }
};

struct commun_posting_api: base_contract_api {
    symbol_code commun_code;
    commun_posting_api(golos_tester* tester, name code, symbol_code cmmn_code)
    :   base_contract_api(tester, code)
        ,commun_code(cmmn_code) {}

    action_result create_msg(
        mssgid message_id,
        mssgid parent_id = {N(), "parentprmlnk"},
        std::string title = "headermssg",
        std::string body = "bodymssg",
        std::string language = "languagemssg",
        std::vector<std::string> tags = {"tag"},
        std::string json_metadata = "jsonmetadata",
        uint16_t curators_prcnt = 5000
        
    ) {
        return push(N(createmssg), message_id.author, args()
            ("commun_code", commun_code)
            ("message_id", message_id)
            ("parent_id", parent_id)
            ("headermssg", title)
            ("bodymssg", body)
            ("languagemssg", language)
            ("tags", tags)
            ("jsonmetadata", json_metadata)
            ("curators_prcnt", curators_prcnt)
        );
    }

    action_result update_msg(
        mssgid message_id,
        std::string title,
        std::string body,
        std::string language,
        std::vector<std::string> tags,
        std::string json_metadata
    ) {
        return push(N(updatemssg), message_id.author, args()
            ("commun_code", commun_code)
            ("message_id", message_id)
            ("headermssg", title)
            ("bodymssg", body)
            ("languagemssg", language)
            ("tags",tags)
            ("jsonmetadata", json_metadata)
        );
    }

    action_result delete_msg(mssgid message_id) {
        return push(N(deletemssg), message_id.author, args()
            ("commun_code", commun_code)
            ("message_id", message_id)
        );
    }

    action_result set_params(std::string json_params) {
        return push(N(setparams), _code, args()
            ("commun_code", commun_code)
            ("params", json_str_to_obj(json_params)));
    }

    action_result init_default_params() {
        auto comment_depth = get_str_comment_depth(max_comment_depth);
        auto social = get_str_social_acc(name());

        auto params = "[" + comment_depth + "," + social + "]";
        return set_params(params);
    }

    string get_str_comment_depth(uint16_t max_comment_depth) {
        return string("['st_max_comment_depth', {'value':'") + std::to_string(max_comment_depth) + "'}]";
    }

    string get_str_social_acc(name social_acc) {
        return string("['st_social_acc', {'value':'") + name{social_acc}.to_string() + "'}]";
    }

    const uint16_t max_comment_depth = 127;
};


}} // eosio::testing

FC_REFLECT(eosio::testing::mssgid, (author)(permlink))
