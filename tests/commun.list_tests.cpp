#include <commun.point_test_api.hpp>
#include <commun.list_test_api.hpp>
#include "contracts.hpp"


namespace cfg = commun::config;
using namespace eosio::testing;
using namespace eosio::chain;
using namespace fc;
static const auto token_code_str = "GLS";
static const auto _token = symbol(3, token_code_str);
static const auto _token_e = symbol(3, "SLG");
static const auto _token_code = _token.to_symbol_code();
static const auto _token_e_code = _token_e.to_symbol_code();

class commun_list_tester : public golos_tester {
protected:
    commun_point_api point;
    commun_list_api community;

public:
    commun_list_tester()
        : golos_tester(cfg::list_name)
        , point({this, cfg::point_name, _token})
        , community({this, cfg::list_name})
    {
        create_accounts({_commun, _golos, _alice, _bob, _carol, _nicolas,
            cfg::control_name, cfg::point_name, cfg::list_name, cfg::emit_name});
        produce_block();
        install_contract(cfg::point_name, contracts::point_wasm(), contracts::point_abi());
        install_contract(cfg::list_name, contracts::list_wasm(), contracts::list_abi());
        
        set_authority(cfg::emit_name, N(init), create_code_authority({cfg::list_name}), "active");
        link_authority(cfg::emit_name, cfg::emit_name, N(init), N(init));
        
        set_authority(cfg::control_name, N(init), create_code_authority({cfg::list_name}), "active");
        link_authority(cfg::control_name, cfg::control_name, N(init), N(init));
    }

    const account_name _commun = cfg::dapp_name;
    const account_name _golos = N(golos);
    const account_name _alice = N(alice);
    const account_name _bob = N(bob);
    const account_name _carol = N(carol);
    const account_name _nicolas = N(nicolas);

    void create_token(account_name issuer, symbol symbol_token) {
        BOOST_CHECK_EQUAL(success(), point.create(issuer, asset(1000000, symbol_token), 10000, 1));
        produce_blocks(1);
    }

    struct errors: contract_error_messages {
        const string community_exists = amsg("community exists");
        const string community_symbol_code_exists = amsg("community token exists");
        const string no_point_symbol = amsg("point with symbol does not exist");
        const string no_community = amsg("community not exists");
        const string no_changes = amsg("No params changed");
        const string no_opus(name opus) { return amsg("no opus " + opus.to_string()); }
        const string incorrect_author_percent = amsg("incorrect author percent");
        const string add_perm = amsg("cannot add permission");
        const string del_perm = amsg("cannot delete permission");
    } err;
};

BOOST_AUTO_TEST_SUITE(commun_list_tests)

BOOST_FIXTURE_TEST_CASE(create_community, commun_list_tester) try {
    BOOST_TEST_MESSAGE("create_community");
    create_token(_golos, _token);

    BOOST_CHECK_EQUAL(err.no_point_symbol, community.create(cfg::list_name, _token_e_code, "community 1"));

    BOOST_CHECK_EQUAL(success(), community.create(cfg::list_name, _token_code, "community 1"));

    produce_blocks(10);

    create_token(_nicolas, _token_e);

    BOOST_CHECK_EQUAL(err.community_symbol_code_exists, community.create(cfg::list_name, _token_code, "community 1"));
    BOOST_CHECK_EQUAL(err.community_exists, community.create(cfg::list_name, _token_e_code, "community 1"));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(setparams_test, commun_list_tester) try {
    BOOST_TEST_MESSAGE("setparams test");
    create_token(_golos, _token);
    BOOST_CHECK_EQUAL(err.no_community, community.setparams( _golos, _token_code, community.args() ));
    BOOST_CHECK_EQUAL(success(), community.create(cfg::list_name, _token_code, "community_name"));
    BOOST_CHECK_EQUAL(err.no_changes, community.setparams( _golos, _token_code, community.args() ));
    BOOST_CHECK_EQUAL(success(), community.setparams( _golos, _token_code, community.args()
        ("leaders_num", 20) ));
    BOOST_CHECK_EQUAL(success(), community.setparams( _golos, _token_code, community.args()
        ("leaders_num", 20)("emission_rate", 5000) ));
    
    BOOST_CHECK_EQUAL(err.add_perm, community.setparams(_golos, _token_code, community.args()
        ("permission", N(newperm))("required_threshold", 19)));
    BOOST_CHECK_EQUAL(success(), community.setsysparams(_token_code, community.sysparams()
        ("permission", N(newperm))("required_threshold", 19)));
    BOOST_CHECK_EQUAL(err.no_changes, community.setparams(_golos, _token_code, community.args()
        ("permission", N(newperm))("required_threshold", 19)));
    BOOST_CHECK_EQUAL(success(), community.setparams(_golos, _token_code, community.args()
        ("permission", N(newperm))("required_threshold", 20)));
    BOOST_CHECK_EQUAL(err.del_perm, community.setparams(_golos, _token_code, community.args()
        ("permission", N(newperm))("required_threshold", 0)));
    BOOST_CHECK_EQUAL(success(), community.setsysparams(_token_code, community.sysparams()
        ("permission", N(newperm))("required_threshold", 0)));

    BOOST_CHECK_EQUAL(err.incorrect_author_percent, community.setparams( _golos, _token_code, community.args()
        ("author_percent", 24*cfg::_1percent) ));
    BOOST_CHECK_EQUAL(success(), community.setparams( _golos, _token_code, community.args()
        ("author_percent", 25*cfg::_1percent) ));
    BOOST_CHECK_EQUAL(success(), community.setparams( _golos, _token_code, community.args()
        ("author_percent", 50*cfg::_1percent) ));
    BOOST_CHECK_EQUAL(success(), community.setparams( _golos, _token_code, community.args()
        ("author_percent", 75*cfg::_1percent) ));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(setinfo_test, commun_list_tester) try {
    BOOST_TEST_MESSAGE("setinfo test");
    create_token(_golos, _token);

    BOOST_TEST_MESSAGE("--- checking community for existence");
    BOOST_CHECK_EQUAL(err.no_community, community.setinfo(_golos, _token_code, community.info()
        ("description", "description")));

    BOOST_TEST_MESSAGE("--- checking that info was added successfully");
    BOOST_CHECK_EQUAL(success(), community.create(cfg::list_name, _token_code, "community_name"));
    BOOST_CHECK_EQUAL(err.no_changes, community.setinfo(_golos, _token_code, community.info()));
    BOOST_CHECK_EQUAL(success(), community.setinfo(_golos, _token_code, community.info()
        ("description", "description")
        ("language", "language")
        ("rules", "rules")
        ("avatar_image", "avatar_image")
        ("cover_image", "cover_image")));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(setsysparams_test, commun_list_tester) try {
    BOOST_TEST_MESSAGE("setsysparams test");
    create_token(_golos, _token);
    BOOST_CHECK_EQUAL(err.no_community, community.setsysparams( _token_code, community.sysparams() ));
    BOOST_CHECK_EQUAL(success(), community.create(cfg::list_name, _token_code, "community_name"));
    BOOST_CHECK_EQUAL(err.no_changes, community.setsysparams( _token_code, community.sysparams() ));

    BOOST_CHECK_EQUAL(success(), community.setsysparams( _token_code, community.sysparams()
        ("collection_period", 3600) ));
    BOOST_CHECK_EQUAL(success(), community.setsysparams( _token_code, community.sysparams()
        ("collection_period", 3600)("moderation_period", 3600) ));

    BOOST_CHECK_EQUAL(err.no_opus("notexist"), community.setsysparams( _token_code, community.sysparams()
        ("remove_opuses", std::set<name>{"notexist"} )));
    BOOST_CHECK_EQUAL(success(), community.setsysparams( _token_code, community.sysparams()
        ("opuses", std::set<opus_info>{{"opus1"}, {"opus2"}} )));
    produce_block();
    BOOST_CHECK_EQUAL(success(), community.setsysparams( _token_code, community.sysparams()
        ("remove_opuses", std::set<name>{"opus1", "opus2"} )));
    produce_block();
    BOOST_CHECK_EQUAL(err.no_opus("opus1"), community.setsysparams( _token_code, community.sysparams()
        ("remove_opuses", std::set<name>{"opus1", "opus2"} )));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(follow_hide_test, commun_list_tester) try {
    BOOST_TEST_MESSAGE("follow and hide test");
    create_token(_golos, _token);
    BOOST_CHECK_EQUAL(err.no_community, community.follow(_token_code, _alice));
    BOOST_CHECK_EQUAL(err.no_community, community.unfollow(_token_code, _alice));
    BOOST_CHECK_EQUAL(err.no_community, community.hide(_token_code, _alice));
    BOOST_CHECK_EQUAL(err.no_community, community.unhide(_token_code, _alice));
    BOOST_CHECK_EQUAL(success(), community.create(cfg::list_name, _token_code, "community_name"));
    BOOST_CHECK_EQUAL(success(), community.follow(_token_code, _alice));
    BOOST_CHECK_EQUAL(success(), community.unfollow(_token_code, _alice));
    BOOST_CHECK_EQUAL(success(), community.hide(_token_code, _alice));
    BOOST_CHECK_EQUAL(success(), community.unhide(_token_code, _alice));
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
