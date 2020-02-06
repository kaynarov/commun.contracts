#pragma once
#include "objects.hpp"
#include <commun/dispatchers.hpp>

namespace commun {

using namespace eosio;
using std::optional;

/**
 * \brief This class implements the \a c.list contract functionality
 * \ingroup list_class
 */
class
/// @cond
[[eosio::contract("commun.list")]]
/// @endcond
commun_list: public contract {
    uint64_t validate_name(tables::community& community_tbl, const std::string& community_name);
public:
    using contract::contract;

    /**
        \brief The \ref create action is used to create an individual community in the commun application.

        \param commun_code a point symbol of the new community
        \param community_name a name of the new community

        \signreq
            — <i>trusted community client</i> .
    */
    [[eosio::action]] void create(symbol_code commun_code, std::string community_name);

    /**
        \brief The \ref setappparams action is used to configure the parameters of dApp.

        \param leaders_num number of dApp (top) leaders.  Default value is 21
        \param max_votes maximum number of leaders a user can vote for. Default value is 30
        \param permission permission for multisig transaction, for which the threshold is set. If this parameter is set, but not stored in DB, it will be placed in that DB. If \a required_threshold is set, the \a permission should be set too
        \param required_threshold threshold at which a multisig transaction is considered approved. If this parameter is set to «0», the \a permission will be removed. If \a permission is set, the threshold should be set too

        All parameters are optionally. So, each of them can be set via separate calling of this action.

        \signreq
            — <i>majority of Commun dApp leaders</i> .

        \note
        The parameters \a permission and \a required_threshold can also be used for adjust default permissions as follows:
        <center> <i> lead.smajor = leaders_num×2/3+1 </i> </center>
        <center> <i> lead.major = leaders_num×1/2+1 </i> </center>
        <center> <i> lead.minor = leaders_num×1/3+1 </i> </center>
        Default permissions are calculated as parts of \a leaders_num. The \a required_threshold parameter sets absolute count of leaders when adding a permission or changing its default value. The \a setappparams action allows leaders to only set, not change, the authorities.
    */
    [[eosio::action]] void setappparams(optional<uint8_t> leaders_num, optional<uint8_t> max_votes, 
                                        optional<name> permission, optional<uint8_t> required_threshold);
    /**
        \brief The \ref setsysparams action is used to set up the system parameters for a separate community.

        \param commun_code a point symbol of the community
        \param community_name new community name
        \param permission permission for multisig transaction, for which the threshold is set. If this parameter is set, but not stored in DB, it will be placed in that DB. If \a required_threshold is set, the \a permission should be set too
        \param required_threshold threshold at which a multisig transaction is considered approved. If this parameter is set to «0», the \a permission will be removed. If \a permission is set, the threshold should be set too
        \param collection_period mosaic collection period (in seconds). Default value is 604800 (calculated as 7×24×60×60)
        \param moderation_period mosaic moderation period (in seconds). Default value is 259200 (calculated as 3×24×60×60)
        \param extra_reward_period mosaic extra reward period (in seconds)
        \param gems_per_day number of gems that a user can freeze per day. Default value is 10
        \param rewarded_mosaic_num number of mosaics receiving a reward. Default value is 10
        \param opuses opuses which should be added or updated if they exist
        \param remove_opuses opuses which should be removed
        \param min_lead_rating minimum mosaic leader rating required to receive a reward. Default value is 10001 (calculated as advice_weight[0]+1 = 10000+1)
        \param damned_gem_reward_enabled \a true — it allows a gem owner to get reward from a gem with negative shares. Default value is \a false
        \param refill_gem_enabled \a true — it allows to refill the gems with points and shares. Default value is \a false
        \param custom_gem_size_enabled \a true — it allows to adjust the custom size of the gem when voting, posting or leaving a comment. Default value is \a false

        All parameters except \a commun_code are optionally. So, each of them can be set via separate calling of this action.

        \signreq
            — <i>trusted community client</i> .

        \note
        The parameters \a permission and \a required_threshold can be used by leaders for adjust default permissions as follows:
        <center> <i> lead.smajor = leaders_num×2/3+1 </i> </center>
        <center> <i> lead.major = leaders_num×1/2+1 </i> </center>
        <center> <i> lead.minor = leaders_num×1/3+1 </i> </center>
        Default permissions are calculated as parts of \a leaders_num. The \a required_threshold parameter sets absolute count of leaders when adding a permission or changing its default value. Unlike \a setappparams, the \a setsysparams action allows leaders to add or remove the authorities.
    */
    [[eosio::action]] void setsysparams(symbol_code commun_code, optional<std::string> community_name,
        optional<name> permission, optional<uint8_t> required_threshold,
        optional<int64_t> collection_period, optional<int64_t> moderation_period, optional<int64_t> extra_reward_period,
        optional<uint16_t> gems_per_day, optional<uint16_t> rewarded_mosaic_num,
        std::set<structures::opus_info> opuses, std::set<name> remove_opuses, optional<int64_t> min_lead_rating,
        optional<bool> damned_gem_reward_enabled, optional<bool> refill_gem_enabled, optional<bool> custom_gem_size_enabled);

    /**
        \brief The \ref setparams action is used to set up the parameters for a community.

        \param commun_code a point symbol of the community
        \param leaders_num number of community (top) leaders.  Default value is 3
        \param max_votes maximum number of leaders a user can vote for. Default value is 5
        \param permission permission for multisig transaction, for which the threshold is set. This action can not create or delete \a permissions. If \a required_threshold is set, this parameter should be set too
        \param required_threshold threshold at which a multisig transaction is considered approved. If \a permission is set, this parameter should be set too
        \param emission_rate annual emission rate (in percent). This parameter takes values from 1,00 to 50,00 inclusive in increments of 5,00 (i.e. 1,00; 5,00; 10,00; ...; 50,00). Default value is 20,00
        \param leaders_percent share (in percent) of the annual emission deducted as a reward to community leaders. This parameter takes values from 1,00 to 10,00 inclusive in increments of 1,00. Default value is 3,00
        \param author_percent share (in percent) deducted from a post reward to author of the post. This parameter can take one of the values: 25,00; 50,00 and 75,00. Remaining share of the post reward is allocated to curators. Default value is 50,00

        All parameters except \a commun_code are optionally. So, each of them can be set via separate calling of this action.

        Depending on the \a required_threshold set, a number of signatures required to complete a multisig transaction may be vary. For example, setting threshold=3 is considered to be obtained if the signature is affixed with at least three leaders.

        \signreq
            — <i>majority of community leaders</i> .
    */
    [[eosio::action]] void setparams(symbol_code commun_code,
        optional<uint8_t> leaders_num, optional<uint8_t> max_votes, 
        optional<name> permission, optional<uint8_t> required_threshold, 
        optional<uint16_t> emission_rate, optional<uint16_t> leaders_percent, optional<uint16_t> author_percent);

    /**
        \brief The \ref setinfo action is used to set up a non-consensus information for a community.

        \param commun_code a point symbol of the community
        \param description community description. The text length is not limited
        \param language community content language
        \param rules a set of rules for community members. The text length is not limited 
        \param avatar_image community avatar
        \param cover_image community profile cover
        \param theme community theme

        All parameters except \a commun_code are optionally. So, each of them can be set via separate calling of this action.
        This action does not store any state in DB, it only checks an authority and community presence.

        \signreq
            — <i>majority of community leaders</i> .
    */
    [[eosio::action]] void setinfo(symbol_code commun_code,
        optional<std::string> description, optional<std::string> language, optional<std::string> rules,
        optional<std::string> avatar_image, optional<std::string> cover_image, optional<std::string> theme);

    /**
        \brief The \ref follow action allows a user to follow (to track) the posts of specified community.

        \param commun_code a point symbol of the community 
        \param follower account of the user

        This action does not store any state in DB, it only checks input parameters.

        \signreq
            — the \a follower account ;
            — <i>trusted community client</i> .
    */
    [[eosio::action]] void follow(symbol_code commun_code, name follower);

    /**
        \brief The \ref unfollow action allows a user to opt out of tracking the posts of specified community.

        \param commun_code a point symbol of the community 
        \param follower account of the user

        This action does not store any state in DB, it only checks input parameters.

        \signreq
            — the \a follower account ;
            — <i>trusted community client</i> .
    */
    [[eosio::action]] void unfollow(symbol_code commun_code, name follower);

    /**
        \brief The \ref hide action is used to hide posts of specified community from a user.

        \param commun_code a point symbol of the community 
        \param follower account of the user

        This action does not store any state in DB, it only checks input parameters.

        \signreq
            — the \a follower account ;
            — <i>trusted community client</i> .
    */
    [[eosio::action]] void hide(symbol_code commun_code, name follower);

    /**
        \brief The \ref unhide action cancels hiding posts of specified community from a user.

        \param commun_code a point symbol of the community 
        \param follower account of the user

        This action does not store any state in DB, it only checks input parameters.

        \signreq
            — the \a follower account ;
            — <i>trusted community client</i> .
    */
    [[eosio::action]] void unhide(symbol_code commun_code, name follower);

    /**
        \brief The \ref ban action is used by a leader to ban (to block) a user in providebw service. To pay for the resources (bandwidth) used by an account, one more action \a providebw can be added to the transaction. Such service requires that the transaction should be signed by two persons — by those who performs an action and who pays for used bandwidth. So, a leader can ban a user who uses this service for personal gain.

        \param commun_code a point symbol of the community in which the user is banned
        \param account user account is to be banned
        \param reason text explaining the reason for banning (it is required parameter)

        This action does not store any state in DB, it only checks input parameters.

        \signreq
            — <i>minority of community leaders</i> .
    */
    [[eosio::action]] void ban(symbol_code commun_code, name account, std::string reason);

    /**
        \brief The \ref unban action is used by a leader to unban (to allow) a user in providebw service.

        \param commun_code a point symbol of the community in which the user is unbanned
        \param account user account is to be unbanned
        \param reason text explaining the reason for unbanning (it is required parameter)

        This action does not store any state in DB, it only checks input parameters.

        \signreq
            — <i>minority of community leaders</i> .
    */
    [[eosio::action]] void unban(symbol_code commun_code, name account, std::string reason);

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
