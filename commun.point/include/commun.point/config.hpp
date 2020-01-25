#pragma once
#include <commun/config.hpp>


namespace commun { namespace config {
const std::string restock_prefix = "restock: ";
const std::string minimum_prefix = "minimum: ";

static constexpr uint16_t def_transfer_fee = _1percent/10;
static constexpr int64_t def_min_transfer_fee_points = 1;
static constexpr uint32_t safe_max_delay = 30 * seconds_per_day;    // max delay and max lock period

static const auto create_permission = "createperm"_n;
static const auto issue_permission = "issueperm"_n;
static const auto transfer_permission = "transferperm"_n;
}} // commun::config
