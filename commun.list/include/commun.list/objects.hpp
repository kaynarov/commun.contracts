#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <commun/config.hpp>

namespace commun::structures {

using namespace eosio;

struct community {
    symbol_code commun_code;
    std::string community_name;

    uint16_t leaders_num = 21;

    uint16_t emission_rate = 0; // TODO
    uint16_t leaders_percent = 0; // TODO

    uint16_t author_percent = 50 * config::_1percent;
    int64_t collection_period = 7 * 24 * 60 * 60;
    int64_t moderation_period = 10 * 24 * 60 * 60;
    int64_t lock_period = 0; // TODO
    uint16_t gems_per_day = 0; // TODO
    uint16_t rewarded_mosaic_num = 20;
    int64_t post_pledge_token = 0; // TODO
    int64_t comment_pledge_token = 0; // TODO
    int64_t min_gem_pledge_token = 0; // TODO

    uint64_t primary_key() const {
        return commun_code.raw();
    }
};

}

namespace commun::tables {
    using namespace eosio;

    using comn_name_index = eosio::indexed_by<"byname"_n, eosio::member<structures::community, std::string, &structures::community::community_name>>;
    using community = eosio::multi_index<"community"_n, structures::community, comn_name_index>;
}
