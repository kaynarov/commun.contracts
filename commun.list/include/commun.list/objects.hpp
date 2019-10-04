#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include "config.hpp"

namespace commun::structures {

using namespace eosio;

struct community {
    symbol commun_symbol;
    std::string community_name;

    // ctrl
    uint16_t leaders_num = 21;

    // emit
    uint16_t emission_rate = 0; // TODO
    uint16_t leaders_percent = 0; // TODO

    // publish
    uint16_t author_percent = config::def_author_percent;
    int64_t collection_period = config::def_collection_period;
    int64_t moderation_period = config::def_moderation_period;
    int64_t active_period = config::def_active_period;
    int64_t lock_period = 0; // TODO
    uint16_t gems_per_day = 10;
    uint16_t rewarded_mosaic_num = config::def_rewarded_mosaic_num;

    int64_t post_pledge_token = 0; // TODO
    int64_t comment_pledge_token = 0; // TODO
    int64_t min_gem_pledge_token = 0; // TODO

    uint64_t primary_key() const {
        return commun_symbol.code().raw();
    }
};

}

namespace commun::tables {
    using namespace eosio;

    using comn_name_index = eosio::indexed_by<"byname"_n, eosio::member<structures::community, std::string, &structures::community::community_name>>;
    using community = eosio::multi_index<"community"_n, structures::community, comn_name_index>;
}
