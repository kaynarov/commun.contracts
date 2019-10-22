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

/**
    \brief Action parameter type contains a kit of fields of account profile.
    \ingroup social_class
*/
struct accountmeta {
    accountmeta() = default;

    optional<std::string> avatar_url; /**< User's avatar image. */
    optional<std::string> cover_url; /**< User's profile cover image. */
    optional<std::string> biography; /**< Biography of the user. */
    optional<std::string> facebook; /**< User's Facebook account. */
    optional<std::string> telegram; /**< User's Telegram account. */
    optional<std::string> whatsapp; /**< User's WhatsApp account. */
    optional<std::string> wechat; /**< User's WeChat account. */

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
