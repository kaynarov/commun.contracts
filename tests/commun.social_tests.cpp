#include "golos_tester.hpp"
#include "commun.social_test_api.hpp"
#include "contracts.hpp"
#include <commun/config.hpp>


namespace cfg = commun::config;
using namespace eosio::testing;
using namespace eosio::chain;
using namespace fc;
static const account_name _client = N(communcom);

class commun_social_tester : public golos_tester {
protected:
    commun_social_api social;

    std::vector<account_name> _users;
public:
    commun_social_tester()
        : golos_tester(cfg::social_name)
        , social({this, cfg::social_name})
        , _users{"dave"_n, "erin"_n, _client} {
        create_accounts(_users);
        create_account(cfg::social_name);
        produce_block();
        install_contract(cfg::social_name, contracts::social_wasm(), contracts::social_abi());

        set_authority(cfg::social_name, cfg::client_permission_name,
            authority (1, {}, {{.permission = {_client, cfg::active_name}, .weight = 1}}), "owner");
        link_authority(cfg::social_name, cfg::social_name, cfg::client_permission_name, N(pin));
        link_authority(cfg::social_name, cfg::social_name, cfg::client_permission_name, N(unpin));
        link_authority(cfg::social_name, cfg::social_name, cfg::client_permission_name, N(block));
        link_authority(cfg::social_name, cfg::social_name, cfg::client_permission_name, N(unblock));
        link_authority(cfg::social_name, cfg::social_name, cfg::client_permission_name, N(updatemeta));
        link_authority(cfg::social_name, cfg::social_name, cfg::client_permission_name, N(deletemeta));
    }

protected:

    struct errors: contract_error_messages {
        const string cannot_pin_yourself        = amsg("You cannot pin yourself");
        const string cannot_unpin_yourself      = amsg("You cannot unpin yourself");
        const string cannot_block_yourself      = amsg("You cannot block yourself");
        const string cannot_unblock_yourself    = amsg("You cannot unblock yourself");
        const string no_pinning                 = amsg("Pinning account doesn't exist.");
        const string no_blocking                = amsg("Blocking account doesn't exist.");
    } err;
};

BOOST_AUTO_TEST_SUITE(commun_social_tests)

BOOST_FIXTURE_TEST_CASE(pin_unpin_test, commun_social_tester) try {
    BOOST_TEST_MESSAGE("pin unpin test");

    BOOST_CHECK_EQUAL(err.missing_auth("dave"_n), social.push(N(pin), "erin"_n, social.args()
        ("pinner", "dave"_n)("pinning", "erin"_n)));
    BOOST_CHECK_EQUAL(err.missing_auth(_code), social.pin("dave"_n, "erin"_n, account_name()));
    BOOST_CHECK_EQUAL(err.no_pinning, social.pin("dave"_n, "notexist"_n, _client));
    BOOST_CHECK_EQUAL(err.cannot_pin_yourself, social.pin("dave"_n, "dave"_n, _client));
    BOOST_CHECK_EQUAL(success(), social.pin("dave"_n, "erin"_n, _client));
    produce_block();

    BOOST_CHECK_EQUAL(err.missing_auth("dave"_n), social.push(N(unpin), "erin"_n, social.args()
        ("pinner", "dave"_n)("pinning", "erin"_n)));
    BOOST_CHECK_EQUAL(err.missing_auth(_code), social.unpin("dave"_n, "erin"_n, account_name()));
    BOOST_CHECK_EQUAL(err.no_pinning, social.unpin("dave"_n, "notexist"_n, _client));
    BOOST_CHECK_EQUAL(err.cannot_unpin_yourself, social.unpin("dave"_n, "dave"_n, _client));
    BOOST_CHECK_EQUAL(success(), social.unpin("dave"_n, "erin"_n, _client));
    produce_block();
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(block_unblock_test, commun_social_tester) try {
    BOOST_TEST_MESSAGE("block unblock test");

    BOOST_CHECK_EQUAL(err.missing_auth("dave"_n), social.push(N(block), "erin"_n, social.args()
        ("blocker", "dave"_n)("blocking", "erin"_n)));
    BOOST_CHECK_EQUAL(err.missing_auth(_code), social.block("dave"_n, "erin"_n, account_name()));
    BOOST_CHECK_EQUAL(err.no_blocking, social.block("dave"_n, "notexist"_n, _client));
    BOOST_CHECK_EQUAL(err.cannot_block_yourself, social.block("dave"_n, "dave"_n, _client));
    BOOST_CHECK_EQUAL(success(), social.block("dave"_n, "erin"_n, _client));
    produce_block();

    BOOST_CHECK_EQUAL(err.missing_auth("dave"_n), social.push(N(unblock), "erin"_n, social.args()
        ("blocker", "dave"_n)("blocking", "erin"_n)));
    BOOST_CHECK_EQUAL(err.missing_auth(_code), social.unblock("dave"_n, "erin"_n, account_name()));
    BOOST_CHECK_EQUAL(err.no_blocking, social.unblock("dave"_n, "notexist"_n, _client));
    BOOST_CHECK_EQUAL(err.cannot_unblock_yourself, social.unblock("dave"_n, "dave"_n, _client));
    BOOST_CHECK_EQUAL(success(), social.unblock("dave"_n, "erin"_n, _client));
    produce_block();
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(update_meta_test, commun_social_tester) try {
    BOOST_TEST_MESSAGE("update meta test");

    accountmeta meta;
    BOOST_CHECK_EQUAL(err.missing_auth("dave"_n), social.push(N(updatemeta), "erin"_n, social.args()
        ("account", "dave"_n)("meta", meta)));
    BOOST_CHECK_EQUAL(success(), social.updatemeta("dave"_n, meta, _client));
    BOOST_CHECK_EQUAL(err.missing_auth(_code), social.updatemeta("dave"_n, meta, account_name()));
    meta.avatar_url = "";
    BOOST_CHECK_EQUAL(success(), social.updatemeta("dave"_n, meta, _client));
    meta.avatar_url = "https://cdn.foobar.test/avatar.png";
    meta.cover_url  = "http://bestcovers/cover1.jpg";
    meta.biography  = "My biography is very short";
    meta.facebook   = "facebook";
    meta.telegram   = "telegram";
    meta.whatsapp   = "whatsapp";
    meta.wechat     = "wechat";
    meta.first_name = "John";
    meta.last_name  = "Snow";
    meta.country    = "Mordor";
    meta.city       = "Minas Ithil";
    meta.birth_date = fc::time_point_sec::from_iso_string("2001-01-01T00:00:00");
    meta.instagram  = "john.snow";
    meta.linkedin   = "snow.john";
    meta.twitter    = "Sauron";
    meta.github     = "TTT Soft";
    meta.website_url = "https://myurl.com";
    BOOST_CHECK_EQUAL(success(), social.updatemeta("dave"_n, meta, _client));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(delete_meta_test, commun_social_tester) try {
    BOOST_TEST_MESSAGE("delete meta test");

    BOOST_CHECK_EQUAL(err.missing_auth("dave"_n), social.push(N(deletemeta), "erin"_n, social.args()
        ("account", "dave"_n)));
    BOOST_CHECK_EQUAL(err.missing_auth(_code), social.deletemeta("dave"_n, account_name()));
    BOOST_CHECK_EQUAL(success(), social.deletemeta("dave"_n, _client));
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END() // commun_social_tests
