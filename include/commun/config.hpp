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
using eosio::chain::name;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-string-literal-operator-template"
template <typename T, T... Str>
inline name operator ""_n() {
    return name({Str...});
}
#pragma clang diagnostic pop
#else
using eosio::name;
#endif

#define CYBER_TOKEN "cyber.token"
#define COMMUN_POINT "c.point"

namespace commun { namespace config {

// contracts
static const auto dapp_name = N(c); // can't compile "c"_n
static const auto list_name = "c.list"_n;
static const auto emit_name = "c.emit"_n;
static const auto control_name = "c.ctrl"_n;
static const auto gallery_name = "c.gallery"_n;
static const auto social_name = "c.social"_n;
static const auto recover_name = "c.recover"_n;
static const auto publish_name = gallery_name;

static const auto internal_name = "cyber"_n;
static const auto token_name = "cyber.token"_n;
static const auto null_name = "cyber.null"_n;
static const auto point_name = "c.point"_n;

// permissions
static const auto code_name = "cyber.code"_n;
static const auto owner_name = "owner"_n;
static const auto active_name = "active"_n;

static const auto super_majority_name = "lead.smajor"_n;
static const auto majority_name = "lead.major"_n;
static const auto minority_name = "lead.minor"_n;
static const auto recovery_name = "lead.recover"_n;

static const auto client_permission_name = "clientperm"_n;

// numbers and time
static constexpr auto _1percent = 100;
static constexpr auto _100percent = 100 * _1percent;
static constexpr auto block_interval_ms = 3000;//1000 / 2;
static constexpr int64_t seconds_per_minute = 60;
static constexpr int64_t seconds_per_hour = seconds_per_minute * 60;
static constexpr int64_t seconds_per_day = seconds_per_hour * 24;
static constexpr int64_t blocks_per_year = int64_t(365)*seconds_per_day*1000/block_interval_ms;

#ifndef UNIT_TEST_ENV
    static constexpr auto reserve_token = eosio::symbol("CMN", 4);
#else
    static const auto reserve_token = eosio::chain::symbol(4, "CMN");
#endif

} // config

constexpr int64_t time_to_blocks(int64_t time) {
    return time / (1000 * config::block_interval_ms);
}
constexpr int64_t seconds_to_blocks(int64_t sec) {
    return time_to_blocks(sec * 1e6);
}

} // commun::config
