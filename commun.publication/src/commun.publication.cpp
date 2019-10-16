#include <commun.publication/commun.publication.hpp>
#include <eosio/event.hpp>
#include <commun.publication/objects.hpp>
#include <commun.list/commun.list.hpp>
#include <commun/dispatchers.hpp>

namespace commun {

void publication::create(
    symbol_code commun_code,
    mssgid_t message_id,
    mssgid_t parent_id,
    std::string header,
    std::string body,
    std::vector<std::string> tags,
    std::string metadata,
    std::optional<uint16_t> weight
) {
    require_auth(message_id.author);
    
    eosio::check(!weight.has_value() || (*weight > 0), "weight can't be 0.");
    eosio::check(!weight.has_value() || (*weight <= config::_100percent), "weight can't be more than 100%.");

    eosio::check(message_id.permlink.length() && message_id.permlink.length() < config::max_length,
        "Permlink length is empty or more than 256.");
    eosio::check(validate_permlink(message_id.permlink), "Permlink contains wrong symbol.");
    eosio::check(header.length() < config::max_length, "Title length is more than 256.");
    eosio::check(body.length(), "Body is empty.");

    vertices vertices_table(_self, commun_code.raw());
    auto tracery = message_id.tracery();
    eosio::check(vertices_table.find(tracery) == vertices_table.end(), "This message already exists.");

    uint64_t parent_pk = 0;
    uint16_t level = 0;
    uint64_t parent_tracery = 0;
    if (parent_id.author) {
        parent_tracery = parent_id.tracery();
        auto parent_vertex = vertices_table.find(parent_tracery);
        eosio::check(parent_vertex != vertices_table.end(), "Parent message doesn't exist");

        vertices_table.modify(parent_vertex, eosio::same_payer, [&](auto& item) {
            ++item.childcount;
        });

        level = 1 + parent_vertex->level;
    }
    eosio::check(level <= config::max_comment_depth, "publication::create: level > MAX_COMMENT_DEPTH");

    vertices_table.emplace(message_id.author, [&](auto& item) {
        item.tracery = tracery;
        item.parent_tracery = parent_tracery;
        item.level = level;
        item.childcount = 0;
    });

    auto community = commun_list::get_community(config::list_name, commun_code);
    accparams accparams_table(_self, commun_code.raw());
    auto acc_param = get_acc_param(accparams_table, commun_code, message_id.author);
    auto gems_per_period = get_gems_per_period(commun_code);

    int64_t amount_to_freeze = 0;
    if (parent_id.author) {
        eosio::check(!weight.has_value(), "weight is redundant for comments");
        auto opus_itr = community.opuses.find(opus_info{config::comment_opus_name});
        check(opus_itr != community.opuses.end(), "unknown opus, probably comments in the community are disabled");
        amount_to_freeze = std::max(opus_itr->mosaic_pledge, std::max(opus_itr->min_mosaic_inclusion, opus_itr->min_gem_inclusion));
    }
    else {
        amount_to_freeze = get_amount_to_freeze(point::get_balance(config::point_name, message_id.author, commun_code).amount, 
            get_frozen_amount(_self, message_id.author, commun_code), community.gems_per_day, weight);
    }
    asset quantity(amount_to_freeze, community.commun_symbol);

    //providers are not used for comments
    create_mosaic(_self, message_id.author, tracery, parent_id.author ? config::comment_opus_name : config::post_opus_name,
        quantity, community.author_percent,
        parent_id.author ? gallery_types::providers_t() : get_providers(commun_code, message_id.author, gems_per_period, weight));    
}

void publication::update(symbol_code commun_code, mssgid_t message_id,
        std::string header, std::string body,
        std::vector<std::string> tags, std::string metadata) {
    update_mosaic(_self, commun_code, message_id.author, message_id.tracery());
}

void publication::settags(symbol_code commun_code, name leader, mssgid_t message_id,
        std::vector<std::string> add_tags, std::vector<std::string> remove_tags, std::string reason) {
    require_auth(leader);
    eosio::check(!add_tags.empty() || !remove_tags.empty(), "No changes in tags.");
    eosio::check(control::in_the_top(config::control_name, commun_code, leader), (leader.to_string() + " is not a leader").c_str());
    check_mssg_exists(commun_code, message_id);
}

void publication::remove(symbol_code commun_code, mssgid_t message_id) {
    auto tracery = message_id.tracery();
    claim_gems_by_creator(_self, tracery, commun_code, message_id.author, true);
    gallery_types::mosaics mosaics_table(_self, commun_code.raw());
    eosio::check(mosaics_table.find(tracery) == mosaics_table.end(), "Unable to remove comment with votes.");
}

void publication::report(symbol_code commun_code, name reporter, mssgid_t message_id, std::string reason) {
    require_auth(reporter);
    eosio::check(!reason.empty(), "Reason cannot be empty.");
    check_mssg_exists(commun_code, message_id);
}

void publication::lock(symbol_code commun_code, name leader, mssgid_t message_id, string reason) {
    eosio::check(!reason.empty(), "Reason cannot be empty.");
    lock_mosaic(_self, commun_code, leader, message_id.tracery());
}

void publication::unlock(symbol_code commun_code, name leader, mssgid_t message_id, string reason) {
    eosio::check(!reason.empty(), "Reason cannot be empty.");
    unlock_mosaic(_self, commun_code, leader, message_id.tracery());
}

void publication::upvote(symbol_code commun_code, name voter, mssgid_t message_id, std::optional<uint16_t> weight) {
    eosio::check(!weight.has_value() || (*weight <= config::_100percent), "weight can't be more than 100%.");
    set_vote(commun_code, voter, message_id, weight, false);
}

void publication::downvote(symbol_code commun_code, name voter, mssgid_t message_id, std::optional<uint16_t> weight) {
    eosio::check(!weight.has_value() || (*weight <= config::_100percent), "weight can't be more than 100%.");
    set_vote(commun_code, voter, message_id, weight, true);
}

void publication::unvote(symbol_code commun_code, name voter, mssgid_t message_id) {
    eosio::check(voter != message_id.author, "author can't unvote");
    claim_gems_by_creator(_self, message_id.tracery(), commun_code, voter, true);
}

void publication::hold(symbol_code commun_code, mssgid_t message_id, name gem_owner, std::optional<name> gem_creator) {
    hold_gem(_self, message_id.tracery(), commun_code, gem_owner, gem_creator.value_or(gem_owner));
}

void publication::transfer(symbol_code commun_code, mssgid_t message_id, name gem_owner, std::optional<name> gem_creator, name recipient) {
    transfer_gem(_self, message_id.tracery(), commun_code, gem_owner, gem_creator.value_or(gem_owner), recipient);
}

void publication::claim(symbol_code commun_code, mssgid_t message_id, name gem_owner, 
                        std::optional<name> gem_creator, std::optional<bool> eager) {
    
    claim_gem(_self, message_id.tracery(), commun_code, gem_owner, gem_creator.value_or(gem_owner), eager.value_or(false));
}

void publication::set_vote(symbol_code commun_code, name voter, const mssgid_t& message_id, std::optional<uint16_t> weight, bool damn) {
    auto community = commun_list::get_community(config::list_name, commun_code);
    accparams accparams_table(_self, commun_code.raw());
    auto acc_param = get_acc_param(accparams_table, commun_code, voter);

    gallery_types::mosaics mosaics_table(_self, commun_code.raw());
    auto mosaic = mosaics_table.get(message_id.tracery(), "mosaic doesn't exist");

    auto gems_per_period = get_gems_per_period(commun_code, mosaic.close_date.sec_since_epoch());

    asset quantity(
        get_amount_to_freeze(
            point::get_balance(config::point_name, voter, commun_code).amount, 
            get_frozen_amount(_self, voter, commun_code),
            gems_per_period,
            weight), 
        community.commun_symbol);

    add_to_mosaic(_self, message_id.tracery(), quantity, damn, voter, get_providers(commun_code, message_id.author, gems_per_period, weight));
}

void publication::reblog(symbol_code commun_code, name rebloger, mssgid_t message_id, std::string header, std::string body) {
    require_auth(rebloger);

    eosio::check(rebloger != message_id.author, "You cannot reblog your own content.");
    eosio::check(header.length() < config::max_length, "Title length is more than 256.");
    eosio::check(
        !header.length() || (header.length() && body.length()),
        "Body must be set if title is set."
    );

    check_mssg_exists(commun_code, message_id);
}

void publication::erasereblog(symbol_code commun_code, name rebloger, mssgid_t message_id) {
    require_auth(rebloger);
    eosio::check(rebloger != message_id.author, "You cannot erase reblog your own content.");
    check_mssg_exists(commun_code, message_id);
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

void publication::check_mssg_exists(symbol_code commun_code, const mssgid_t& message_id) {
    vertices vertices_table(_self, commun_code.raw());
    eosio::check(vertices_table.find(message_id.tracery()) != vertices_table.end(), "Message does not exist.");
}

accparams::const_iterator publication::get_acc_param(accparams& accparams_table, symbol_code commun_code, name account) {
    auto ret = accparams_table.find(account.value);
    if (ret == accparams_table.end()) {
        ret = accparams_table.emplace(account, [&](auto& p) { p = { .account = account };});
    }
    return ret;
}

uint16_t publication::get_gems_per_period(symbol_code commun_code, int64_t mosaic_active_period) {
    static const int64_t seconds_per_day = 24 * 60 * 60;
    auto community = commun_list::get_community(config::list_name, commun_code);
    if (mosaic_active_period == 0)
        mosaic_active_period = community.active_period;
    uint16_t gems_per_day = community.gems_per_day;
    return std::max<int64_t>(safe_prop(gems_per_day, mosaic_active_period, seconds_per_day), 1);
}

int64_t publication::get_amount_to_freeze(int64_t balance, int64_t frozen, uint16_t gems_per_period, std::optional<uint16_t> weight) {
    int64_t available = balance - frozen;
    if (available <= 0) {
        return 0;
    }
    int64_t weighted_points = weight.has_value() ? safe_pct(*weight, balance) : balance / gems_per_period;
    return (weighted_points || weight.has_value()) ? std::min(available, weighted_points) : available;
}

gallery_types::providers_t publication::get_providers(symbol_code commun_code, name account, 
                                                      uint16_t gems_per_period, std::optional<uint16_t> weight) {
    auto community = commun_list::get_community(config::list_name, commun_code);
    accparams accparams_table(_self, commun_code.raw());
    auto acc_param = get_acc_param(accparams_table, commun_code, account);
    gallery_types::provs provs_table(_self, commun_code.raw());
    gallery_types::providers_t ret;
    auto provs_index = provs_table.get_index<"bykey"_n>();
    for (size_t n = 0; n < acc_param->providers.size(); n++) {
        auto prov_name = acc_param->providers[n];
        auto prov_itr = provs_index.find(std::make_tuple(prov_name, account));
        if (prov_itr != provs_index.end() && point::balance_exists(config::point_name, prov_name, commun_code)) {
            auto actual_limit = std::max<int64_t>(
                0, point::get_balance(config::point_name, prov_name, commun_code).amount - get_frozen_amount(_self, prov_name, commun_code));
            auto amount = std::min(get_amount_to_freeze(prov_itr->total, prov_itr->frozen, gems_per_period, weight), actual_limit);
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
    commun_list::check_community_exists(config::list_name, commun_code);

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

void publication::provide(name grantor, name recipient, asset quantity, std::optional<uint16_t> fee) {
    provide_points(_self, grantor, recipient, quantity, fee);
}

void publication::advise(symbol_code commun_code, name leader, std::set<mssgid_t> favorites) {
    std::set<uint64_t> favorite_mosaics;
    for (const auto& m : favorites) {
        favorite_mosaics.insert(m.tracery());
    }
    advise_mosaics(_self, commun_code, leader, favorite_mosaics);
}

void publication::ban(symbol_code commun_code, mssgid_t message_id) {
    ban_mosaic(_self, commun_code, message_id.tracery());
}

} // commun

DISPATCH_WITH_TRANSFER(commun::publication, commun::config::point_name, ontransfer,
    (create)(update)(settags)(remove)(report)(lock)(unlock)(upvote)(downvote)(unvote)
    (claim)(hold)(transfer)(reblog)(erasereblog)(setproviders)(provide)(advise)(ban))
