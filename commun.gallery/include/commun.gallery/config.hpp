#pragma once

namespace commun { namespace config {

static constexpr double shares_cw = 0.3333;

static constexpr uint8_t max_grages_num = 30;

static constexpr int64_t default_comm_points_grade_sum = 5000;
static constexpr std::array<int64_t, 20>  default_comm_grades = 
    {{1000, 1000, 1000, 500, 500, 300, 300, 300, 300, 300, 100, 100, 100, 100, 100, 50, 50, 50, 50, 50}};

static constexpr std::array<int64_t, 4>  default_lead_grades = 
    {{1000, 500, 300, 200}};

static constexpr uint16_t max_providers_num = 7;

static constexpr int64_t forced_chopping_delay = 30 * 24 * 60 * 60;

static constexpr uint8_t auto_claim_num = 3;
static constexpr uint8_t auto_deactivate_num = 3;

#ifndef UNIT_TEST_ENV
    static const eosio::time_point eternity(eosio::days(365 * 8000));
#endif

}} // commun::config
