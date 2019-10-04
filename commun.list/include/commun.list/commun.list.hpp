#include "objects.hpp"

namespace commun {

using namespace eosio;
using std::optional;

class commun_list: public contract {
public:
    using contract::contract;

    [[eosio::action]] void create(symbol_code commun_code, std::string community_name);

    [[eosio::action]] void setsysparams(symbol_code commun_code,
        optional<int64_t> collection_period, optional<int64_t> moderation_period, optional<int64_t> lock_period,
        optional<uint16_t> gems_per_day, optional<uint16_t> rewarded_mosaic_num,
        optional<int64_t> post_pledge_token, optional<int64_t> comment_pledge_token, optional<int64_t> min_gem_pledge_token);

    [[eosio::action]] void setparams(symbol_code commun_code,
        optional<uint16_t> leaders_num, optional<uint16_t> emission_rate,
        optional<uint16_t> leaders_percent, optional<uint16_t> author_percent);

    [[eosio::action]] void setinfo(symbol_code commun_code, std::string description,
        std::string language, std::string rules, std::string avatar_image, std::string cover_image);

    [[eosio::action]] void follow(symbol_code commun_code, name follower);
    [[eosio::action]] void unfollow(symbol_code commun_code, name follower);

    static inline auto get_community(name list_contract_account, symbol_code commun_code) {
        tables::community community_tbl(list_contract_account, list_contract_account.value);
        return community_tbl.get(commun_code.raw(), "community not exists");
    }
};

}
