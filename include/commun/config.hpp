#pragma once

// TODO: Fully migrate to _n and remove
#ifndef UNIT_TEST_ENV
// test env still use N macro, so we continue to use it here
#   define STRINGIFY_(x) #x
#   define STRINGIFY(x) STRINGIFY_(x)
#   define N(x) eosio::name(STRINGIFY(x))
#   include <eosio/symbol.hpp>
#endif

#ifdef UNIT_TEST_ENV
#   include <eosio/chain/types.hpp>
template <typename T, T... Str>
inline eosio::chain::name operator ""_n() {
   return eosio::chain::name({Str...});
}
#endif

namespace commun { namespace config {

// contracts
static const auto list_name = "cmmn.list"_n;
static const auto emit_name = "cmmn.emit"_n;
static const auto control_name = "cmmn.ctrl"_n;
static const auto gallery_name = "cmmn.gallery"_n;
static const auto social_name = "cmmn.social"_n;
static const auto publish_name = gallery_name;

static const auto internal_name = "cyber"_n;
static const auto token_name = "cyber.token"_n;
static const auto point_name = "cmmn.point"_n;

// permissions
static const auto code_name = "cyber.code"_n;
static const auto owner_name = "owner"_n;
static const auto active_name = "active"_n;

// numbers and time
static constexpr auto _1percent = 100;
static constexpr auto _100percent = 100 * _1percent;
static constexpr auto block_interval_ms = 3000;//1000 / 2;
static constexpr int64_t blocks_per_year = int64_t(365)*24*60*60*1000/block_interval_ms;

#ifndef UNIT_TEST_ENV
    static constexpr auto reserve_token = eosio::symbol("COMMUN", 4);
#else
    static const auto reserve_token = eosio::chain::symbol(4, "COMMUN");
#endif

} // config

constexpr int64_t time_to_blocks(int64_t time) {
    return time / (1000 * config::block_interval_ms);
}
constexpr int64_t seconds_to_blocks(int64_t sec) {
    return time_to_blocks(sec * 1e6);
}

} // commun::config
