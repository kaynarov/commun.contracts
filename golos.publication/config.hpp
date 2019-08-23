#pragma once
#include <commun/config.hpp>

namespace golos { namespace config {

	using namespace commun::config;

constexpr unsigned deletevotes_expiration_sec = 3*60*60;

constexpr size_t max_deletions_per_trx   = 100;

const uint16_t max_length = 256;

}} // golos::config
