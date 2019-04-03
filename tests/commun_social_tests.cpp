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

public:
    commun_social_tester()
        : golos_tester(cfg::commun_social_name)
        , social({this, cfg::commun_social_name})
    {
        create_account(cfg::commun_social_name);
        produce_block();
        install_contract(cfg::commun_social_name, contracts::commun_social_wasm(), contracts::commun_social_abi());
    }
};

BOOST_AUTO_TEST_SUITE(commun_social_tests)

BOOST_FIXTURE_TEST_CASE(update_meta_test, commun_social_tester) try {
    BOOST_TEST_MESSAGE("update meta test");

    BOOST_TEST_MESSAGE("--- checking that metadata was updated");
    BOOST_CHECK_EQUAL(success(), social.update_meta("avatar", "cover_url", 
                                                    "biography", "facebook", 
                                                    "telegram", "whatsapp", 
                                                    "wechat"));
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
