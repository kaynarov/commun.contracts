#include "golos_tester.hpp"
#include "commun_social_test_api.hpp"
#include "contracts.hpp"
#include <commun/config.hpp>


namespace cfg = commun::config;
using namespace eosio::testing;
using namespace eosio::chain;
using namespace fc;

class commun_social_tester : public golos_tester {
protected:
    commun_social_api social;

    std::vector<account_name> _users;
public:
    commun_social_tester()
        : golos_tester(cfg::commun_social_name)
        , social({this, cfg::commun_social_name})
        , _users{"dave"_n, "erin"_n} {
        create_accounts(_users);
        create_account(cfg::commun_social_name);
        produce_block();
        install_contract(cfg::commun_social_name, contracts::commun_social_wasm(), contracts::commun_social_abi());
    }

protected:

    struct errors: contract_error_messages {
        const string cannot_pin_yourself        = amsg("You cannot pin yourself");
        const string cannot_unpin_yourself      = amsg("You cannot unpin yourself");
        const string cannot_block_yourself      = amsg("You cannot block yourself");
        const string cannot_unblock_yourself    = amsg("You cannot unblock yourself");
    } err;
};

BOOST_AUTO_TEST_SUITE(commun_social_tests)

BOOST_FIXTURE_TEST_CASE(pin_unpin_test, commun_social_tester) try {
    BOOST_TEST_MESSAGE("pin unpin test");

    BOOST_TEST_MESSAGE("--- pin: dave dave");
    BOOST_CHECK_EQUAL(err.cannot_pin_yourself, social.pin("dave"_n, "dave"_n));
    produce_block();

    BOOST_TEST_MESSAGE("--- unpin: dave dave");
    BOOST_CHECK_EQUAL(err.cannot_unpin_yourself, social.unpin("dave"_n, "dave"_n));
    produce_block();

    BOOST_TEST_MESSAGE("--- pin: dave erin");
    BOOST_CHECK_EQUAL(success(), social.pin("dave"_n, "erin"_n));
    produce_block();

    BOOST_TEST_MESSAGE("--- unpin: dave erin");
    BOOST_CHECK_EQUAL(success(), social.unpin("dave"_n, "erin"_n));
    produce_block();
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(block_unblock_test, commun_social_tester) try {
    BOOST_TEST_MESSAGE("block unblock test");

    BOOST_TEST_MESSAGE("--- block: dave dave");
    BOOST_CHECK_EQUAL(err.cannot_block_yourself, social.block("dave"_n, "dave"_n));
    produce_block();

    BOOST_TEST_MESSAGE("--- unblock: dave dave");
    BOOST_CHECK_EQUAL(err.cannot_unblock_yourself, social.unblock("dave"_n, "dave"_n));
    produce_block();

    BOOST_TEST_MESSAGE("--- block: dave erin");
    BOOST_CHECK_EQUAL(success(), social.block("dave"_n, "erin"_n));
    produce_block();

    BOOST_TEST_MESSAGE("--- unblock: dave erin");
    BOOST_CHECK_EQUAL(success(), social.unblock("dave"_n, "erin"_n));
    produce_block();
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(update_meta_test, commun_social_tester) try {
    BOOST_TEST_MESSAGE("update meta test");

    BOOST_TEST_MESSAGE("--- checking that metadata was updated");
    BOOST_CHECK_EQUAL(success(), social.update_meta("dave"_n,
        "https://cdn.foobar.test/avatar.png", "http://bestcovers/cover1.jpg",
        "My biography is very short",
        "facebook", "telegram", "whatsapp", "wechat"));
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END() // commun_social_tests
