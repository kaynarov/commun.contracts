#include "gallery_tester.hpp"
#include "commun.point_test_api.hpp"
#include "commun.gallery_test_api.hpp"
#include "cyber.token_test_api.hpp"
#include "contracts.hpp"
#include "../commun.point/include/commun.point/config.hpp"
#include "../commun.gallery/include/commun.gallery/config.hpp"
#include "../commun.emit/include/commun.emit/config.hpp"
using int128_t = fc::int128_t;
#include "../include/commun/util.hpp"

namespace cfg = commun::config;
using namespace eosio::testing;
using namespace eosio::chain;
using namespace fc;
static const auto point_code_str = "GLS";
static const auto _point = symbol(3, point_code_str);
using commun::config::commun_point_name;
using commun::config::commun_gallery_name;
using commun::config::commun_emit_name;
class commun_gallery_tester : public gallery_tester {
protected:
    cyber_token_api token;
    commun_point_api point;
    commun_gallery_api gallery;
    
public:
    commun_gallery_tester()
        : gallery_tester(commun_gallery_name)
        , token({this, cfg::token_name, cfg::reserve_token})
        , point({this, commun_point_name, _point})
        , gallery({this, _code, _point})
    {
        create_accounts({_commun, _golos, _alice, _bob, _carol,
            cfg::token_name, commun_point_name, commun_gallery_name, commun_emit_name});
        produce_block();
        install_contract(commun_point_name, contracts::point_wasm(), contracts::point_abi());
        install_contract(commun_emit_name, contracts::emit_wasm(), contracts::emit_abi());
        install_contract(cfg::token_name, contracts::token_wasm(), contracts::token_abi());
        install_contract(_code, contracts::gallery_wasm(), contracts::gallery_abi());

        set_authority(commun_emit_name, cfg::mscsreward_perm_name, create_code_authority({_code}), "active");
        link_authority(commun_emit_name, commun_emit_name, cfg::mscsreward_perm_name, N(mscsreward));

        std::vector<account_name> transfer_perm_accs{_code, commun_emit_name};
        std::sort(transfer_perm_accs.begin(), transfer_perm_accs.end());
        set_authority(commun_point_name, cfg::issue_permission, create_code_authority({commun_emit_name}), "active");
        set_authority(commun_point_name, cfg::transfer_permission, create_code_authority(transfer_perm_accs), "active");

        link_authority(commun_point_name, commun_point_name, cfg::issue_permission, N(issue));
        link_authority(commun_point_name, commun_point_name, cfg::transfer_permission, N(transfer));
    }

    const account_name _commun = N(commun);
    const account_name _golos = N(golos);
    const account_name _alice = N(alice);
    const account_name _bob = N(bob);
    const account_name _carol = N(carol);

    struct errors: contract_error_messages {
        const string no_balance = amsg("point balance does not exist");
        const string no_reserve = amsg("no reserve");
        const string overdrawn_balance = amsg("overdrawn balance");
        const string eval_period = amsg("evaluation period isn't over yet");
        const string insufficient_quantity(account_name account) { return amsg("insufficient quantity for " + string(account)); };
        const string no_mosaic = amsg("mosaic doesn't exist");
        const string no_points_provided = amsg("no points provided");
    } err;

    void init() {
        int64_t supply  = 5000000000000;
        int64_t reserve = 50000000000;
        uint16_t royalty = 2500;
        int64_t min_gem_points = commun::safe_prop(commun::config::min_gem_cost, supply, reserve);
        int64_t init_amount = supply / 2;

        uint16_t annual_emission_rate = 1000;
        uint16_t leaders_reward_prop = 1000;

        BOOST_CHECK_EQUAL(success(), token.create(_commun, asset(reserve, token._symbol)));
        BOOST_CHECK_EQUAL(success(), token.issue(_commun, _carol, asset(reserve, token._symbol), ""));

        BOOST_CHECK_EQUAL(success(), point.create(_golos, asset(supply * 2, point._symbol), 10000, 1));
        BOOST_CHECK_EQUAL(success(), point.set_freezer(commun::config::commun_gallery_name));

        BOOST_CHECK_EQUAL(success(), push_action(commun_emit_name, N(create), commun_emit_name, mvo()
            ("commun_symbol", point._symbol)("annual_emission_rate", annual_emission_rate)("leaders_reward_prop", leaders_reward_prop)));

        BOOST_CHECK_EQUAL(success(), token.transfer(_carol, commun_point_name, asset(reserve, token._symbol), cfg::restock_prefix + point_code_str));
        BOOST_CHECK_EQUAL(success(), point.issue(_golos, _golos, asset(supply, point._symbol), std::string(point_code_str) + " issue"));
        BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _alice, asset(init_amount, point._symbol)));
        BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _carol, asset(init_amount, point._symbol)));
        BOOST_CHECK_EQUAL(success(), point.open(_code, point._symbol, _code));
        BOOST_CHECK_EQUAL(success(), gallery.create(point._symbol));
        BOOST_CHECK_EQUAL(success(), gallery.createmosaic(_alice, 1, asset(min_gem_points, point._symbol), royalty));
    }
};

BOOST_AUTO_TEST_SUITE(gallery_tests)

BOOST_FIXTURE_TEST_CASE(basic_tests, commun_gallery_tester) try {
    BOOST_TEST_MESSAGE("Basic gallery tests");
    int64_t supply  = 5000000000000;
    int64_t reserve = 50000000000;
    uint16_t royalty = 2500;
    int64_t min_gem_points = commun::safe_prop(commun::config::min_gem_cost, supply, reserve);
    int64_t init_amount = supply / 2;

    uint16_t annual_emission_rate = 1000;
    uint16_t leaders_reward_prop = 1000;

    BOOST_CHECK_EQUAL(success(), token.create(_commun, asset(reserve, token._symbol)));
    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _carol, asset(reserve, token._symbol), ""));

    BOOST_CHECK_EQUAL(success(), point.create(_golos, asset(supply * 2, point._symbol), 10000, 1));
    BOOST_CHECK_EQUAL(success(), point.set_freezer(commun::config::commun_gallery_name));

    BOOST_CHECK_EQUAL(success(), push_action(commun_emit_name, N(create), commun_emit_name, mvo()
        ("commun_symbol", point._symbol)("annual_emission_rate", annual_emission_rate)("leaders_reward_prop", leaders_reward_prop)));

    BOOST_CHECK_EQUAL(success(), token.transfer(_carol, commun_point_name, asset(reserve, token._symbol), cfg::restock_prefix + point_code_str));
    BOOST_CHECK_EQUAL(success(), point.issue(_golos, _golos, asset(supply, point._symbol), std::string(point_code_str) + " issue"));
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _alice, asset(init_amount, point._symbol)));
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _carol, asset(init_amount, point._symbol)));
    BOOST_CHECK_EQUAL(errgallery.no_balance, gallery.create(point._symbol));
    BOOST_CHECK_EQUAL(success(), point.open(_code, point._symbol, _code));
    BOOST_CHECK_EQUAL(success(), gallery.create(point._symbol));
    BOOST_CHECK_EQUAL(errgallery.overdrawn_balance, gallery.createmosaic(_alice, 1, asset(point.get_amount(_alice) + 1, point._symbol), royalty));
    BOOST_CHECK_EQUAL(errgallery.insufficient_quantity(_alice), gallery.createmosaic(_alice, 1, asset(min_gem_points - 1, point._symbol), royalty));
    BOOST_CHECK_EQUAL(success(), gallery.createmosaic(_alice, 1, asset(min_gem_points, point._symbol), royalty));
    BOOST_CHECK_EQUAL(success(), gallery.addtomosaic(_alice, 1, asset(point.get_amount(_carol), point._symbol), false, _carol));

    produce_block();
    produce_block(fc::seconds(commun::config::default_evaluation_period - (cfg::block_interval_ms / 1000)));
    BOOST_CHECK_EQUAL(errgallery.eval_period, gallery.claimgem(_alice, 1, point._symbol.to_symbol_code(), _alice));

    produce_blocks(1);

    BOOST_CHECK_EQUAL(point.get_supply(), supply);
    BOOST_CHECK_EQUAL(success(), gallery.claimgem(_alice, 1, point._symbol.to_symbol_code(), _carol)); //carol got nothing
    auto reward = point.get_supply() - supply;
    BOOST_TEST_MESSAGE("--- reward = " << reward);
    BOOST_CHECK_EQUAL(success(), gallery.claimgem(_alice, 1, point._symbol.to_symbol_code(), _alice));
    BOOST_CHECK_EQUAL(point.get_amount(_alice), init_amount + reward);

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE(create_tests, commun_gallery_tester) try {
    BOOST_TEST_MESSAGE("create gallery tests");
    int64_t supply  = 5000000000000;

    BOOST_CHECK_EQUAL(errgallery.no_balance, gallery.create(point._symbol));
    BOOST_CHECK_EQUAL(success(), point.create(_golos, asset(supply * 2, point._symbol), 10000, 1));
    BOOST_CHECK_EQUAL(success(), point.open(_code, point._symbol, _code));
    BOOST_CHECK_EQUAL(success(), gallery.create(point._symbol));

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(createmosaic_tests, commun_gallery_tester) try {
    BOOST_TEST_MESSAGE("createmosaic tests");
    int64_t supply  = 5000000000000;
    int64_t reserve = 50000000000;
    uint16_t royalty = 2500;
    int64_t min_gem_points = commun::safe_prop(commun::config::min_gem_cost, supply, reserve);
    int64_t init_amount = supply / 2;

    BOOST_CHECK_EQUAL(success(), point.create(_golos, asset(supply * 2, point._symbol), 10000, 1));

    BOOST_CHECK_EQUAL(success(), token.create(_commun, asset(reserve, token._symbol)));
    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _carol, asset(reserve, token._symbol), ""));
    BOOST_CHECK_EQUAL(success(), token.transfer(_carol, commun_point_name, asset(reserve, token._symbol), cfg::restock_prefix + point_code_str));

    BOOST_CHECK_EQUAL(success(), point.open(_code, point._symbol, _code));
    BOOST_CHECK_EQUAL(success(), point.issue(_golos, _golos, asset(supply, point._symbol), std::string(point_code_str) + " issue"));
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _alice, asset(init_amount, point._symbol)));
    BOOST_CHECK_EQUAL(success(), gallery.create(point._symbol));

    BOOST_CHECK_EQUAL(errgallery.overdrawn_balance, gallery.createmosaic(_alice, 1, asset(point.get_amount(_alice) + 1, point._symbol), royalty));
    BOOST_CHECK_EQUAL(errgallery.insufficient_quantity(_alice), gallery.createmosaic(_alice, 1, asset(min_gem_points - 1, point._symbol), royalty));
    BOOST_CHECK_EQUAL(success(), gallery.createmosaic(_alice, 1, asset(min_gem_points, point._symbol), royalty));

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(addtomosaic_tests, commun_gallery_tester) try {
    BOOST_TEST_MESSAGE("addtomosaic tests");
    BOOST_CHECK_EQUAL(errgallery.no_mosaic, gallery.addtomosaic(_alice, 1, asset(point.get_amount(_carol), point._symbol), false, _carol));
    init();

    BOOST_CHECK_EQUAL(success(), gallery.addtomosaic(_alice, 1, asset(point.get_amount(_carol), point._symbol), false, _carol));

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(claimgem_tests, commun_gallery_tester) try {
    BOOST_TEST_MESSAGE("claimgem tests");
    int64_t supply  = 5000000000000;
    init();

    BOOST_CHECK_EQUAL(success(), gallery.addtomosaic(_alice, 1, asset(point.get_amount(_carol), point._symbol), false, _carol));
    produce_block();
    produce_block(fc::seconds(commun::config::default_evaluation_period - (cfg::block_interval_ms / 1000)));
    BOOST_CHECK_EQUAL(errgallery.eval_period, gallery.claimgem(_alice, 1, point._symbol.to_symbol_code(), _alice));

    produce_blocks(1);

    BOOST_CHECK_EQUAL(point.get_supply(), supply);
    BOOST_CHECK_EQUAL(success(), gallery.claimgem(_alice, 1, point._symbol.to_symbol_code(), _carol)); //carol got nothing

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(provide_tests, commun_gallery_tester) try {
    BOOST_TEST_MESSAGE("provide tests");

    BOOST_CHECK_EQUAL(errgallery.no_points_provided, gallery.provide(_alice, _carol, asset(0, point._symbol)));
    BOOST_CHECK_EQUAL(success(), gallery.provide(_alice, _carol, asset(1000, point._symbol)));
    BOOST_CHECK_EQUAL(success(), gallery.provide(_alice, _carol, asset(0, point._symbol)));
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
