#include <commun.publication/commun.publication.hpp>
#include <eosio/event.hpp>
#include <commun.publication/objects.hpp>
#include <commun.social/commun.social.hpp>
#include <commun/dispatchers.hpp>

namespace commun {

struct posting_params_setter: set_params_visitor<posting_state> {
    using set_params_visitor::set_params_visitor;

    bool operator()(const max_comment_depth_prm& param) {
        return set_param(param, &posting_state::max_comment_depth_param);
    }
};

// cached to prevent unneeded db access
const posting_state& publication::params(symbol_code commun_code) {
    static const auto cfg = posting_params_singleton(_self, commun_code.raw()).get();
    return cfg;
}

void publication::createmssg(
    symbol_code commun_code,
    mssgid_t message_id,
    mssgid_t parent_id,
    std::string headermssg,
    std::string bodymssg,
    std::string languagemssg,
    std::vector<std::string> tags,
    std::string jsonmetadata,
    uint16_t curators_prcnt
) {
    require_auth(message_id.author);

    eosio::check(message_id.permlink.length() && message_id.permlink.length() < config::max_length,
        "Permlink length is empty or more than 256.");
    eosio::check(validate_permlink(message_id.permlink), "Permlink contains wrong symbol.");
    eosio::check(headermssg.length() < config::max_length, "Title length is more than 256.");
    eosio::check(bodymssg.length(), "Body is empty.");
    eosio::check(curators_prcnt <= config::_100percent, "curators_prcnt can't be more than 100%.");

    const auto& max_comment_depth_param = params(commun_code).max_comment_depth_param;

    vertices vertices_table(_self, commun_code.raw());
    auto vertices_index = vertices_table.get_index<"bykey"_n>();
    auto tracery = message_id.tracery();
    eosio::check(vertices_index.find(std::make_tuple(message_id.author, tracery)) == vertices_index.end(), "This message already exists.");

    uint64_t parent_pk = 0;
    uint16_t level = 0;
    uint64_t parent_tracery = 0;
    if (parent_id.author) {
        parent_tracery = parent_id.tracery();
        auto parent_vertex = vertices_index.find(std::make_tuple(parent_id.author, parent_tracery));
        eosio::check(parent_vertex != vertices_index.end(), "Parent message doesn't exist");

        vertices_index.modify(parent_vertex, eosio::same_payer, [&](auto& item) {
            ++item.childcount;
        });

        level = 1 + parent_vertex->level;
    }
    eosio::check(level <= max_comment_depth_param.max_comment_depth, "publication::createmssg: level > MAX_COMMENT_DEPTH");

    vertices_table.emplace(message_id.author, [&](auto& item) {
        item.id = vertices_table.available_primary_key();
        item.creator = message_id.author;
        item.tracery = tracery;
        item.parent_creator = parent_id.author;
        item.parent_tracery = parent_tracery;
        item.level = level;
        item.childcount = 0;
    });

    gallery_types::params params_table(_self, commun_code.raw());
    const auto& param = params_table.get(commun_code.raw(), "param does not exists");
    accparams accparams_table(_self, commun_code.raw());
    auto acc_param = get_acc_param(accparams_table, commun_code, message_id.author);

    asset quantity(
        get_amount_to_freeze(
            point::get_balance(config::point_name, message_id.author, commun_code).amount, 
            get_frozen_amount(_self, message_id.author, commun_code),
            acc_param->actions_per_day, 
            param.mosaic_active_period), 
        param.commun_symbol);

    create_mosaic(_self, message_id.author, tracery, quantity, config::_100percent - curators_prcnt, get_providers(commun_code, message_id.author));    
}

void publication::updatemssg(symbol_code commun_code, mssgid_t message_id,
        std::string headermssg, std::string bodymssg,
        std::string languagemssg, std::vector<std::string> tags, std::string jsonmetadata) {
    require_auth(message_id.author);
    vertices vertices_table(_self, commun_code.raw());
    auto vertices_index = vertices_table.get_index<"bykey"_n>();
    eosio::check(vertices_index.find(std::make_tuple(message_id.author, message_id.tracery())) != vertices_index.end(), 
        "You can't update this message, because this message doesn't exist.");
}

void publication::deletemssg(symbol_code commun_code, mssgid_t message_id) {
    auto tracery = message_id.tracery();
    claim_gems_by_creator(_self, message_id.author, tracery, commun_code, message_id.author, true);
    gallery_types::mosaics mosaics_table(_self, commun_code.raw());
    auto mosaics_idx = mosaics_table.get_index<"bykey"_n>();
    eosio::check(mosaics_idx.find(std::make_tuple(message_id.author, tracery)) == mosaics_idx.end(), "Unable to delete comment with votes.");
}

void publication::upvote(symbol_code commun_code, name voter, mssgid_t message_id, uint16_t weight) {
    eosio::check(weight > 0, "weight can't be 0.");
    eosio::check(weight <= config::_100percent, "weight can't be more than 100%.");
    set_vote(commun_code, voter, message_id, weight);
}

void publication::downvote(symbol_code commun_code, name voter, mssgid_t message_id, uint16_t weight) {
    eosio::check(weight > 0, "weight can't be 0.");
    eosio::check(weight <= config::_100percent, "weight can't be more than 100%.");
    set_vote(commun_code, voter, message_id, -weight);
}

void publication::unvote(symbol_code commun_code, name voter, mssgid_t message_id) {
    eosio::check(voter != message_id.author, "author can't unvote");
    claim_gems_by_creator(_self, message_id.author, message_id.tracery(), commun_code, voter, true);
}

void publication::hold(symbol_code commun_code, mssgid_t message_id, name gem_owner, std::optional<name> gem_creator) {
    hold_gem(_self, message_id.author, message_id.tracery(), commun_code, gem_owner, gem_creator.value_or(gem_owner));
}

void publication::transfer(symbol_code commun_code, mssgid_t message_id, name gem_owner, std::optional<name> gem_creator, name recipient) {
    transfer_gem(_self, message_id.author, message_id.tracery(), commun_code, gem_owner, gem_creator.value_or(gem_owner), recipient);
}

void publication::claim(symbol_code commun_code, mssgid_t message_id, name gem_owner, 
                        std::optional<name> gem_creator, std::optional<bool> eager) {
    
    claim_gem(_self, message_id.author, message_id.tracery(), commun_code, gem_owner, gem_creator.value_or(gem_owner), eager.value_or(false));
}

void publication::set_vote(symbol_code commun_code, name voter, const mssgid_t& message_id, int16_t weight) {
    gallery_types::params params_table(_self, commun_code.raw());
    const auto& param = params_table.get(commun_code.raw(), "param does not exists");
    accparams accparams_table(_self, commun_code.raw());
    auto acc_param = get_acc_param(accparams_table, commun_code, voter);

    uint16_t abs_weight = std::abs(weight);
    asset quantity(
        safe_pct(abs_weight, get_amount_to_freeze(
            point::get_balance(config::point_name, voter, commun_code).amount, 
            get_frozen_amount(_self, voter, commun_code),
            acc_param->actions_per_day, 
            param.mosaic_active_period)), 
        param.commun_symbol);

    add_to_mosaic(_self, message_id.author, message_id.tracery(), quantity, weight < 0, voter, get_providers(commun_code, message_id.author, abs_weight));
}

void publication::setparams(symbol_code commun_code, std::vector<posting_params> params) {
    require_auth(_self);

    gallery_types::params params_table(_self, commun_code.raw());
    if (params_table.find(commun_code.raw()) == params_table.end()) {
        create_gallery(_self, point::get_supply(config::point_name, commun_code).symbol);
    }

    posting_params_singleton cfg(_self, commun_code.raw());
    param_helper::check_params(params, cfg.exists());
    param_helper::set_parameters<posting_params_setter>(params, cfg, _self);
}

void publication::reblog(symbol_code commun_code, name rebloger, mssgid_t message_id, std::string headermssg, std::string bodymssg) {
    require_auth(rebloger);

    eosio::check(rebloger != message_id.author, "You cannot reblog your own content.");
    eosio::check(headermssg.length() < config::max_length, "Title length is more than 256.");
    eosio::check(
        !headermssg.length() || (headermssg.length() && bodymssg.length()),
        "Body must be set if title is set."
    );

    vertices vertices_table(_self, commun_code.raw());
    auto vertices_index = vertices_table.get_index<"bykey"_n>();
    eosio::check(vertices_index.find(std::make_tuple(message_id.author, message_id.tracery())) != vertices_index.end(), 
        "You can't reblog, because this message doesn't exist.");
}

void publication::erasereblog(symbol_code commun_code, name rebloger, mssgid_t message_id) {
    require_auth(rebloger);
    eosio::check(rebloger != message_id.author, "You cannot erase reblog your own content.");

    vertices vertices_table(_self, commun_code.raw());
    auto vertices_index = vertices_table.get_index<"bykey"_n>();
    eosio::check(vertices_index.find(std::make_tuple(message_id.author, message_id.tracery())) != vertices_index.end(), 
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

accparams::const_iterator publication::get_acc_param(accparams& accparams_table, symbol_code commun_code, name account) {
    auto ret = accparams_table.find(account.value);
    if (ret == accparams_table.end()) {
        ret = accparams_table.emplace(account, [&](auto& p) { p = {
            .account = account,
            .actions_per_day = config::default_actions_per_day
        };});
    }
    return ret;
}

int64_t publication::get_amount_to_freeze(int64_t balance, int64_t frozen, uint16_t actions_per_day, int64_t mosaic_active_period, int64_t actual_limit) {
    int64_t available = balance - frozen;
    if (available <= 0 || actual_limit <= 0) {
        return 0;
    }
    static const int64_t seconds_per_day = 24 * 60 * 60;
    int64_t actions_per_period = std::max<int64_t>(safe_prop(actions_per_day, mosaic_active_period, seconds_per_day), 1);
    int64_t points_per_action = balance / actions_per_period;
    return std::min(points_per_action ? std::min(available, points_per_action) : available, actual_limit);
}

gallery_types::providers_t publication::get_providers(symbol_code commun_code, name account, uint16_t weight) {
    gallery_types::params params_table(_self, commun_code.raw());
    const auto& param = params_table.get(commun_code.raw(), "param does not exists");
    accparams accparams_table(_self, commun_code.raw());
    auto acc_param = get_acc_param(accparams_table, commun_code, account);
    gallery_types::provs provs_table(_self, commun_code.raw());
    gallery_types::providers_t ret;
    auto provs_index = provs_table.get_index<"bykey"_n>();
    for (size_t n = 0; n < acc_param->providers.size(); n++) {
        auto prov_name = acc_param->providers[n];
        auto prov_itr = provs_index.find(std::make_tuple(prov_name, account));
        if (prov_itr != provs_index.end() && point::balance_exists(config::point_name, prov_name, commun_code)) {
            auto amount = safe_pct(weight, get_amount_to_freeze(
                prov_itr->total, 
                prov_itr->frozen,
                acc_param->actions_per_day, 
                param.mosaic_active_period,
                point::get_balance(config::point_name, prov_name, commun_code).amount - get_frozen_amount(_self, prov_name, commun_code)));

            if (amount) {
                ret.emplace_back(std::make_pair(prov_name, amount));
            }
        }
        else {
            accparams_table.modify(acc_param, name(), [&](auto& a) { a.providers[n] = name(); });
        }
    }
    accparams_table.modify(acc_param, name(), [&](auto& a) {
        a.providers.erase(
            std::remove_if(a.providers.begin(), a.providers.end(), [](const name& p) { return p == name(); }),
            a.providers.end());
    });
    return ret;
}

void publication::setproviders(symbol_code commun_code, name recipient, std::vector<name> providers) {
    require_auth(recipient);
    gallery_types::params params_table(_self, commun_code.raw());
    const auto& param = params_table.get(commun_code.raw(), "param does not exists");

    gallery_types::provs provs_table(_self, commun_code.raw());
    auto provs_index = provs_table.get_index<"bykey"_n>();
    for (size_t n = 0; n < providers.size(); n++) {
        auto prov_name = providers[n];
        auto prov_itr = provs_index.find(std::make_tuple(prov_name, recipient));
        if (prov_itr == provs_index.end() || !point::balance_exists(config::point_name, prov_name, commun_code)) {
            providers[n] = name();
        }
    }
    providers.erase(std::remove_if(providers.begin(), providers.end(), [](const name& p) { return p == name(); }), providers.end());
    eosio::check(providers.size() <= config::max_providers_num, "too many providers");

    accparams accparams_table(_self, commun_code.raw());
    auto acc_param = get_acc_param(accparams_table, commun_code, recipient);
    accparams_table.modify(acc_param, name(), [&](auto& a) { a.providers = providers; });
}

void publication::setfrequency(symbol_code commun_code, name account, uint16_t actions_per_day) {
    require_auth(account);
    gallery_types::params params_table(_self, commun_code.raw());
    const auto& param = params_table.get(commun_code.raw(), "param does not exists");

    accparams accparams_table(_self, commun_code.raw());
    auto acc_param = get_acc_param(accparams_table, commun_code, account);
    accparams_table.modify(acc_param, name(), [&](auto& a) { a.actions_per_day = actions_per_day; });
}

void publication::provide(name grantor, name recipient, asset quantity, std::optional<uint16_t> fee) {
    provide_points(_self, grantor, recipient, quantity, fee);
}

void publication::advise(symbol_code commun_code, name leader, std::vector<mssgid_t> favorites) {
    std::vector<gallery_types::mosaic_key_t> favorite_mosaics;
    favorite_mosaics.reserve(favorites.size());
    for (const auto& m : favorites) {
        favorite_mosaics.push_back({m.author, m.tracery()});
    }
    advise_mosaics(_self, commun_code, leader, favorite_mosaics);
}

void publication::slap(symbol_code commun_code, name leader, mssgid_t message_id) {
    slap_mosaic(_self, commun_code, leader, message_id.author, message_id.tracery());
}

} // commun

DISPATCH_WITH_TRANSFER(commun::publication, commun::config::point_name, ontransfer,
    (createmssg)(updatemssg)(deletemssg)(upvote)(downvote)(unvote)(claim)(hold)(transfer)(setparams)
    (reblog)(erasereblog)(setproviders)(setfrequency)(provide)(advise)(slap))
