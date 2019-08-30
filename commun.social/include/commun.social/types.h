#pragma once

#ifdef UNIT_TEST_ENV
#   include <eosio/chain/types.hpp>
namespace eosio { namespace testing {
    using namespace eosio::chain;
#   include <fc/optional.hpp>
    using fc::optional;
#else
#   include <eosio/eosio.hpp>
    using namespace eosio;
    using std::optional;
#endif

struct accountmeta {
    accountmeta() = default;

    optional<std::string> avatar_url;
    optional<std::string> cover_url;
    optional<std::string> biography;
    optional<std::string> facebook;
    optional<std::string> telegram;
    optional<std::string> whatsapp;
    optional<std::string> wechat;

#ifndef UNIT_TEST_ENV
    EOSLIB_SERIALIZE(accountmeta,
        (avatar_url)(cover_url)(biography)(facebook)(telegram)(whatsapp)(wechat))
#endif
};

#ifdef UNIT_TEST_ENV
}} // eosio::testing
FC_REFLECT(eosio::testing::accountmeta,
    (avatar_url)(cover_url)(biography)(facebook)(telegram)(whatsapp)(wechat))
#endif
