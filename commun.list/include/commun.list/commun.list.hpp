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

    /**
        \brief The create action creates the community.

        \param commun_code a point symbol of the community
        \param community_name name of th community

        Performing the action requires the validators signature.
    */
    [[eosio::action]] void create(symbol_code commun_code, std::string community_name);

    /**
        \brief The setappparams action is used to update parameters of dApp.

        \param leaders_num number of dApp (top) leaders
        \param max_votes max number of leaders votes by the user
        \param permission multisig permission to which set threshold. If not exists, will be created. Should be set if required_threshold set
        \param required_threshold threshold which should be set to specified multisig permission. If set to 0, permission will be removed. Should be set if permission set

        All parameters are optional. To not change parameter, do not pass it.

        Performing the action requires the validators signature.
    */
    [[eosio::action]] void setappparams(optional<uint8_t> leaders_num, optional<uint8_t> max_votes, 
                                        optional<name> permission, optional<uint8_t> required_threshold);
    /**
        \brief The setsysparams action is used to update system parameters of community.

        \param commun_code a point symbol of the community
        \param permission multisig permission to which set threshold. If not exists, will be created. Should be set if required_threshold set
        \param required_threshold threshold which should be set to specified multisig permission. If set to 0, permission will be removed. Should be set if permission set
        \param collection_period mosaic collection period in seconds
        \param moderation_period mosaic moderation period in seconds
        \param lock_period mosaic lock period in seconds
        \param gems_per_day count of gems user can freeze per day
        \param rewarded_mosaic_num count of mosaics receiving reward
        \param opuses opuses which should be added, or updated if already exist
        \param remove_opuses opuses which should be removed
        \param min_lead_rating minimal leader rating of mosaic to receive reward

        All parameters are optional. To not change parameter, do not pass it.

        Performing the action requires the validators signature.
    */
    [[eosio::action]] void setsysparams(symbol_code commun_code,
        optional<name> permission, optional<uint8_t> required_threshold,
        optional<int64_t> collection_period, optional<int64_t> moderation_period, optional<int64_t> lock_period,
        optional<uint16_t> gems_per_day, optional<uint16_t> rewarded_mosaic_num,
        std::set<structures::opus_info> opuses, std::set<name> remove_opuses, optional<int64_t> min_lead_rating,
        optional<bool> damned_gem_reward_enabled, optional<bool> refill_gem_enabled);

    /**
        \brief The setparams action is used to update parameters of community.

        \param commun_code a point symbol of the community
        \param leaders_num number of community (top) leaders
        \param max_votes max number of leaders votes by the user
        \param permission multisig permission to which set threshold. (This action cannot create or delete permissions). Should be set if required_threshold set
        \param required_threshold threshold which should be set to specified multisig permission. Should be set if permission set
        \param emission_rate annual emission rate in percents. Values: 1.00%, or from 5.00% to 50.00% with step 5.00%
        \param leaders_percent percent of leaders reward from emission. Values: 1.00% to 10.00% with step 1.00%
        \param author_percent percent of author reward in post reward. Values: 25.00%, or 50.00%, or 75.00%. Remaining reward part is for curators

        All parameters are optional. To not change parameter, do not pass it.

        Performing the action requires the community leaders signature.
    */
    [[eosio::action]] void setparams(symbol_code commun_code,
        optional<uint8_t> leaders_num, optional<uint8_t> max_votes, 
        optional<name> permission, optional<uint8_t> required_threshold, 
        optional<uint16_t> emission_rate, optional<uint16_t> leaders_percent, optional<uint16_t> author_percent);

    /**
        \brief The setinfo action is used to update non-consensus info of community.

        \param commun_code a point symbol of the community
        \param description community description
        \param language community content language
        \param rules community users for its members
        \param avatar_image community avatar
        \param cover_image community profile cover

        Action do not stores any state in DB, it only checks authority, and community presence.

        Performing the action requires the community leaders signature.
    */
    [[eosio::action]] void setinfo(symbol_code commun_code,
        optional<std::string> description, optional<std::string> language, optional<std::string> rules,
        optional<std::string> avatar_image, optional<std::string> cover_image);

    /**
        \brief The follow action allows user to follow the community posts.

        \param commun_code a point symbol of the community 
        \param follower account of the user

        Action do not stores any state in DB, it only checks input parameters.

        Performing the action requires the follower account signature.
    */
    [[eosio::action]] void follow(symbol_code commun_code, name follower);

    /**
        \brief The unfollow action allows user to unfollow the community posts.

        \param commun_code a point symbol of the community 
        \param follower account of the user

        Action do not stores any state in DB, it only checks input parameters.

        Performing the action requires the unfollower account signature.
    */
    [[eosio::action]] void unfollow(symbol_code commun_code, name follower);

    /**
        \brief The follow action hides posts of specified community from the user.

        \param commun_code a point symbol of the community 
        \param follower account of the user

        Action do not stores any state in DB, it only checks input parameters.

        Performing the action requires the follower account signature.
    */
    [[eosio::action]] void hide(symbol_code commun_code, name follower);

    /**
        \brief The follow action cancels hiding posts of specified community from the user.

        \param commun_code a point symbol of the community 
        \param follower account of the user

        Action do not stores any state in DB, it only checks input parameters.

        Performing the action requires the follower account signature.
    */
    [[eosio::action]] void unhide(symbol_code commun_code, name follower);

    /**
        \brief The ban action is used by a leader to ban user in providebw service,

        \param commun_code a point symbol of the community where user banning
        \param leader account of the leader
        \param account account to ban
        \param reason reason of the ban (required)

        Action do not stores any state in DB, it only checks input parameters.

        Performing the action requires the leader account signature.
    */
    [[eosio::action]] void ban(symbol_code commun_code, name leader, name account, std::string reason);

    /**
        \brief The unban action is used by a leader to unban user in providebw service,

        \param commun_code a point symbol of the community where user unbanning
        \param leader account of the leader
        \param account account to unban
        \param reason reason of the unban (required)

        Action do not stores any state in DB, it only checks input parameters.

        Performing the action requires the leader account signature.
    */
    [[eosio::action]] void unban(symbol_code commun_code, name leader, name account, std::string reason);

    static auto& get_community(symbol_code commun_code) {
        static tables::community community_tbl(config::list_name, config::list_name.value);
        return community_tbl.get(commun_code.raw(), "community not exists");
    }

    static inline auto& get_community(symbol commun_symbol) {
        auto& community = get_community(commun_symbol.code());
        eosio::check(community.commun_symbol == commun_symbol, "symbol precision mismatch");
        return community;
    }

    static inline void check_community_exists(symbol_code commun_code) {
        get_community(commun_code);
    }

    static inline void check_community_exists(symbol commun_symbol) {
        get_community(commun_symbol);
    }

    static const structures::control_param_t& get_control_param(symbol_code commun_code) {
        if (commun_code) {
            return get_community(commun_code).control_param;
        }
        else {
            static tables::dapp dapp_tbl(config::list_name, config::list_name.value);
            auto& d = dapp_tbl.get(structures::dapp::primary_key(), "not initialized");
            return d.control_param;
        }
    }
};

}
