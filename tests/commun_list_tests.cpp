#include "golos_tester.hpp"
#include "bancor.token_test_api.hpp"
#include "commun_list_test_api.hpp"
#include "contracts.hpp"
#include "../common/config.hpp"


namespace cfg = commun::config;
using namespace eosio::testing;
using namespace eosio::chain;
using namespace fc;
static const auto token_code_str = "GLS";
static const auto _token = symbol(3, token_code_str);

class commun_list_tester : public golos_tester {
protected:
    bancor_token_api token;
    commun_list_api community;

public:
    commun_list_tester()
        : golos_tester(cfg::commun_list_name)
        , token({this, cfg::bancor_name, _token})
        , community({this, cfg::commun_list_name})
    {
        create_accounts({_commun, _golos, _alice, _bob, _carol, _nicolas,
            cfg::registrar_name, cfg::token_name, cfg::bancor_name, cfg::commun_list_name});
        produce_block();
        install_contract(cfg::bancor_name, contracts::bancor_wasm(), contracts::bancor_abi());
        install_contract(cfg::commun_list_name, contracts::commun_list_wasm(), contracts::commun_list_abi());
    }

    const account_name _commun = N(commun);
    const account_name _golos = N(golos);
    const account_name _alice = N(alice);
    const account_name _bob = N(bob);
    const account_name _carol = N(carol);
    const account_name _nicolas = N(nicolas);

    void create_token() {
        BOOST_CHECK_EQUAL(success(), token.create(cfg::bancor_name, asset(1000000, _token), 10000, 1));
        auto stats = token.get_stats();
        BOOST_TEST_MESSAGE(fc::json::to_string(stats));
        produce_blocks(1);
    }
    
    auto get_cur_time() {
        return control->head_block_time().time_since_epoch();
    }

    struct errors: contract_error_messages {
        const string community_exists = amsg("community exists");
        const string community_not_exists = amsg("community not exists");
    } err;
};

BOOST_AUTO_TEST_SUITE(community_list_tests)

BOOST_FIXTURE_TEST_CASE(create_community, commun_list_tester) try {
    create_token();
    BOOST_CHECK_EQUAL(success(), community.create_record(_token, cfg::commun_token_name));

    produce_blocks(10);

    BOOST_CHECK_EQUAL(err.community_exists, community.create_record(_token, cfg::commun_token_name));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(update_community, commun_list_tester) try {
    create_token();

    BOOST_CHECK_EQUAL(err.community_not_exists, community.update_record(_token, cfg::commun_token_name, _bob, _carol));

    BOOST_CHECK_EQUAL(success(), community.create_record(_token, cfg::commun_token_name));
    BOOST_CHECK_EQUAL(success(), community.update_record(_token, cfg::commun_token_name, _bob, _carol));
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
