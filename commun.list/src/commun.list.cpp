#include <commun.list.hpp>
#include <commun.point/commun.point.hpp>
#include <commun/config.hpp>

using namespace commun;

void commun_list::create(symbol_code commun_code, std::string community_name) {

    check(commun::point::exist(config::point_name, commun_code), "not found token");

    require_auth(_self);

    tables::community community_tbl(_self, _self.value);

    check(community_tbl.find(commun_code.raw()) == community_tbl.end(), "community token exists");

    auto community_index = community_tbl.get_index<"byname"_n>();
    check(community_index.find(community_name) == community_index.end(), "community exists");

    community_tbl.emplace(_self, [&](auto& item) {
        item.commun_code = commun_code;
        item.community_name = community_name;
    });
}

#define SET_PARAM(PARAM) if (PARAM) { item.PARAM = *PARAM; _empty = false; }

void commun_list::setsysparams(symbol_code commun_code,
        optional<int64_t> collection_period, optional<int64_t> moderation_period, optional<int64_t> lock_period,
        optional<uint16_t> gems_per_day, optional<uint16_t> rewarded_mosaic_num,
        optional<int64_t> post_pledge_token, optional<int64_t> comment_pledge_token, optional<int64_t> min_gem_pledge_token) {
    require_auth(point::get_issuer(config::point_name, commun_code));

    // <> Place for checks

    tables::community community_tbl(_self, _self.value);
    auto community = community_tbl.get(commun_code.raw(), "community token doesn't exist");

    community_tbl.modify(community, eosio::same_payer, [&](auto& item) {
        bool _empty = true;
        SET_PARAM(collection_period);
        SET_PARAM(moderation_period);
        SET_PARAM(lock_period);
        SET_PARAM(gems_per_day);
        SET_PARAM(rewarded_mosaic_num);
        SET_PARAM(post_pledge_token);
        SET_PARAM(comment_pledge_token);
        SET_PARAM(min_gem_pledge_token);
        eosio::check(!_empty, "No params changed");
    });
}

#undef SET_PARAM

void commun_list::addinfo(symbol_code commun_code, std::string community_name, 
                          std::string ticker, std::string avatar, std::string cover_img_link, 
                          std::string description, std::string rules) {
    require_auth(commun::point::get_issuer(config::point_name, commun_code));
    
    tables::community community_tbl(_self, _self.value);
    auto community_index = community_tbl.get_index<"byname"_n>();
    auto community_itr = community_index.find(community_name);
    check(community_itr != community_index.end(), "community doesn't exist");
}

EOSIO_DISPATCH(commun::commun_list, (create)(addinfo))
