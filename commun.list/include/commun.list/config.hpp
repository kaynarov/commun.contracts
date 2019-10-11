#pragma once
#include <commun/config.hpp>

namespace commun { namespace structures {

struct opus_info {
#ifndef UNIT_TEST_ENV
    eosio::name name;
#else
    eosio::chain::name name;
#endif

    int64_t mosaic_pledge = 0;
    int64_t min_mosaic_inclusion = 0; 
    int64_t min_gem_inclusion = 0;

    // for set
    friend bool operator<(const opus_info& a, const opus_info& b) {
        return a.name < b.name;
    }
    friend bool operator>(const opus_info& a, const opus_info& b) {
        return a.name.value > b.name.value;
    }
};
}} // commun::structures

namespace commun { namespace config {

static constexpr std::array<int64_t, 10>  advice_weight = 
    {{10000, 7071, 5774, 5000, 4472, 4082, 3780, 3536, 3333, 3162}}; //sqrt(100000000/k)

static constexpr int64_t def_collection_period =  7 * 24 * 60 * 60;
static constexpr int64_t def_moderation_period = 10 * 24 * 60 * 60;
static constexpr int64_t def_active_period     = 14 * 24 * 60 * 60;

static constexpr uint16_t def_author_percent = 50 * _1percent;

static constexpr uint16_t def_gems_per_day = 10;
static constexpr uint8_t def_rewarded_mosaic_num = 20;
static constexpr int64_t def_min_lead_rating = advice_weight[0] + 1;

static constexpr uint16_t def_emission_rate = _1percent * 20;
static constexpr uint16_t def_leaders_percent = _1percent * 10;

#ifndef UNIT_TEST_ENV
static constexpr auto post_opus_name = eosio::name("post");
static constexpr auto comment_opus_name = eosio::name("comment");
#else
static auto post_opus_name = eosio::chain::name("post");
static auto comment_opus_name = eosio::chain::name("comment");
#endif
static std::set<structures::opus_info> def_opuses = {{ \
    structures::opus_info{ .name = post_opus_name },   \
    structures::opus_info{ .name = comment_opus_name } \
}};

static constexpr uint8_t def_comm_leaders_num = 3;
static constexpr uint8_t def_comm_max_votes = 5;

static constexpr uint8_t def_dapp_leaders_num = 21;
static constexpr uint8_t def_dapp_max_votes = 30;

}} // commun::config

#ifdef UNIT_TEST_ENV
#include <fc/reflect/reflect.hpp>
FC_REFLECT(commun::structures::opus_info,
    (name)(mosaic_pledge)(min_mosaic_inclusion)(min_gem_inclusion))
#endif
