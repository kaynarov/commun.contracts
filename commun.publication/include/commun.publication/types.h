#pragma once

#include <commun/config.hpp>

using enum_t = uint8_t;
using base_t = int64_t;// !if you change it -- don't forget about the abi-file
using wide_t = int128_t;
constexpr int fixed_point_fractional_digits = 12;

#ifdef UNIT_TEST_ENV
#   define ABI_TABLE
#   include <eosio/chain/types.hpp>
namespace eosio { namespace testing {
    using namespace eosio::chain;
#else
#   define ABI_TABLE [[eosio::table]]
#   include <eosio/eosio.hpp>
    using namespace eosio;
#endif

#ifdef UNIT_TEST_ENV
}} // eosio::testing
#endif
