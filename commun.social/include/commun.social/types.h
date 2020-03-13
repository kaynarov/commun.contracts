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
    \brief The \ref accountmeta structure contains account profile data.
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
    optional<std::string> first_name; /**< User's First Name. */
    optional<std::string> last_name; /**< User's Last Name. */
    optional<std::string> country; /**< User's Country. */
    optional<std::string> city; /**< User's City. */
    optional<std::string> birth_date; /**< User's Birth date. */
    optional<std::string> instagram; /**< User's Instagram account. */
    optional<std::string> linkedin; /**< User's LinkedIN account. */
    optional<std::string> twitter; /**< User's Twitter account. */
    optional<std::string> github; /**< User's Github account. */
    optional<std::string> website_url; /**< User's WebSiteURL. */

#ifndef UNIT_TEST_ENV
    EOSLIB_SERIALIZE(accountmeta,
        (avatar_url)(cover_url)(biography)(facebook)(telegram)(whatsapp)(wechat)
        (first_name)(last_name)(country)(city)(birth_date)(instagram)(linkedin)(twitter)(github)(website_url))
#endif
};

#ifdef UNIT_TEST_ENV
}} // eosio::testing
FC_REFLECT(eosio::testing::accountmeta,
    (avatar_url)(cover_url)(biography)(facebook)(telegram)(whatsapp)(wechat)
    (first_name)(last_name)(country)(city)(birth_date)(instagram)(linkedin)(twitter)(github)(website_url))
#endif
