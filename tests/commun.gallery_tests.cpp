#include "gallery_tester.hpp"
#include "commun.ctrl_test_api.hpp"
#include "commun.point_test_api.hpp"
#include "commun.emit_test_api.hpp"
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
using commun::config::commun_ctrl_name;
using commun::config::default_mosaic_active_period;
using commun::config::default_evaluation_period;
using commun::config::reward_mosaics_period;
using commun::config::forced_chopping_delay;

const account_name _commun = N(commun);
const account_name _golos = N(golos);
const account_name _alice = N(alice);
const account_name _bob = N(bob);
const account_name _carol = N(carol);

const uint16_t _max_witnesses = 2;
const uint16_t _max_witness_votes = 4;

class commun_gallery_tester : public gallery_tester {
protected:
    cyber_token_api token;
    commun_point_api point;
    commun_ctrl_api ctrl;
    commun_emit_api emit;
    commun_gallery_api gallery;
public:
    commun_gallery_tester()
        : gallery_tester(commun_gallery_name)
        , token({this, cfg::token_name, cfg::reserve_token})
        , point({this, commun_point_name, _point})
        , ctrl({this, commun_ctrl_name, _point.to_symbol_code(), _golos})
        , emit({this, cfg::commun_emit_name})
        , gallery({this, _code, _point})
    {
        create_accounts({_commun, _golos, _alice, _bob, _carol,
            cfg::token_name, commun_ctrl_name, commun_point_name, commun_gallery_name, commun_emit_name});
        produce_block();
        install_contract(commun_ctrl_name, contracts::ctrl_wasm(), contracts::ctrl_abi());
        install_contract(commun_point_name, contracts::point_wasm(), contracts::point_abi());
        install_contract(commun_emit_name, contracts::emit_wasm(), contracts::emit_abi());
        install_contract(cfg::token_name, contracts::token_wasm(), contracts::token_abi());
        install_contract(_code, contracts::gallery_wasm(), contracts::gallery_abi());
        
        set_authority(commun_emit_name, cfg::reward_perm_name, create_code_authority({_code}), "active");
        link_authority(commun_emit_name, commun_emit_name, cfg::reward_perm_name, N(issuereward));
        
        std::vector<account_name> transfer_perm_accs{_code, commun_emit_name};
        std::sort(transfer_perm_accs.begin(), transfer_perm_accs.end());
        set_authority(commun_point_name, cfg::issue_permission, create_code_authority({commun_emit_name}), "active");
        set_authority(commun_point_name, cfg::transfer_permission, create_code_authority(transfer_perm_accs), "active");
        set_authority(commun_ctrl_name, N(changepoints), create_code_authority({commun_point_name}), "active");

        link_authority(commun_point_name, commun_point_name, cfg::issue_permission, N(issue));
        link_authority(commun_point_name, commun_point_name, cfg::transfer_permission, N(transfer));
        link_authority(commun_ctrl_name, commun_ctrl_name, N(changepoints), N(changepoints));
    }
    
    void init() {
        supply  = 5000000000000;
        reserve = 50000000000;
        royalty = 2500;
        min_gem_points = commun::safe_prop(commun::config::min_gem_cost, supply, reserve);
        annual_emission_rate = 1000;
        leaders_reward_prop = 1000;
        block_interval = cfg::block_interval_ms / 1000;
        
        BOOST_CHECK_EQUAL(success(), token.create(_commun, asset(reserve, token._symbol)));
        BOOST_CHECK_EQUAL(success(), token.issue(_commun, _carol, asset(reserve, token._symbol), ""));

        BOOST_CHECK_EQUAL(success(), point.create(_golos, asset(supply * 2, point._symbol), 10000, 1));
        BOOST_CHECK_EQUAL(success(), point.setfreezer(commun::config::commun_gallery_name));
        
        BOOST_CHECK_EQUAL(success(), emit.create(point._symbol, annual_emission_rate, leaders_reward_prop));
        
        BOOST_CHECK_EQUAL(success(), token.transfer(_carol, commun_point_name, asset(reserve, token._symbol), cfg::restock_prefix + point_code_str));
        BOOST_CHECK_EQUAL(success(), point.issue(_golos, _golos, asset(supply, point._symbol), std::string(point_code_str) + " issue"));
        BOOST_CHECK_EQUAL(errgallery.no_balance, gallery.create(point._symbol));
        BOOST_CHECK_EQUAL(success(), point.open(_code, point._symbol, _code));
        BOOST_CHECK_EQUAL(success(), gallery.create(point._symbol));

    }
    
    void prepare_ctrl(const std::vector<account_name>& leaders, account_name voter) {
    
        BOOST_CHECK_EQUAL(success(), ctrl.set_params(ctrl.default_params(_golos, _max_witnesses, _max_witness_votes)));
        produce_block();
        ctrl.prepare_multisig(_golos);
        produce_block();
        for (int i = 0; i < leaders.size(); i++) {
            BOOST_CHECK_EQUAL(success(), ctrl.reg_witness(leaders[i], "localhost"));
            ctrl.vote_witness(voter, leaders[i]);
        }
        produce_block();
    }
    
    int64_t supply;
    int64_t reserve;
    uint16_t royalty;
    uint16_t annual_emission_rate;
    uint16_t leaders_reward_prop;
    int64_t min_gem_points;
    int64_t block_interval;

};

BOOST_AUTO_TEST_SUITE(gallery_tests)

BOOST_FIXTURE_TEST_CASE(basic_tests, commun_gallery_tester) try {
    BOOST_TEST_MESSAGE("Basic gallery tests");
    init();
    int64_t init_amount = supply / 2;
    
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _alice, asset(init_amount, point._symbol)));
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _carol, asset(init_amount, point._symbol)));

    BOOST_CHECK_EQUAL(errgallery.overdrawn_balance, gallery.createmosaic(_alice, 1, asset(point.get_amount(_alice) + 1, point._symbol), royalty));
    BOOST_CHECK_EQUAL(errgallery.insufficient_quantity, gallery.createmosaic(_alice, 1, asset(min_gem_points - 1, point._symbol), royalty));
    BOOST_CHECK_EQUAL(success(), gallery.createmosaic(_alice, 1, asset(min_gem_points, point._symbol), royalty));
    BOOST_CHECK_EQUAL(success(), gallery.addtomosaic(_alice, 1, asset(point.get_amount(_carol), point._symbol), false, _carol));

    produce_block();

    produce_block(fc::seconds(default_evaluation_period - (cfg::block_interval_ms / 1000)));
    BOOST_CHECK_EQUAL(errgallery.eval_period, gallery.claimgem(_alice, 1, point._symbol.to_symbol_code(), _alice));
    
    produce_blocks(1);

    BOOST_CHECK_EQUAL(point.get_supply(), supply);
    BOOST_CHECK_EQUAL(success(), gallery.claimgem(_alice, 1, point._symbol.to_symbol_code(), _carol)); //carol got nothing
    auto reward = point.get_supply() - supply;
    BOOST_TEST_MESSAGE("--- reward = " << reward);
    BOOST_CHECK_EQUAL(success(), gallery.claimgem(_alice, 1, point._symbol.to_symbol_code(), _alice));
    BOOST_CHECK_EQUAL(point.get_amount(_alice), init_amount + reward);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(provide_test, commun_gallery_tester) try {
    BOOST_TEST_MESSAGE("Provide test");
    init();
    int64_t init_amount = supply / 2;
    uint16_t fee = 5000;
    
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _alice, asset(init_amount, point._symbol)));
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _carol, asset(init_amount, point._symbol)));
    BOOST_CHECK_EQUAL(success(), point.open(_bob, point._symbol, _bob));
    
    BOOST_CHECK_EQUAL(errgallery.no_points_provided, gallery.provide(_alice, _carol, asset(0, point._symbol)));
    BOOST_CHECK_EQUAL(success(), gallery.provide(_alice, _bob, asset(init_amount, point._symbol), fee));
    BOOST_CHECK_EQUAL(success(), gallery.provide(_carol, _bob, asset(init_amount, point._symbol), fee));
    
    BOOST_CHECK_EQUAL(success(), gallery.createmosaic(_bob, 1, asset(0, point._symbol), royalty, {std::make_pair(_alice, init_amount)}));
    produce_block();
    produce_block(fc::seconds(reward_mosaics_period - block_interval));
    //while creating this mosaic - the previous one receives a reward
    BOOST_CHECK_EQUAL(success(), gallery.createmosaic(_bob, 2, asset(0, point._symbol), royalty, {std::make_pair(_carol, init_amount)}));
    
    produce_block();
    produce_block(fc::seconds(default_mosaic_active_period - reward_mosaics_period - block_interval));
    BOOST_CHECK_EQUAL(errgallery.overdrawn_balance, gallery.createmosaic(_bob, 3, asset(0, point._symbol), royalty, {std::make_pair(_alice, init_amount)}));
    produce_block();
    BOOST_CHECK_EQUAL(success(), gallery.createmosaic(_bob, 3, asset(0, point._symbol), royalty, {std::make_pair(_alice, init_amount)}));
    produce_block();
    auto reward = point.get_amount(_bob) * 2;
    BOOST_CHECK(reward > 0);
    BOOST_TEST_MESSAGE("--- reward = " << reward);
    BOOST_CHECK_EQUAL((point.get_amount(_alice) - init_amount), reward / 2); // fee == 50%
    
    produce_block(fc::seconds(forced_chopping_delay + reward_mosaics_period - block_interval));
    BOOST_CHECK_EQUAL(gallery.get_frozen(_carol), init_amount);
    BOOST_CHECK_EQUAL(success(), gallery.createmosaic(_bob, 4, asset(reward / 2, point._symbol), royalty));
    BOOST_CHECK(point.get_amount(_bob) > reward / 2);
    
    //carol points are unfrozen now, but she did not receive a reward
    BOOST_CHECK_EQUAL(point.get_amount(_carol), init_amount);
    BOOST_CHECK_EQUAL(gallery.get_frozen(_carol), 0);    
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(reward_the_top_test, commun_gallery_tester) try {
    BOOST_TEST_MESSAGE("Reward the top");
    init();
    int mosaics_num = 50;
    int64_t min_gem_points = commun::safe_prop(commun::config::min_gem_cost, supply, reserve);
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _bob, asset(supply, point._symbol)));
    prepare_ctrl({_alice, _carol}, _bob);
    int64_t points_sum = 0;
    auto first_comm_mosaic = mosaics_num - cfg::default_comm_grades.size() + 1;
    for (int i = 1; i <= mosaics_num; i++) {
        int64_t cur_points = min_gem_points * i;
        BOOST_CHECK_EQUAL(success(), gallery.createmosaic(_bob, i, asset(cur_points, point._symbol), royalty));
        if (i >= first_comm_mosaic) {
            points_sum += cur_points;
        }
    }
    std::map<uint64_t, int64_t> ranked_mosaics;
    
    BOOST_CHECK_EQUAL(errgallery.not_a_leader(_bob), gallery.advise(point._symbol.to_symbol_code(), _bob, {{_bob, 9},{_bob, 10}}));
    
    //these advices should be replaced after the next action
    BOOST_CHECK_EQUAL(success(), gallery.advise(point._symbol.to_symbol_code(), _alice, {{_bob, 9},{_bob, 10}}));
    
    BOOST_CHECK_EQUAL(success(), gallery.advise(point._symbol.to_symbol_code(), _alice, {{_bob, 1},{_bob, 3},{_bob, 50}}));
    BOOST_CHECK_EQUAL(success(), gallery.advise(point._symbol.to_symbol_code(), _carol, {{_bob, 1},{_bob, 2}}));
    ranked_mosaics[ 1] = cfg::default_lead_grades[0];
    ranked_mosaics[ 2] = cfg::default_lead_grades[1];
    ranked_mosaics[50] = cfg::default_lead_grades[2];
    ranked_mosaics[ 3] = cfg::default_lead_grades[3];
    
    BOOST_TEST_MESSAGE("--- points_sum" << points_sum);
    
    produce_block();
    produce_block(fc::seconds(reward_mosaics_period - block_interval));
    BOOST_CHECK_EQUAL(success(), gallery.addtomosaic(_bob, 1, asset(min_gem_points, point._symbol), false, _bob));
    
    for (int i = first_comm_mosaic; i <= mosaics_num; i++) {
        int64_t cur_points = min_gem_points * i;
        ranked_mosaics[i] += cfg::default_comm_grades[mosaics_num - i] + commun::safe_prop(cfg::default_comm_points_grade_sum, cur_points, points_sum);
        BOOST_TEST_MESSAGE("--- comm_grades_" << i << " = " << ranked_mosaics[i]);
    }
    
    std::vector<std::pair<uint64_t, int64_t> > top_mosaics;
    top_mosaics.reserve(ranked_mosaics.size());
    for (const auto& m : ranked_mosaics) {
        top_mosaics.emplace_back(m);
    }
    
    std::sort(top_mosaics.begin(), top_mosaics.end(),
        [](const std::pair<uint64_t, int64_t>& lhs, const std::pair<uint64_t, int64_t>& rhs) { return lhs.second > rhs.second; });
    uint64_t grades_sum = 0;
    for (int i = 0; i < cfg::default_rewarded_num; i++) {
        grades_sum += top_mosaics[i].second;
    }
    
    auto total_reward = point.get_supply() - supply;
    std::map<uint64_t, int64_t> rewards;
    auto left_reward  = total_reward;
    
    for (int i = 0; i < cfg::default_rewarded_num; i++) {
        auto cur_reward = commun::safe_prop(total_reward, top_mosaics[i].second, grades_sum);
        rewards[top_mosaics[i].first] = cur_reward;
        left_reward -= cur_reward;
    }
    rewards[top_mosaics.front().first] += left_reward;
    
    for (int i = 1; i <= mosaics_num; i++) {
        auto cur_reward = get_mosaic(_code, _point, _bob, i)["reward"].as<int64_t>();
        BOOST_CHECK_EQUAL(rewards[i], cur_reward);
        BOOST_TEST_MESSAGE("--- reward_" << i << " = " << cur_reward);
    }

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
    init();
    int64_t init_amount = supply / 2;
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _alice, asset(init_amount, point._symbol)));
    BOOST_CHECK_EQUAL(errgallery.overdrawn_balance, gallery.createmosaic(_alice, 1, asset(point.get_amount(_alice) + 1, point._symbol), royalty));
    BOOST_CHECK_EQUAL(errgallery.insufficient_quantity, gallery.createmosaic(_alice, 1, asset(min_gem_points - 1, point._symbol), royalty));
    BOOST_CHECK_EQUAL(success(), gallery.createmosaic(_alice, 1, asset(min_gem_points, point._symbol), royalty));

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(addtomosaic_tests, commun_gallery_tester) try {
    BOOST_TEST_MESSAGE("addtomosaic tests");
    init();
    int64_t init_amount = supply / 2;
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _alice, asset(init_amount, point._symbol)));
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _carol, asset(init_amount, point._symbol)));
    BOOST_CHECK_EQUAL(errgallery.no_mosaic, gallery.addtomosaic(_alice, 1, asset(point.get_amount(_carol), point._symbol), false, _carol));
    BOOST_CHECK_EQUAL(success(), gallery.createmosaic(_alice, 1, asset(min_gem_points, point._symbol), royalty));
    BOOST_CHECK_EQUAL(success(), gallery.addtomosaic(_alice, 1, asset(point.get_amount(_carol), point._symbol), false, _carol));

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(claimgem_tests, commun_gallery_tester) try {
    BOOST_TEST_MESSAGE("claimgem tests");
    int64_t supply  = 5000000000000;
    init();
    int64_t init_amount = supply / 2;
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _alice, asset(init_amount, point._symbol)));
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _carol, asset(init_amount, point._symbol)));
    BOOST_CHECK_EQUAL(success(), gallery.createmosaic(_alice, 1, asset(min_gem_points, point._symbol), royalty));
    BOOST_CHECK_EQUAL(success(), gallery.addtomosaic(_alice, 1, asset(point.get_amount(_carol), point._symbol), false, _carol));
    produce_block();
    produce_block(fc::seconds(commun::config::default_evaluation_period - (cfg::block_interval_ms / 1000)));
    BOOST_CHECK_EQUAL(errgallery.eval_period, gallery.claimgem(_alice, 1, point._symbol.to_symbol_code(), _alice));

    produce_blocks(1);

    BOOST_CHECK_EQUAL(point.get_supply(), supply);
    BOOST_CHECK_EQUAL(success(), gallery.claimgem(_alice, 1, point._symbol.to_symbol_code(), _carol)); //carol got nothing

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
