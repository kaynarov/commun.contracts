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
