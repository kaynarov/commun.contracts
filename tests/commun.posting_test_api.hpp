#pragma once
#include "test_api_helper.hpp"
#include <contracts.hpp>
#include <commun/config.hpp>

namespace cfg = commun::config;

namespace eosio { namespace testing {

struct mssgid {
    eosio::chain::name author;
    std::string permlink;

    uint64_t tracery() const {
        return fc::hash64(permlink.c_str(), permlink.size());
    }

    bool operator ==(const mssgid& rhs) const {
        return author == rhs.author && permlink == rhs.permlink;
    }
};

struct commun_posting_api: base_contract_api {
    symbol_code commun_code;
    commun_posting_api(golos_tester* tester, name code, symbol_code comn_code)
    :   base_contract_api(tester, code)
    ,   commun_code(comn_code) {}

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

    action_result settags(name leader,
        mssgid message_id,
        std::vector<std::string> add_tags,
        std::vector<std::string> remove_tags,
        std::string reason
    ) {
        return push(N(settags), leader, args()
            ("commun_code", commun_code)
            ("leader", leader)
            ("message_id", message_id)
            ("add_tags", add_tags)
            ("remove_tags", remove_tags)
            ("reason", reason)
        );
    }

    action_result delete_msg(mssgid message_id) {
        return push(N(deletemssg), message_id.author, args()
            ("commun_code", commun_code)
            ("message_id", message_id)
        );
    }

    action_result report_mssg(name reporter,
        mssgid message_id,
        std::string reason
    ) {
        return push(N(reportmssg), reporter, args()
            ("commun_code", commun_code)
            ("reporter", reporter)
            ("message_id", message_id)
            ("reason", reason)
        );
    }

    action_result reblog_msg(
        account_name rebloger,
        mssgid message_id,
        std::string title = "headermssg",
        std::string body = "bodymssg"
    ) {
        return push(N(reblog), rebloger, args()
            ("commun_code", commun_code)
            ("rebloger", rebloger)
            ("message_id", message_id)
            ("headermssg", title)
            ("bodymssg", body)
        );
    }

    action_result erase_reblog_msg(
        account_name rebloger,
        mssgid message_id
    ) {
        return push(N(erasereblog), rebloger, args()
            ("commun_code", commun_code)
            ("rebloger", rebloger)
            ("message_id", message_id)
        );
    }

    action_result upvote(account_name voter, mssgid message_id, uint16_t weight = cfg::_100percent) {
        return push(N(upvote), voter, args()
            ("commun_code", commun_code)
            ("voter", voter)
            ("message_id", message_id)
            ("weight", weight)
        );
    }
    action_result downvote(account_name voter, mssgid message_id, uint16_t weight = cfg::_100percent) {
        return push(N(downvote), voter, args()
            ("commun_code", commun_code)
            ("voter", voter)
            ("message_id", message_id)
            ("weight", weight)
        );
    }
    action_result unvote(account_name voter, mssgid message_id) {
        return push(N(unvote), voter, args()
            ("commun_code", commun_code)
            ("voter", voter)
            ("message_id", message_id)
        );
    }
    
    action_result claim(mssgid message_id, account_name gem_owner, 
            account_name gem_creator = account_name(), bool eager = false, account_name signer = account_name()) {
        auto a = args()
            ("commun_code", commun_code)
            ("message_id", message_id)
            ("gem_owner", gem_owner);
        if (gem_creator) {
            a("gem_creator", gem_creator);
        }
        if (eager) {
            a("eager", true);
        }
        
        return push(N(claim), signer ? signer : gem_owner, a);
    }

    action_result hold(mssgid message_id, account_name gem_owner, account_name gem_creator = account_name()) {
        auto a = args()
            ("commun_code", commun_code)
            ("message_id", message_id)
            ("gem_owner", gem_owner);
        if (gem_creator) {
            a("gem_creator", gem_creator);
        }
        return push(N(hold), gem_owner, a);
    }

    action_result transfer(mssgid message_id, account_name gem_owner, account_name gem_creator, account_name recipient, bool recipient_sign = true) {
        auto a = args()
            ("commun_code", commun_code)
            ("message_id", message_id)
            ("gem_owner", gem_owner);
        if (gem_creator) {
            a("gem_creator", gem_creator);
        }
        a("recipient", recipient);

        return recipient_sign ? 
            push_msig(N(transfer), {{gem_owner, config::active_name}, {recipient, config::active_name}}, {gem_owner, recipient}, a) :
            push(N(transfer), gem_owner, a);
    }

    action_result setproviders(account_name recipient, std::vector<account_name> providers) {
        return push(N(setproviders), recipient, args()
            ("commun_code", commun_code)
            ("recipient", recipient)
            ("providers", providers)
        );
    }

    action_result setfrequency(account_name account, uint16_t actions_per_day) {
        return push(N(setfrequency), account, args()
            ("commun_code", commun_code)
            ("account", account)
            ("actions_per_day", actions_per_day)
        );
    }

    action_result advise(account_name leader, std::vector<mssgid> favorites) {
        return push(N(advise), leader, args()
            ("commun_code", commun_code)
            ("leader", leader)
            ("favorites", favorites));
    }

    action_result slap(account_name leader, mssgid message_id) {
        return push(N(slap), leader, args()
            ("commun_code", commun_code)
            ("leader", leader)
            ("message_id", message_id));
    }

    action_result set_params(std::string json_params) {
        return push(N(setparams), _code, args()
            ("commun_code", commun_code)
            ("params", json_str_to_obj(json_params)));
    }

    action_result init_default_params() {
        auto comment_depth = get_str_comment_depth(max_comment_depth);

        auto params = "[" + comment_depth + "]";
        return set_params(params);
    }

    string get_str_comment_depth(uint16_t max_comment_depth) {
        return string("['st_max_comment_depth', {'value':'") + std::to_string(max_comment_depth) + "'}]";
    }

    variant get_vertex(mssgid message_id) {
        variant obj = _tester->get_chaindb_lower_bound_struct(_code, commun_code.value, N(vertex), N(bykey),
            std::make_pair(message_id.author, message_id.tracery()), "message");
        if (!obj.is_null() && obj.get_object().size()) {
            if (obj["creator"] == message_id.author.to_string() && obj["tracery"].as<uint64_t>() == message_id.tracery()) {
                return obj;
            }
        }
        return variant();
    }

    variant get_accparam(account_name acc) {
        return _tester->get_chaindb_struct(_code, commun_code.value, N(accparam), acc.value, "accparam");
    }

    uint64_t tracery(std::string permlink) const {
        return mssgid{name(), permlink}.tracery();
    }

    const uint16_t max_comment_depth = 127;
};

}} // eosio::testing

FC_REFLECT(eosio::testing::mssgid, (author)(permlink))
