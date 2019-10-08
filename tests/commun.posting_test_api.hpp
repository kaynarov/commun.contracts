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
        std::string key = author.to_string() + "/" + permlink;
        return fc::hash64(key.c_str(), key.size());
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
        std::string title = "header",
        std::string body = "body",
        std::vector<std::string> tags = {"tag"},
        std::string metadata = "metadata",
        uint16_t curators_prcnt = 5000,
        std::optional<uint16_t> weight = std::optional<uint16_t>()
    ) {
        auto a = args()
            ("commun_code", commun_code)
            ("message_id", message_id)
            ("parent_id", parent_id)
            ("header", title)
            ("body", body)
            ("tags", tags)
            ("metadata", metadata)
            ("curators_prcnt", curators_prcnt);

        if (weight.has_value()) {
            a("weight", *weight);
        }
        return push(N(createmssg), message_id.author, a);
    }

    action_result update_msg(
        mssgid message_id,
        std::string title,
        std::string body,
        std::vector<std::string> tags,
        std::string metadata
    ) {
        return push(N(updatemssg), message_id.author, args()
            ("commun_code", commun_code)
            ("message_id", message_id)
            ("header", title)
            ("body", body)
            ("tags",tags)
            ("metadata", metadata)
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
        std::string title = "header",
        std::string body = "body"
    ) {
        return push(N(reblog), rebloger, args()
            ("commun_code", commun_code)
            ("rebloger", rebloger)
            ("message_id", message_id)
            ("header", title)
            ("body", body)
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
    
    mvo get_vote_args(account_name voter, mssgid message_id, std::optional<uint16_t> weight = std::optional<uint16_t>()) {
        auto ret = args()
            ("commun_code", commun_code)
            ("voter", voter)
            ("message_id", message_id);
        if (weight.has_value()) {
            ret("weight", *weight);
        }
        return ret;
    }

    action_result upvote(account_name voter, mssgid message_id, std::optional<uint16_t> weight = std::optional<uint16_t>()) {
        return push(N(upvote), voter, get_vote_args(voter, message_id, weight));
    }
    action_result downvote(account_name voter, mssgid message_id, std::optional<uint16_t> weight = std::optional<uint16_t>()) {
        return push(N(downvote), voter, get_vote_args(voter, message_id, weight));
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

    variant get_vertex(mssgid message_id) {
        return get_struct(commun_code.value, N(vertex), message_id.tracery(), "vertex");
    }

    variant get_accparam(account_name acc) {
        return _tester->get_chaindb_struct(_code, commun_code.value, N(accparam), acc.value, "accparam");
    }
};

}} // eosio::testing

FC_REFLECT(eosio::testing::mssgid, (author)(permlink))
