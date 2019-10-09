#include <commun.list.hpp>
#include <commun.ctrl/commun.ctrl.hpp>
#include <commun.point/commun.point.hpp>
#include <commun/config.hpp>

using namespace commun;

void commun_list::create(symbol_code commun_code, std::string community_name) {
    require_auth(_self);
    auto commun_symbol = point::get_supply(config::point_name, commun_code).symbol;

    tables::community community_tbl(_self, _self.value);

    check(community_tbl.find(commun_code.raw()) == community_tbl.end(), "community token exists");

    auto community_index = community_tbl.get_index<"byname"_n>();
    check(community_index.find(community_name) == community_index.end(), "community exists");

    community_tbl.emplace(_self, [&](auto& item) {
        item.commun_symbol = commun_symbol;
        item.community_name = community_name;
    });
}

#define SET_PARAM(PARAM) if (PARAM) { c.PARAM = *PARAM; _empty = false; }

void commun_list::setsysparams(symbol_code commun_code,
        optional<int64_t> collection_period, optional<int64_t> moderation_period, optional<int64_t> lock_period,
        optional<uint16_t> gems_per_day, optional<uint16_t> rewarded_mosaic_num,
        std::set<structures::opus_info> opuses, std::set<name> remove_opuses) {

    require_auth(_self);

    // <> Place for checks

    tables::community community_tbl(_self, _self.value);
    auto community = community_tbl.get(commun_code.raw(), "community not exists");

    community_tbl.modify(community, eosio::same_payer, [&](auto& c) {
        bool _empty = true;
        SET_PARAM(collection_period);
        SET_PARAM(moderation_period);
        SET_PARAM(lock_period);
        SET_PARAM(gems_per_day);
        SET_PARAM(rewarded_mosaic_num);
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
        eosio::check(!_empty, "No params changed");
    });
}

void commun_list::setparams(symbol_code commun_code,
        optional<uint16_t> leaders_num, optional<uint16_t> emission_rate,
        optional<uint16_t> leaders_percent, optional<uint16_t> author_percent) {
    require_auth(point::get_issuer(config::point_name, commun_code));

    // <> Place for checks

    tables::community community_tbl(_self, _self.value);
    auto community = community_tbl.get(commun_code.raw(), "community not exists");

    community_tbl.modify(community, eosio::same_payer, [&](auto& c) {
        bool _empty = true;
        SET_PARAM(leaders_num);
        SET_PARAM(emission_rate);
        SET_PARAM(leaders_percent);
        SET_PARAM(author_percent);
        eosio::check(!_empty, "No params changed");
    });
}

#undef SET_PARAM

void commun_list::setinfo(symbol_code commun_code, std::string description,
        std::string language, std::string rules, std::string avatar_image, std::string cover_image) {
    require_auth(point::get_issuer(config::point_name, commun_code));
    get_community(_self, commun_code);
}

void commun_list::follow(symbol_code commun_code, name follower) {
    require_auth(follower);
    get_community(_self, commun_code);
}

void commun_list::unfollow(symbol_code commun_code, name follower) {
    require_auth(follower);
    get_community(_self, commun_code);
}

void commun_list::ban(symbol_code commun_code, name leader, name account, std::string reason) {
    require_auth(leader);
    eosio::check(control::in_the_top(config::control_name, commun_code, leader), (leader.to_string() + " is not a leader").c_str());
    eosio::check(is_account(account), "Account not exists.");
    eosio::check(!reason.empty(), "Reason cannot be empty.");
    check_community_exists(_self, commun_code);
}

void commun_list::unban(symbol_code commun_code, name leader, name account, std::string reason) {
    require_auth(leader);
    eosio::check(control::in_the_top(config::control_name, commun_code, leader), (leader.to_string() + " is not a leader").c_str());
    eosio::check(is_account(account), "Account not exists.");
    eosio::check(!reason.empty(), "Reason cannot be empty.");
    check_community_exists(_self, commun_code);
}

EOSIO_DISPATCH(commun::commun_list, (create)(setsysparams)(setparams)(setinfo)(follow)(unfollow)(ban)(unban))
