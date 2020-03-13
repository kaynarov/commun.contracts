#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include "config.hpp"

namespace commun::structures {

using namespace eosio;
using std::optional;

/**
 * \brief The structure stores a part of community or dApp configurations in DB.
 * \ingroup list_tables
 *
 * This structure represents a threshold required for specified leaders permission.
 */
struct threshold {
    name permission; //!< a name of permission for which threshold is specified
    uint8_t required; //!< a threshold required for the specified permission
};

/**
 * \brief The structure stores a part of community or dApp configurations in DB.
 * \ingroup list_tables
 *
 * This structure represents the leaders data used by commun.control contract.
 */
struct control_param_t {
    uint8_t leaders_num; //!< Number of leaders can be in top simultaneously
    uint8_t max_votes; //!< Number of leaders can be voted by one user simultaneously
    std::vector<threshold> custom_thresholds; //!< Custom leaders permissions which can be requested for multi-signature transactions in community or dApp (non-custom permissions are: minority, majority and super majority) 
    bool update(optional<name> permission_, optional<uint8_t> required_threshold_,
                optional<uint8_t> leaders_num_ = optional<uint8_t>(), optional<uint8_t> max_votes_ = optional<uint8_t>(),
                bool can_change_permissions_num = true) {
        bool ret = false;
        if (leaders_num_.has_value()) {
            ret = ret || leaders_num != *leaders_num_;
            leaders_num = *leaders_num_;
            check(leaders_num, "leaders num must be positive");
        }
        if (max_votes_.has_value()) {
            ret = ret || max_votes != *max_votes_;
            max_votes = *max_votes_;
            check(max_votes, "max votes must be positive");
        }
        
        check(permission_.has_value() == required_threshold_.has_value(), "inconsistent set of threshold parameters");
        if (permission_.has_value()) {
            auto perm = *permission_;
            auto req = *required_threshold_;
            check(perm != name(), "incorrect permission name");
            
            auto thr = std::find_if(custom_thresholds.begin(), custom_thresholds.end(), 
                [perm](const threshold& t) { return t.permission == perm; });
            bool found = thr != custom_thresholds.end();
            if (req && found) {
                ret = ret || thr->required != req;
                thr->required = req;
            }
            else if (!req && found) {
                check(can_change_permissions_num, "cannot delete permission");
                custom_thresholds.erase(thr);
                ret = true;
            }
            else {
                check(can_change_permissions_num, "cannot add permission");
                check(req, "required threshold must be positive");
                custom_thresholds.push_back({perm, req});
                ret = true;
            }
        }
        
        if (leaders_num_.has_value()) {
            for (const auto& t : custom_thresholds) {
                check(t.required <= leaders_num, "leaders num can't be less than required threshold");
            }
        }
        else if (required_threshold_.has_value()) {
            auto req = *required_threshold_;
            check(!req || req <= leaders_num, "required threshold can't be greater than leaders num");
        }
        return ret;
    };
};

/**
 * \brief The structure stores dApp configuration in DB.
 * \ingroup list_tables
 *
 * Such record is created (in case it does not exist) by \ref commun_list::create or by \ref commun_list::setappparams, and changed by \ref commun_list::setappparams. This type of recording is the only one in the application.
 *
 */
// DOCS_TABLE: dapp
struct dapp {
    uint64_t id = primary_key();
    control_param_t control_param = control_param_t{ .leaders_num = config::def_dapp_leaders_num, .max_votes = config::def_dapp_max_votes }; //!< Parameters of leadership

    static uint64_t primary_key() {
        return 0;
    }
};

/**
 * \brief The structure represents the emission table for a community in DB.
 * \ingroup list_tables
 *
 * This structure contains parameters of emission.
 */
struct emission_receiver {
    name     contract; //!< Emission receiver
    uint64_t period;   //!< Period of reward
    uint16_t percent;  //!< Percent of emission
};

/**
 * \brief The structure represents the community table in DB.
 * \ingroup list_tables
 *
 * This structure contains parameters configuring a community.
 *
 * Such record is created by \ref commun_list::create, and modified by \ref commun_list::setparams or by \ref commun_list::setsysparams.
 *
 */
// DOCS_TABLE: community
struct community {
    symbol commun_symbol; //!< Symbol code (primary key) of the community point tokens
    uint64_t community_hash; //!< Community name hash making the name unique

    control_param_t control_param = control_param_t{ .leaders_num = config::def_comm_leaders_num, .max_votes = config::def_comm_max_votes };  //!< Parameters of leadership

    // emit
    uint16_t emission_rate = config::def_emission_rate;  //!< Emission rate of the community point tokens
    std::vector<emission_receiver> emission_receivers;   //!< List of emission receivers

    // publish
    uint16_t author_percent = config::def_author_percent; //!< The share of award allocated to author of post
    int64_t collection_period = config::def_collection_period; //!< The period (in seconds) of mosaic collection
    int64_t moderation_period = config::def_moderation_period; //!< The period (in seconds) of mosaic moderation
    int64_t extra_reward_period = config::def_extra_reward_period; //!< The period (in seconds) of mosaic extra reward
    uint16_t gems_per_day = config::def_gems_per_day; //!< Number of gems that a user can freeze per day
    uint16_t rewarded_mosaic_num = config::def_rewarded_mosaic_num; //!< Number of mosaics receiving a reward
    int64_t min_lead_rating = config::def_min_lead_rating; //!< The value for minimum mosaic leader rating required to receive a reward

    std::set<opus_info> opuses = config::def_opuses; //!< The value for opuses with pledges
    bool damned_gem_reward_enabled = config::def_damned_gem_reward_enabled; //!< The flag to enable getting a damned gem reward
    bool refill_gem_enabled = config::def_refill_gem_enabled; //!< The flag to enable refilling a gem
    bool custom_gem_size_enabled = config::def_custom_gem_size_enabled; //!< The flag to enable adjusting a custom gem size

    uint64_t primary_key() const {
        return commun_symbol.code().raw();
    }
    
    const opus_info& get_opus(name opus, const std::string s = "unknown opus") const {
        auto opus_itr = opuses.find(opus_info{opus});
        check(opus_itr != opuses.end(), s);
        return *opus_itr;
    }

    const emission_receiver& get_emission_receiver(name to_contract) const {
        return get_emission_receiver<const emission_receiver>(emission_receivers, to_contract);
    }

    emission_receiver& get_emission_receiver(name to_contract) {
        return get_emission_receiver<emission_receiver>(emission_receivers, to_contract);
    }

private:
    template <typename Result, typename List>
    Result& get_emission_receiver(List& list, name to_contract) const {
        auto itr = std::find_if(list.begin(), list.end(), [&](auto& receiver) {
            return receiver.contract == to_contract;
        });
        eosio::check(itr != list.end(), to_contract.to_string() + " wasn't initialized for reward period");
        return *itr;
    }
};

}

namespace commun::tables {
    using namespace eosio;

    using comn_hash_index [[eosio::order("community_hash","asc")]] = eosio::indexed_by<"byhash"_n, eosio::member<structures::community, uint64_t, &structures::community::community_hash>>;
    using community [[using eosio: order("commun_symbol._sym","asc"), contract("commun.list")]] = eosio::multi_index<"community"_n, structures::community, comn_hash_index>;

    using dapp [[using eosio: order("id","asc"), contract("commun.list")]] = eosio::multi_index<"dapp"_n, structures::dapp>;
}
