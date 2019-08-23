#include "golos.publication.hpp"
#include <eosio/event.hpp>
#include "utils.hpp"
#include "objects.hpp"
#include <cyberway.contracts/common/util.hpp>
#include <commun.social/commun.social.hpp>


namespace golos {

extern "C" {
    void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        //publication(receiver).apply(code, action);
        auto execute_action = [&](const auto fn) {
            return eosio::execute_action(eosio::name(receiver), eosio::name(code), fn);
        };

#define NN(x) N(x).value

        if (receiver != code)
            return;

        if (NN(createmssg) == action)
            execute_action(&publication::create_message);
        if (NN(updatemssg) == action)
            execute_action(&publication::update_message);
        if (NN(deletemssg) == action)
            execute_action(&publication::delete_message);
        if (NN(upvote) == action)
            execute_action(&publication::upvote);
        if (NN(downvote) == action)
            execute_action(&publication::downvote);
        if (NN(unvote) == action)
            execute_action(&publication::unvote);
        if (NN(setparams) == action)
            execute_action(&publication::set_params);
        if (NN(reblog) == action)
            execute_action(&publication::reblog);
        if (NN(erasereblog) == action)
            execute_action(&publication::erase_reblog);
        if (NN(deletevotes) == action)
            execute_action(&publication::deletevotes);
    }
#undef NN
}

struct posting_params_setter: set_params_visitor<posting_state> {
    std::optional<st_bwprovider> new_bwprovider;
    using set_params_visitor::set_params_visitor;

    bool operator()(const max_vote_changes_prm& param) {
        return set_param(param, &posting_state::max_vote_changes_param);
    }

    bool operator()(const max_comment_depth_prm& param) {
        return set_param(param, &posting_state::max_comment_depth_param);
    }

    bool operator()(const social_acc_prm& param) {
        return set_param(param, &posting_state::social_acc_param);
    }

    bool operator()(const bwprovider_prm& p) {
        new_bwprovider = p;
        return set_param(p, &posting_state::bwprovider_param);
    }};

// cached to prevent unneeded db access
const posting_state& publication::params() {
    static const auto cfg = posting_params_singleton(_self, _self.value).get();
    return cfg;
}

void publication::create_message(
    structures::mssgid message_id,
    structures::mssgid parent_id,
    std::string headermssg,
    std::string bodymssg,
    std::string languagemssg,
    std::vector<std::string> tags,
    std::string jsonmetadata
) {
    require_auth(message_id.author);

    eosio::check(message_id.permlink.length() && message_id.permlink.length() < config::max_length,
            "Permlink length is empty or more than 256.");
    eosio::check(validate_permlink(message_id.permlink), "Permlink contains wrong symbol.");
    eosio::check(headermssg.length() < config::max_length, "Title length is more than 256.");
    eosio::check(bodymssg.length(), "Body is empty.");

    const auto& max_comment_depth_param = params().max_comment_depth_param;
    const auto& social_acc_param = params().social_acc_param;

    if (parent_id.author) {
        if (social_acc_param.account) {
            eosio::check(!commun::commun_social::is_blocking(social_acc_param.account, parent_id.author, message_id.author),
                    "You are blocked by this account");
        }
    }

    tables::permlink_table permlink_table(_self, message_id.author.value);
    tables::message_table message_table(_self, _self.value);
    uint64_t message_pk = permlink_table.available_primary_key();
    if (message_pk == 0)
        message_pk = 1;

    auto permlink_index = permlink_table.get_index<"byvalue"_n>();
    auto permlink_itr = permlink_index.find(message_id.permlink);
    eosio::check(permlink_itr == permlink_index.end(), "This message already exists.");

    uint64_t parent_pk = 0;
    uint16_t level = 0;
    if(parent_id.author) {
        tables::permlink_table parent_table(_self, parent_id.author.value);
        auto parent_index = parent_table.get_index<"byvalue"_n>();
        auto parent_itr = parent_index.find(parent_id.permlink);
        eosio::check((parent_itr != parent_index.end()), "Parent message doesn't exist");

        parent_table.modify(*parent_itr, eosio::same_payer, [&](auto& item) {
            ++item.childcount;
        });

        parent_pk = parent_itr->id;
        level = 1 + parent_itr->level;
    }
    eosio::check(level <= max_comment_depth_param.max_comment_depth, "publication::create_message: level > MAX_COMMENT_DEPTH");

    message_table.emplace(message_id.author, [&]( auto &item ) {
        item.author = message_id.author;
        item.id = message_pk;
        item.date = static_cast<uint64_t>(eosio::current_time_point().time_since_epoch().count());
    });

    permlink_table.emplace(message_id.author, [&]( auto &item) {
        item.id = message_pk;
        item.parentacc = parent_id.author;
        item.parent_id = parent_pk;
        item.value = message_id.permlink;
        item.level = level;
        item.childcount = 0;
    });
}

void publication::update_message(structures::mssgid message_id,
                              std::string headermssg, std::string bodymssg,
                              std::string languagemssg, std::vector<std::string> tags,
                              std::string jsonmetadata) {
    require_auth(message_id.author);
    tables::permlink_table permlink_table(_self, message_id.author.value);
    auto permlink_index = permlink_table.get_index<"byvalue"_n>();
    auto permlink_itr = permlink_index.find(message_id.permlink);
    eosio::check(permlink_itr != permlink_index.end(),
                 "You can't update this message, because this message doesn't exist.");
}

void publication::delete_message(structures::mssgid message_id) {
    require_auth(message_id.author);

    tables::permlink_table permlink_table(_self, message_id.author.value);
    auto permlink_index = permlink_table.get_index<"byvalue"_n>();
    auto permlink_itr = permlink_index.find(message_id.permlink);
    eosio::check(permlink_itr != permlink_index.end(), "Permlink doesn't exist.");
    eosio::check(permlink_itr->childcount == 0, "You can't delete comment with child comments.");

    tables::message_table message_table(_self, _self.value);
    auto mssg_itr = message_table.find(permlink_itr->id);

    if (permlink_itr->parentacc) {
        tables::permlink_table parent_table(_self, permlink_itr->parentacc.value);
        auto parent_itr = parent_table.find(permlink_itr->parent_id);
        eosio::check(parent_itr != parent_table.end(), "Parent permlink doesn't exist.");

        parent_table.modify(*parent_itr, eosio::same_payer, [&](auto& item) {
            --item.childcount;
        });
    }
    permlink_index.erase(permlink_itr);

    if (mssg_itr == message_table.end()) {
        return;
    }

    cancel_deferred((static_cast<uint128_t>(mssg_itr->id) << 64) | message_id.author.value);

    auto remove_id = mssg_itr->id;
    message_table.erase(mssg_itr);
    send_deletevotes_trx(remove_id, message_id.author, message_id.author);
}

void publication::upvote(name voter, structures::mssgid message_id, uint16_t weight) {
    eosio::check(weight > 0, "weight can't be 0.");
    eosio::check(weight <= config::_100percent, "weight can't be more than 100%.");
    set_vote(voter, message_id, weight);
}

void publication::downvote(name voter, structures::mssgid message_id, uint16_t weight) {
    eosio::check(weight > 0, "weight can't be 0.");
    eosio::check(weight <= config::_100percent, "weight can't be more than 100%.");
    set_vote(voter, message_id, -weight);
}

void publication::unvote(name voter, structures::mssgid message_id) {
    set_vote(voter, message_id, 0);
}

void publication::providebw_for_trx(transaction& trx, const permission_level& provider) {
    if (provider.actor != name()) {
        trx.actions.emplace_back(action{provider, "cyber"_n, "providebw"_n, std::make_tuple(provider.actor, _self)});
    }
}

void publication::send_deletevotes_trx(int64_t message_id, name author, name payer) {
    transaction trx(eosio::current_time_point() + eosio::seconds(config::deletevotes_expiration_sec));
    trx.actions.emplace_back(action{permission_level(_self, config::code_name), _self, "deletevotes"_n, std::make_tuple(message_id, author)});
    providebw_for_trx(trx, params().bwprovider_param.provider);
    trx.delay_sec = 0;
    trx.send((static_cast<uint128_t>(message_id) << 64) | author.value, payer);
}

void publication::deletevotes(int64_t message_id, name author) {
    require_auth(_self);

    tables::vote_table vote_table(_self, author.value);
    auto votetable_index = vote_table.get_index<"messageid"_n>();
    auto vote_itr = votetable_index.lower_bound(std::make_tuple(message_id, INT64_MAX));
    size_t i = 0;
    for (auto vote_etr = votetable_index.end(); vote_itr != vote_etr;) {
        if (config::max_deletions_per_trx <= i++) {
            break;
        }
        auto& vote = *vote_itr;
        ++vote_itr;
        if (vote.message_id != message_id) {
            vote_itr = votetable_index.end();
            break;
        }
        vote_table.erase(vote);
    }

    if (vote_itr != votetable_index.end()) {
        send_deletevotes_trx(message_id, author, _self);
    }
}

void publication::set_vote(name voter, const structures::mssgid& message_id, int16_t weight) {

}

void publication::set_params(std::vector<posting_params> params) {
    require_auth(_self);
    posting_params_singleton cfg(_self, _self.value);
    param_helper::check_params(params, cfg.exists());
    auto setter = param_helper::set_parameters<posting_params_setter>(params, cfg, _self);
    if (setter.new_bwprovider) {
        auto provider = setter.new_bwprovider->provider;
        if (provider.actor != name()) {
            dispatch_inline("cyber"_n, "providebw"_n, {provider}, std::make_tuple(provider.actor, _self));
        }
    }
}

void publication::reblog(name rebloger, structures::mssgid message_id, std::string headermssg, std::string bodymssg) {
    require_auth(rebloger);

    eosio::check(rebloger != message_id.author, "You cannot reblog your own content.");
    eosio::check(headermssg.length() < config::max_length, "Title length is more than 256.");
    eosio::check(
        !headermssg.length() || (headermssg.length() && bodymssg.length()),
        "Body must be set if title is set."
    );

    tables::permlink_table permlink_table(_self, message_id.author.value);
    auto permlink_index = permlink_table.get_index<"byvalue"_n>();
    auto permlink_itr = permlink_index.find(message_id.permlink);
    eosio::check(permlink_itr != permlink_index.end(),
                 "You can't reblog, because this message doesn't exist.");
}

void publication::erase_reblog(name rebloger, structures::mssgid message_id) {
    require_auth(rebloger);
    eosio::check(rebloger != message_id.author, "You cannot erase reblog your own content.");

    tables::permlink_table permlink_table(_self, message_id.author.value);
    auto permlink_index = permlink_table.get_index<"byvalue"_n>();
    auto permlink_itr = permlink_index.find(message_id.permlink);
    eosio::check(permlink_itr != permlink_index.end(),
                 "You can't erase reblog, because this message doesn't exist.");
}

bool publication::validate_permlink(std::string permlink) {
    for (auto symbol : permlink) {
        if ((symbol >= '0' && symbol <= '9') ||
            (symbol >= 'a' && symbol <= 'z') ||
             symbol == '-') {
            continue;
        } else {
            return false;
        }
    }
    return true;
}

const auto& publication::get_message(const tables::message_table& messages, const structures::mssgid& message_id) {
    tables::permlink_table permlink_table(_self, message_id.author.value);
    auto permlink_index = permlink_table.get_index<"byvalue"_n>();
    auto permlink_itr = permlink_index.find(message_id.permlink);
    // rare case - not critical for performance
    eosio::check(permlink_itr != permlink_index.end(), "Permlink doesn't exist.");

    auto message_itr = messages.find(permlink_itr->id);
    eosio::check(message_itr != messages.end(), "Message doesn't exist in cashout window.");
    return *message_itr;
}

} // golos
