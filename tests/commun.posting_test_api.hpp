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
    
    action_result push_maybe_msig(account_name act, account_name actor, mvo a, account_name client) {
        return client ?
            push_msig(act, {{actor, config::active_name}, {_code, cfg::client_permission_name}}, 
                {actor, client}, a) : 
            push(act, actor, a);
    }

    action_result create(
        mssgid message_id,
        mssgid parent_id = {N(), "parentprmlnk"},
        std::string title = "header",
        std::string body = "body",
        std::vector<std::string> tags = {"tag"},
        std::string metadata = "metadata",
        std::optional<uint16_t> weight = std::optional<uint16_t>(), 
        account_name client = account_name()
    ) {
        auto a = args()
            ("commun_code", commun_code)
            ("message_id", message_id)
            ("parent_id", parent_id)
            ("header", title)
            ("body", body)
            ("tags", tags)
            ("metadata", metadata);

        if (weight.has_value()) {
            a("weight", *weight);
        }
        
        return push_maybe_msig(N(create), message_id.author, a, client);
    }

    action_result update(
        mssgid message_id,
        std::string title,
        std::string body,
        std::vector<std::string> tags,
        std::string metadata,
        account_name client = account_name()
    ) {
        auto a = args()
            ("commun_code", commun_code)
            ("message_id", message_id)
            ("header", title)
            ("body", body)
            ("tags",tags)
            ("metadata", metadata);
        
        return push_maybe_msig(N(update), message_id.author, a, client);
    }

    action_result settags(name leader,
        mssgid message_id,
        std::vector<std::string> add_tags,
        std::vector<std::string> remove_tags,
        std::string reason,
        account_name client = account_name()
    ) {
        auto a = args()
            ("commun_code", commun_code)
            ("leader", leader)
            ("message_id", message_id)
            ("add_tags", add_tags)
            ("remove_tags", remove_tags)
            ("reason", reason);
        return push_maybe_msig(N(settags), leader, a, client);
    }

    action_result remove(mssgid message_id, account_name client = account_name()) {
        auto a = args()
            ("commun_code", commun_code)
            ("message_id", message_id);
        return push_maybe_msig(N(remove), message_id.author, a, client);
    }

    action_result report(name reporter,
        mssgid message_id,
        std::string reason,
        account_name client = account_name()
    ) {
        auto a = args()
            ("commun_code", commun_code)
            ("reporter", reporter)
            ("message_id", message_id)
            ("reason", reason);
        return push_maybe_msig(N(report), reporter, a, client);
    }

    action_result reblog(
        account_name rebloger,
        mssgid message_id,
        std::string title = "header",
        std::string body = "body",
        account_name client = account_name()
    ) {
        auto a = args()
            ("commun_code", commun_code)
            ("rebloger", rebloger)
            ("message_id", message_id)
            ("header", title)
            ("body", body);
        return push_maybe_msig(N(reblog), rebloger, a, client);
    }

    action_result erase_reblog(
        account_name rebloger,
        mssgid message_id,
        account_name client = account_name()
    ) {
        auto a = args()
            ("commun_code", commun_code)
            ("rebloger", rebloger)
            ("message_id", message_id);
        return push_maybe_msig(N(erasereblog), rebloger, a, client);
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
    
    action_result vote(bool damn, account_name voter, mssgid message_id, 
                       std::optional<uint16_t> weight = std::optional<uint16_t>(), account_name client = account_name()) {
        auto act = damn ? N(downvote) : N(upvote);
        auto a = get_vote_args(voter, message_id, weight);
        return push_maybe_msig(act, voter, a, client);
    }

    action_result upvote(account_name voter, mssgid message_id, 
                         std::optional<uint16_t> weight = std::optional<uint16_t>(), account_name client = account_name()) {
        return vote(false, voter, message_id, weight, client);
    }
    action_result downvote(account_name voter, mssgid message_id, 
                         std::optional<uint16_t> weight = std::optional<uint16_t>(), account_name client = account_name()) {
        return vote(true, voter, message_id, weight, client);
    }
    action_result unvote(account_name voter, mssgid message_id, account_name client = account_name()) {
        auto a = args()
            ("commun_code", commun_code)
            ("voter", voter)
            ("message_id", message_id);
        return push_maybe_msig(N(unvote), voter, a, client);
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

    action_result advise(account_name leader, std::vector<mssgid> favorites) { // vector is to test if duplicated
        return push(N(advise), leader, args()
            ("commun_code", commun_code)
            ("leader", leader)
            ("favorites", favorites)
        );
    }

    action_result lock(account_name leader, mssgid message_id, const std::string& reason) {
        return push(N(lock), leader, args()
            ("commun_code", commun_code)
            ("leader", leader)
            ("message_id", message_id)
            ("reason", reason)
        );
    }

    action_result unlock(account_name leader, mssgid message_id, const std::string& reason) {
        return push(N(unlock), leader, args()
            ("commun_code", commun_code)
            ("leader", leader)
            ("message_id", message_id)
            ("reason", reason)
        );
    }

    action_result ban(account_name issuer, mssgid message_id) {
        return push(N(ban), issuer, args()
            ("commun_code", commun_code)
            ("message_id", message_id)
        );
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
