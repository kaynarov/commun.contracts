#pragma once
#include <commun/config.hpp>

namespace commun { namespace config {

const uint16_t max_length = 256;

const uint16_t max_comment_depth = 127;

#ifndef UNIT_TEST_ENV
    static constexpr eosio::name post_opus_name = eosio::name("post");
    static constexpr eosio::name comment_opus_name = eosio::name("comment");
    #define DEFAULT_OPUSES std::array<opus_info, 2> default_opuses = {{ \
        opus_info{ .name = post_opus_name },                            \
        opus_info{ .name = comment_opus_name }                          \
    }}
#endif

}} // commun::config
