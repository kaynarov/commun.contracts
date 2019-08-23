#pragma once
#include <common/config.hpp>

namespace golos { namespace config {

constexpr unsigned deletevotes_expiration_sec = 3*60*60;

constexpr size_t max_deletions_per_trx   = 100;

const uint16_t max_length = 256;

}} // golos::config
