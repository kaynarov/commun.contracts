#pragma once
#include <commun/config.hpp>

namespace commun { namespace config {

static constexpr double shares_cw = 0.3333;

static constexpr uint8_t max_grages_num = 30;

static constexpr int64_t default_comm_points_grade_sum = 5000;
static constexpr std::array<int64_t, 20>  default_comm_grades = 
    {{1000, 1000, 1000, 500, 500, 300, 300, 300, 300, 300, 100, 100, 100, 100, 100, 50, 50, 50, 50, 50}};

static constexpr std::array<int64_t, 4>  default_lead_grades = 
    {{1000, 500, 300, 200}};

static constexpr uint8_t default_ban_threshold = 2;

static constexpr uint16_t max_providers_num = 7;

static constexpr int64_t forced_chopping_delay = 30 * 24 * 60 * 60;

static constexpr std::array<int64_t, 10>  advice_weight = 
    {{10000, 7071, 5774, 5000, 4472, 4082, 3780, 3536, 3333, 3162}}; //sqrt(100000000/k)

static constexpr uint8_t auto_claim_num = 3;
static constexpr uint8_t auto_archives_num = 3;

#ifndef DEFAULT_OPUSES
    static constexpr int64_t default_mosaic_pledge = 100;
    static constexpr int64_t default_min_mosaic_inclusion = 1000;
    static constexpr int64_t default_min_gem_inclusion = 200;
#endif

#ifndef UNIT_TEST_ENV
    #ifndef DEFAULT_OPUSES
        static constexpr eosio::name default_opus_name("regulatum");
        #define DEFAULT_OPUSES std::array<opus_info, 1> default_opuses = {{ \
            opus_info {                                                     \
                .name = default_opus_name,                                  \
                .mosaic_pledge = default_mosaic_pledge,                     \
                .min_mosaic_inclusion = default_min_mosaic_inclusion,       \
                .min_gem_inclusion = default_min_gem_inclusion              \
            }                                                               \
        }}
    #endif
    static constexpr DEFAULT_OPUSES;

    static const eosio::time_point eternity(eosio::days(365 * 8000));
#endif

}} // commun::config
