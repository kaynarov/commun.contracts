#include <commun.list/commun.list.hpp>
#include <commun.ctrl/commun.ctrl.hpp>
#include <commun.point/commun.point.hpp>
#include <commun.emit/commun.emit.hpp>
#include <eosio/crypto.hpp>
#include <commun/config.hpp>

using namespace commun;

std::pair<bool, tables::dapp::const_iterator> init_dapp(const tables::dapp& dapp_tbl) {
    auto itr = dapp_tbl.find(structures::dapp::primary_key());
    if (itr == dapp_tbl.end()) {
        itr = dapp_tbl.emplace(dapp_tbl.code(), [&](auto& d) {
            d = structures::dapp{};
        });
        return std::make_pair(true, itr);
    }
    return std::make_pair(false, itr);
}

void commun_list::setappparams(optional<uint8_t> leaders_num, optional<uint8_t> max_votes,
                               optional<name> permission, optional<uint8_t> required_threshold) {
    require_auth(_self);

    tables::dapp dapp_tbl(_self, _self.value);
    auto res = init_dapp(dapp_tbl);

    dapp_tbl.modify(res.second, same_payer, [&](auto& d) {
        res.first = !d.control_param.update(permission, required_threshold, leaders_num, max_votes) && res.first;
    });
    eosio::check(!res.first, "No params changed");
}

void commun_list::create(symbol_code commun_code, std::string community_name) {
    require_auth(_self);

    init_dapp(tables::dapp(_self, _self.value));

    auto commun_symbol = point::get_supply(config::point_name, commun_code).symbol;

    tables::community community_tbl(_self, _self.value);

    check(community_tbl.find(commun_code.raw()) == community_tbl.end(), "community token exists");

    auto community_hash256 = eosio::sha256(community_name.c_str(), community_name.size());
    auto community_hash = *(reinterpret_cast<const uint64_t *>(community_hash256.extract_as_byte_array().data()));
    auto community_index = community_tbl.get_index<"byhash"_n>();
    check(community_index.find(community_hash) == community_index.end(), "community exists");

    community_tbl.emplace(_self, [&](auto& item) {
        item.commun_symbol = commun_symbol;
        item.community_hash = community_hash;
        item.emission_receivers.reserve(2);
        item.emission_receivers.push_back({
            config::control_name,
            config::def_reward_leaders_period,
            config::def_leaders_percent});
        item.emission_receivers.push_back({
            config::gallery_name,
            config::def_reward_mosaics_period,
            config::_100percent - config::def_leaders_percent});
    });
    
    auto send_init_action = [commun_code](name contract_name) {
        action(permission_level{contract_name, "init"_n}, contract_name, "init"_n, std::make_tuple(commun_code)).send();
    };
    send_init_action(config::emit_name);
    send_init_action(config::control_name);
    //TODO: init gallery
}

#define SET_PARAM(PARAM) if (PARAM) { c.PARAM = *PARAM; _empty = false; }
#define PERC(VAL) (config::_1percent * VAL)

void commun_list::setsysparams(symbol_code commun_code,
        optional<name> permission, optional<uint8_t> required_threshold, 
        optional<int64_t> collection_period, optional<int64_t> moderation_period, optional<int64_t> lock_period,
        optional<uint16_t> gems_per_day, optional<uint16_t> rewarded_mosaic_num,
        std::set<structures::opus_info> opuses, std::set<name> remove_opuses, optional<int64_t> min_lead_rating,
        optional<bool> damned_gem_reward_enabled, optional<bool> refill_gem_enabled) {

    require_auth(_self);

    // <> Place for checks

    tables::community community_tbl(_self, _self.value);
    auto& community = community_tbl.get(commun_code.raw(), "community not exists");

    community_tbl.modify(community, eosio::same_payer, [&](auto& c) {
        bool _empty = !c.control_param.update(permission, required_threshold);
        SET_PARAM(collection_period);
        SET_PARAM(moderation_period);
        SET_PARAM(lock_period);
        SET_PARAM(gems_per_day);
        SET_PARAM(rewarded_mosaic_num);
        SET_PARAM(damned_gem_reward_enabled);
        SET_PARAM(refill_gem_enabled);
        if (!opuses.empty()) {
            c.opuses = opuses;
            _empty = false;
        }
        if (!remove_opuses.empty()) {
            for (auto& opus_name : remove_opuses) {
                auto removed = c.opuses.erase(structures::opus_info{opus_name});
                eosio::check(removed, "no opus " + opus_name.to_string());
            }
            _empty = false;
        }
        SET_PARAM(min_lead_rating);
        eosio::check(!_empty, "No params changed");
    });
}

void commun_list::setparams(symbol_code commun_code,
        optional<uint8_t> leaders_num, optional<uint8_t> max_votes, 
        optional<name> permission, optional<uint8_t> required_threshold, 
        optional<uint16_t> emission_rate, optional<uint16_t> leaders_percent, optional<uint16_t> author_percent) {
    require_auth(point::get_issuer(config::point_name, commun_code));

    // <> Place for checks
    eosio::check(!emission_rate.has_value() || *emission_rate == PERC(1) ||
        (PERC(5) <= *emission_rate && *emission_rate <= PERC(50) && (*emission_rate % PERC(5) == 0)),
        "incorrect emission rate");
    eosio::check(!leaders_percent.has_value() ||
        (PERC(1) <= *leaders_percent && *leaders_percent <= PERC(10) && (*leaders_percent % PERC(1) == 0)),
        "incorrect leaders percent");
    eosio::check(!author_percent.has_value() ||
        (*author_percent == PERC(25) || *author_percent == PERC(50) || *author_percent == PERC(75)),
        "incorrect author percent");

    tables::community community_tbl(_self, _self.value); 
    auto& community = community_tbl.get(commun_code.raw(), "community not exists");

    community_tbl.modify(community, eosio::same_payer, [&](auto& c) {
        bool _empty = !c.control_param.update(permission, required_threshold, leaders_num, max_votes, false);
        SET_PARAM(emission_rate);
        SET_PARAM(author_percent);
        if (leaders_percent) for (auto& receiver: c.emission_receivers) {
            _empty = false;
            if (receiver.contract == config::control_name) {
                receiver.percent = *leaders_percent;
            } else if (receiver.contract == config::gallery_name) {
                receiver.percent = config::_100percent - *leaders_percent;
            } else {
                eosio::check(false, "unknown contract in emission receivers");
            }
        }
        eosio::check(!_empty, "No params changed");
    });
}

#undef SET_PARAM
#undef PERC

void commun_list::setinfo(symbol_code commun_code,
    optional<std::string> description, optional<std::string> language, optional<std::string> rules,
    optional<std::string> avatar_image, optional<std::string> cover_image
) {
    eosio::check(
        description || language || rules || avatar_image || cover_image,
        "No params changed");

    require_auth(point::get_issuer(config::point_name, commun_code));
    check_community_exists(commun_code);
}

void commun_list::follow(symbol_code commun_code, name follower) {
    require_auth(follower);
    check_community_exists(commun_code);
}

void commun_list::unfollow(symbol_code commun_code, name follower) {
    require_auth(follower);
    check_community_exists(commun_code);
}

void commun_list::hide(symbol_code commun_code, name follower) {
    require_auth(follower);
    check_community_exists(commun_code);
}

void commun_list::unhide(symbol_code commun_code, name follower) {
    require_auth(follower);
    check_community_exists(commun_code);
}

void commun_list::ban(symbol_code commun_code, name leader, name account, std::string reason) {
    require_auth(leader);
    eosio::check(control::in_the_top(commun_code, leader), (leader.to_string() + " is not a leader").c_str());
    eosio::check(is_account(account), "Account not exists.");
    eosio::check(!reason.empty(), "Reason cannot be empty.");
    check_community_exists(commun_code);
}

void commun_list::unban(symbol_code commun_code, name leader, name account, std::string reason) {
    require_auth(leader);
    eosio::check(control::in_the_top(commun_code, leader), (leader.to_string() + " is not a leader").c_str());
    eosio::check(is_account(account), "Account not exists.");
    eosio::check(!reason.empty(), "Reason cannot be empty.");
    check_community_exists(commun_code);
}

EOSIO_DISPATCH(commun::commun_list, (create)(setappparams)(setsysparams)(setparams)(setinfo)(follow)(unfollow)(hide)(unhide)(ban)(unban))
