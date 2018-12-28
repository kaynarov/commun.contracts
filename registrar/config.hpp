#pragma once
#include <common/config.hpp>

namespace commun { namespace config {
const std::string bid_prefix = "bid: ";

const uint32_t time_denom = 400; // for tests

const uint32_t checkwin_interval = 60 * 60 / time_denom;
const uint32_t min_time_from_last_win = 60 * 60 * 24 / time_denom;
const uint32_t min_time_from_last_bid_start = 60 * 60 * 24 / time_denom;
const uint32_t min_time_from_last_bid_step = 60 * 60 * 3 / time_denom;

const size_t max_bid_checks = 64;
const uint32_t bid_increment_denom = 10;

const int16_t bancor_creation_fee = 3000;
}} // commun::config
