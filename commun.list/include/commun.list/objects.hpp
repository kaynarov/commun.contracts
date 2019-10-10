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
    bool update(optional<uint8_t> leaders_num_, optional<uint8_t> max_votes_, 
                optional<name> permission_, optional<uint8_t> required_threshold_) {
        if (leaders_num_.has_value()) {
            leaders_num = *leaders_num_;
            check(leaders_num, "leaders num must be positive");
        }
        if (max_votes_.has_value()) {
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
                thr->required = req;
            }
            else if (!req && found) {
                custom_thresholds.erase(thr);
            }
            else {
                check(req, "required threshold must be positive");
                custom_thresholds.push_back({perm, req});
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
        return leaders_num_.has_value() || max_votes_.has_value() || permission_.has_value();
    };
};

struct dapp {
    control_param_t control_param = control_param_t{ .leaders_num = config::def_dapp_leaders_num, .max_votes = config::def_dapp_max_votes };
};

struct community {
    symbol commun_symbol;
    std::string community_name;

    control_param_t control_param = control_param_t{ .leaders_num = config::def_comm_leaders_num, .max_votes = config::def_comm_max_votes };

    // emit
    uint16_t emission_rate = config::def_emission_rate;
    uint16_t leaders_percent = config::def_leaders_percent;

    // publish
    uint16_t author_percent = config::def_author_percent;
    int64_t collection_period = config::def_collection_period;
    int64_t moderation_period = config::def_moderation_period;
    int64_t active_period = config::def_active_period;
    int64_t lock_period = 0; // TODO
    uint16_t gems_per_day = 10;
    uint16_t rewarded_mosaic_num = config::def_rewarded_mosaic_num;

    std::set<opus_info> opuses = config::def_opuses;

    uint64_t primary_key() const {
        return commun_symbol.code().raw();
    }
};

}

namespace commun::tables {
    using namespace eosio;

    using comn_name_index = eosio::indexed_by<"byname"_n, eosio::member<structures::community, std::string, &structures::community::community_name>>;
    using community = eosio::multi_index<"community"_n, structures::community, comn_name_index>;
    
    using dapp = eosio::singleton<"dapp"_n, structures::dapp>;
}
