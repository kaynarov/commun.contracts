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
    commun_posting_api(golos_tester* tester, name code)
    :   base_contract_api(tester, code) {}

    action_result create_msg(
        mssgid message_id,
        mssgid parent_id = {N(), "parentprmlnk"},
        std::string title = "headermssg",
        std::string body = "bodymssg",
        std::string language = "languagemssg",
        std::vector<std::string> tags = {"tag"},
        std::string json_metadata = "jsonmetadata"
    ) {
        return push(N(createmssg), message_id.author, args()
            ("message_id", message_id)
            ("parent_id", parent_id)
            ("headermssg", title)
            ("bodymssg", body)
            ("languagemssg", language)
            ("tags", tags)
            ("jsonmetadata", json_metadata)
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
            ("message_id", message_id)
        );
    }

    action_result set_params(std::string json_params) {
        return push(N(setparams), _code, args()
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

    //// posting tables
    variant get_permlink(account_name acc, uint64_t id) {
        return _tester->get_chaindb_struct(_code, acc, N(permlink), id, "permlink");
    }

    variant get_permlink(mssgid message_id) {
        variant obj = _tester->get_chaindb_lower_bound_struct(_code, message_id.author, N(permlink), N(byvalue),
                                                              message_id.get_unique_key(), "message");
        if (!obj.is_null() && obj.get_object().size()) {
            if(obj["value"].as<std::string>() == message_id.permlink) {
                return obj;
            }
        }
        return variant();
    }

    const uint16_t max_comment_depth = 127;
};


}} // eosio::testing

FC_REFLECT(eosio::testing::mssgid, (author)(permlink))
