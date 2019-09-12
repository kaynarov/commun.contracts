#pragma once
#include <commun/config.hpp>


namespace commun { namespace config {
static const auto reward_perm_name = "rewardperm"_n;
static constexpr int64_t reward_mosaics_period = 60 * 60;
static constexpr int64_t reward_leaders_period = 24 * 60 * 60;

static constexpr int64_t reward_period(bool for_leaders) { 
    return for_leaders ? config::reward_leaders_period : config::reward_mosaics_period;
}
}} // commun::config
