#include <commun.point_test_api.hpp>
#include <commun_list_test_api.hpp>
#include "contracts.hpp"


namespace cfg = commun::config;
using namespace eosio::testing;
using namespace eosio::chain;
using namespace fc;
static const auto token_code_str = "GLS";
static const auto _token = symbol(3, token_code_str);
static const auto _token_e = symbol(3, "SLG");

class commun_list_tester : public golos_tester {
protected:
    commun_point_api point;
    commun_list_api community;

public:
    commun_list_tester()
        : golos_tester(cfg::commun_list_name)
        , point({this, cfg::commun_point_name, _token})
        , community({this, cfg::commun_list_name})
    {
        create_accounts({_commun, _golos, _alice, _bob, _carol, _nicolas,
            cfg::control_name, cfg::commun_point_name, cfg::commun_list_name, cfg::invoice_name});
        produce_block();
        install_contract(cfg::commun_point_name, contracts::point_wasm(), contracts::point_abi());
        install_contract(cfg::commun_list_name, contracts::commun_list_wasm(), contracts::commun_list_abi());
    }

    const account_name _commun = N(commun);
    const account_name _golos = N(golos);
    const account_name _alice = N(alice);
    const account_name _bob = N(bob);
    const account_name _carol = N(carol);
    const account_name _nicolas = N(nicolas);

    void create_token(symbol symbol_token) {
        BOOST_CHECK_EQUAL(success(), point.create(cfg::commun_point_name, asset(1000000, symbol_token), 10000, 1));
        produce_blocks(1);
    }
    
    auto get_cur_time() {
        return control->head_block_time().time_since_epoch();
    }

    struct errors: contract_error_messages {
        const string community_exists = amsg("community exists");
        const string community_symbol_code_exists = amsg("community token exists");
        const string not_found_token = amsg("not found token");
        const string no_community = amsg("community doesn't exist");
    } err;
};

BOOST_AUTO_TEST_SUITE(community_list_tests)

BOOST_FIXTURE_TEST_CASE(create_community, commun_list_tester) try {
    create_token(_token);

    BOOST_CHECK_EQUAL(err.not_found_token, community.create_record(cfg::commun_list_name, _token_e.to_symbol_code(), "community 1"));

    BOOST_CHECK_EQUAL(success(), community.create_record(cfg::commun_list_name, _token.to_symbol_code(), "community 1"));

    produce_blocks(10);

    create_token(_token_e);

    BOOST_CHECK_EQUAL(err.community_symbol_code_exists, community.create_record(cfg::commun_list_name, _token.to_symbol_code(), "community 1"));
    BOOST_CHECK_EQUAL(err.community_exists, community.create_record(cfg::commun_list_name, _token_e.to_symbol_code(), "community 1"));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(add_info_test, commun_list_tester) try {
    BOOST_TEST_MESSAGE("add info test");

    create_token(_token);

    BOOST_TEST_MESSAGE("--- checking community for existence");
    BOOST_CHECK_EQUAL(err.no_community, community.add_info(cfg::commun_point_name, _token.to_symbol_code(), "community_name"));

    BOOST_TEST_MESSAGE("--- checking that info was added successfully");
    BOOST_CHECK_EQUAL(success(), community.create_record(cfg::commun_list_name, _token.to_symbol_code(), "community_name"));
    BOOST_CHECK_EQUAL(success(), community.add_info(cfg::commun_point_name, _token.to_symbol_code(), "community_name"));
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
