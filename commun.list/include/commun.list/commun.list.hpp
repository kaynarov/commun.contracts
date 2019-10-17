#pragma once
#include "objects.hpp"

namespace commun {

using namespace eosio;
using std::optional;

/**
 * \brief This class implements comn.list contract behaviour
 * \ingroup list_class
 */
class commun_list: public contract {
public:
    using contract::contract;

    [[eosio::action]] void create(symbol_code commun_code, std::string community_name);
    
    [[eosio::action]] void setappparams(optional<uint8_t> leaders_num, optional<uint8_t> max_votes, 
                                        optional<name> permission, optional<uint8_t> required_threshold);

    [[eosio::action]] void setsysparams(symbol_code commun_code,
        optional<int64_t> collection_period, optional<int64_t> moderation_period, optional<int64_t> lock_period,
        optional<uint16_t> gems_per_day, optional<uint16_t> rewarded_mosaic_num,
        std::set<structures::opus_info> opuses, std::set<name> remove_opuses, optional<int64_t> min_lead_rating);

    [[eosio::action]] void setparams(symbol_code commun_code,
        optional<uint8_t> leaders_num, optional<uint8_t> max_votes, 
        optional<name> permission, optional<uint8_t> required_threshold, 
        optional<uint16_t> emission_rate, optional<uint16_t> leaders_percent, optional<uint16_t> author_percent);

    [[eosio::action]] void setinfo(symbol_code commun_code, std::string description,
        std::string language, std::string rules, std::string avatar_image, std::string cover_image);

    [[eosio::action]] void follow(symbol_code commun_code, name follower);
    [[eosio::action]] void unfollow(symbol_code commun_code, name follower);

    /**
     * \brief action is used by a leader to ban user in providebw service
     *
     * \param commun_code symbol of community POINT.
     * \param leader account of the leader
     * \param account account to ban
     * \param reason reason of the ban
     */
    [[eosio::action]] void ban(symbol_code commun_code, name leader, name account, std::string reason);
    [[eosio::action]] void unban(symbol_code commun_code, name leader, name account, std::string reason);

    static inline auto get_community(name list_contract_account, symbol_code commun_code) {
        tables::community community_tbl(list_contract_account, list_contract_account.value);
        return community_tbl.get(commun_code.raw(), "community not exists");
    }

    static inline void check_community_exists(name list_contract_account, symbol_code commun_code) {
        get_community(list_contract_account, commun_code);
    }

    static inline structures::control_param_t get_control_param(name list_contract_account, symbol_code commun_code) {
        if (commun_code) {
            tables::community community_tbl(list_contract_account, list_contract_account.value);
            return community_tbl.get(commun_code.raw(), "community not exists").control_param;
        }
        else {
            tables::dapp dapp_tbl(list_contract_account, list_contract_account.value);
            eosio::check(dapp_tbl.exists(), "not initialized");
            return dapp_tbl.get().control_param;
        }
    }
};

}
