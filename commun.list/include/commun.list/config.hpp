#pragma once
#include <commun/config.hpp>

namespace commun { namespace config {

static constexpr int64_t def_collection_period =  7 * 24 * 60 * 60;
static constexpr int64_t def_moderation_period = 10 * 24 * 60 * 60;
static constexpr int64_t def_active_period     = 14 * 24 * 60 * 60;

static constexpr uint16_t def_author_percent = _100percent;

static constexpr uint16_t def_gems_per_day = 10;
static constexpr uint8_t def_rewarded_mosaic_num = 20;

static constexpr uint16_t def_emission_rate = _1percent * 20;
static constexpr uint16_t def_leaders_percent = _1percent * 10;

}} // commun::config
