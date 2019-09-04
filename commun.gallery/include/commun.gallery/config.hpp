#pragma once
#include <commun/config.hpp>

namespace commun { namespace config {

static constexpr double shares_cw = 0.3333;

static constexpr int64_t default_collection_period   =  7 * 24 * 60 * 60;
static constexpr int64_t default_evaluation_period   = 10 * 24 * 60 * 60;

static constexpr uint16_t default_max_royalty = _100percent;

static constexpr int64_t default_mosaic_active_period      = 14 * 24 * 60 * 60;

static constexpr uint8_t max_grages_num = 30;
static constexpr uint8_t max_rewarded_num = 25;

static constexpr uint8_t default_rewarded_num = 20;

static constexpr int64_t default_comm_points_grade_sum = 5000;
static constexpr std::array<int64_t, 20>  default_comm_grades = 
    {{1000, 1000, 1000, 500, 500, 300, 300, 300, 300, 300, 100, 100, 100, 100, 100, 50, 50, 50, 50, 50}};

static constexpr std::array<int64_t, 4>  default_lead_grades = 
    {{1000, 500, 300, 200}};

static constexpr uint8_t default_ban_threshold = 2;

static constexpr uint16_t max_providers_num = 7;

static constexpr int64_t dust_holding_period = 40 * 24 * 60 * 60;
static constexpr int64_t dust_cost_limit = 10000; 

static constexpr std::array<int64_t, 10>  advice_weight = 
    {{10000, 7071, 5774, 5000, 4472, 4082, 3780, 3536, 3333, 3162}}; //sqrt(100000000/k)

static constexpr uint8_t auto_claim_num = 2;
    
static constexpr int64_t min_gem_cost = 10;
static constexpr int64_t reward_mosaics_period = 60 * 60;
static constexpr int64_t vote_period = 24 * 60 * 60;

}} // commun::config
