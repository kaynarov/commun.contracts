#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include "config.hpp"

namespace commun::structures {

using namespace eosio;
using std::optional;

struct threshold {
    name permission;
    uint8_t required;
};

struct control_param_t {
    uint8_t leaders_num;
    uint8_t max_votes;
    std::vector<threshold> custom_thresholds;
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

struct dapp {
    uint64_t pk = primary_key();
    control_param_t control_param = control_param_t{ .leaders_num = config::def_dapp_leaders_num, .max_votes = config::def_dapp_max_votes };

    static uint64_t primary_key() {
        return 0;
    }
};

/**
 * \brief struct is part of community configuration in the db
 * \ingroup list_tables
 *
 * Struct represents an emission receiver in the community
 */
struct emission_receiver {
    name     contract; //!< Emission receiver
    uint64_t period;   //!< Period of reward
    uint16_t percent;  //!< Percent of emission
};

/**
 * \brief struct represents a community table in the db
 * \ingroup list_tables
 *
 * Contains information about configuration of the community
 */
struct community {
    symbol commun_symbol; //!< symbol code of the community POINT
    std::string community_name; //!< readable name of the community

    control_param_t control_param = control_param_t{ .leaders_num = config::def_comm_leaders_num, .max_votes = config::def_comm_max_votes };

    // emit
    uint16_t emission_rate = config::def_emission_rate;  //!< Emission rate of community POINTs
    std::vector<emission_receiver> emission_receivers;   //!< List of emission receivers

    // publish
    uint16_t author_percent = config::def_author_percent; //!< percent of author reward in post reward
    int64_t collection_period = config::def_collection_period; //!< mosaic collection period in seconds
    int64_t moderation_period = config::def_moderation_period; //!< mosaic moderation period in seconds
    int64_t extra_reward_period = config::def_extra_reward_period;
    int64_t lock_period = 0; // TODO
    uint16_t gems_per_day = config::def_gems_per_day; //!< count of gems user can freeze per day
    uint16_t rewarded_mosaic_num = config::def_rewarded_mosaic_num; //!< count of mosaics receiving reward
    int64_t min_lead_rating = config::def_min_lead_rating; //!< minimal leader rating of mosaic to receive reward

    std::set<opus_info> opuses = config::def_opuses; //!< opuses with pledges

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
        auto itr = std::find_if(list.begin(), list.end(), [&](auto& reciever) {
            return reciever.contract == to_contract;
        });
        eosio::check(itr != list.end(), to_contract.to_string() + " wasn't initialized for reward period");
        return *itr;
    }
};

}

namespace commun::tables {
    using namespace eosio;

    using comn_name_index = eosio::indexed_by<"byname"_n, eosio::member<structures::community, std::string, &structures::community::community_name>>;
    using community = eosio::multi_index<"community"_n, structures::community, comn_name_index>;
    
    using dapp = eosio::multi_index<"dapp"_n, structures::dapp>;
}
