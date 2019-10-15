#include "gallery_tester.hpp"
#include "commun.point_test_api.hpp"
#include "commun.emit_test_api.hpp"
#include "commun.list_test_api.hpp"
#include "commun.gallery_test_api.hpp"
#include "cyber.token_test_api.hpp"
#include "contracts.hpp"
#include "../commun.point/include/commun.point/config.hpp"
#include "../commun.gallery/include/commun.gallery/config.hpp"
#include "../commun.emit/include/commun.emit/config.hpp"
#include "../commun.list/include/commun.list/config.hpp"
using int128_t = fc::int128_t;
#include "../include/commun/util.hpp"

namespace cfg = commun::config;
using namespace eosio::testing;
using namespace eosio::chain;
using namespace fc;
static const auto point_code_str = "GLS";
static const auto _point = symbol(3, point_code_str);
static const auto point_code = _point.to_symbol_code();

const account_name _commun = cfg::dapp_name;
const account_name _golos = N(golos);
const account_name _alice = N(alice);
const account_name _bob = N(bob);
const account_name _carol = N(carol);

class commun_gallery_tester : public gallery_tester {
protected:
    cyber_token_api token;
    commun_point_api point;
    commun_ctrl_api ctrl;
    commun_emit_api emit;
    commun_list_api community;
    commun_gallery_api gallery;
public:
    commun_gallery_tester()
        : gallery_tester(cfg::gallery_name)
        , token({this, cfg::token_name, cfg::reserve_token})
        , point({this, cfg::point_name, _point})
        , ctrl({this, cfg::control_name, _point.to_symbol_code(), _golos})
        , emit({this, cfg::emit_name})
        , community({this, cfg::list_name})
        , gallery({this, _code, _point})
    {
        create_accounts({_commun, _golos, _alice, _bob, _carol,
            cfg::token_name, cfg::control_name, cfg::point_name, cfg::list_name, cfg::gallery_name, cfg::emit_name});
        produce_block();
        install_contract(cfg::control_name, contracts::ctrl_wasm(), contracts::ctrl_abi());
        install_contract(cfg::point_name, contracts::point_wasm(), contracts::point_abi());
        install_contract(cfg::emit_name, contracts::emit_wasm(), contracts::emit_abi());
        install_contract(cfg::token_name, contracts::token_wasm(), contracts::token_abi());
        install_contract(cfg::list_name, contracts::list_wasm(), contracts::list_abi());
        install_contract(_code, contracts::gallery_wasm(), contracts::gallery_abi());
        
        set_authority(cfg::emit_name, cfg::reward_perm_name, create_code_authority({_code}), "active");
        link_authority(cfg::emit_name, cfg::emit_name, cfg::reward_perm_name, N(issuereward));
        
        set_authority(cfg::emit_name, N(create), create_code_authority({cfg::list_name}), "active");
        link_authority(cfg::emit_name, cfg::emit_name, N(create), N(create));
        
        set_authority(cfg::control_name, N(create), create_code_authority({cfg::list_name}), "active");
        link_authority(cfg::control_name, cfg::control_name, N(create), N(create));
        
        std::vector<account_name> transfer_perm_accs{_code, cfg::emit_name};
        std::sort(transfer_perm_accs.begin(), transfer_perm_accs.end());
        set_authority(cfg::point_name, cfg::issue_permission, create_code_authority({cfg::emit_name}), "active");
        set_authority(cfg::point_name, cfg::transfer_permission, create_code_authority(transfer_perm_accs), "active");
        set_authority(cfg::control_name, N(changepoints), create_code_authority({cfg::point_name}), "active");

        link_authority(cfg::point_name, cfg::point_name, cfg::issue_permission, N(issue));
        link_authority(cfg::point_name, cfg::point_name, cfg::transfer_permission, N(transfer));
        link_authority(cfg::control_name, cfg::control_name, N(changepoints), N(changepoints));
    }
    
    void init() {
        supply  = 5000000000000;
        reserve = 50000000000;
        royalty = 2500;
        min_gem_points = std::max(gallery.default_mosaic_pledge, std::max(gallery.default_min_mosaic_inclusion, gallery.default_min_gem_inclusion));
        annual_emission_rate = 1000;
        leaders_reward_prop = 1000;
        block_interval = cfg::block_interval_ms / 1000;
        
        BOOST_CHECK_EQUAL(success(), token.create(_commun, asset(reserve, token._symbol)));
        BOOST_CHECK_EQUAL(success(), token.issue(_commun, _carol, asset(reserve, token._symbol), ""));

        BOOST_CHECK_EQUAL(success(), point.create(_golos, asset(supply * 2, point._symbol), 10000, 1));
        BOOST_CHECK_EQUAL(success(), point.setfreezer(cfg::gallery_name));
        
        BOOST_CHECK_EQUAL(success(), token.transfer(_carol, cfg::point_name, asset(reserve, token._symbol), cfg::restock_prefix + point_code_str));
        BOOST_CHECK_EQUAL(success(), point.issue(_golos, _golos, asset(supply, point._symbol), std::string(point_code_str) + " issue"));
        BOOST_CHECK_EQUAL(success(), point.open(_code, point_code, _code));

        BOOST_CHECK_EQUAL(success(), community.create(cfg::list_name, point_code, "community 1"));
        BOOST_CHECK_EQUAL(success(), community.setparams(_golos, point_code, community.args()
            ("emission_rate", annual_emission_rate)
            ("leaders_percent", leaders_reward_prop)));
        BOOST_CHECK_EQUAL(success(), community.setsysparams( point_code, community.sysparams()
            ("opuses", std::set<opus_info>{gallery.default_opus} )));
    }
    
    int64_t supply;
    int64_t reserve;
    uint16_t royalty;
    uint16_t annual_emission_rate;
    uint16_t leaders_reward_prop;
    int64_t min_gem_points;
    int64_t block_interval;
};

BOOST_AUTO_TEST_SUITE(commun_gallery_tests)

BOOST_FIXTURE_TEST_CASE(basic_tests, commun_gallery_tester) try {
    BOOST_TEST_MESSAGE("Basic gallery tests");
    init();
    int64_t init_amount = supply / 2;
    
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _alice, asset(init_amount, point._symbol)));
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _carol, asset(init_amount, point._symbol)));

    BOOST_CHECK_EQUAL(errgallery.overdrawn_balance, gallery.createmosaic(_alice, 1, gallery.default_opus.name, asset(point.get_amount(_alice) + 1, point._symbol), royalty));
    BOOST_CHECK_EQUAL(errgallery.not_enough_for_mosaic, gallery.createmosaic(_alice, 1, gallery.default_opus.name, asset(min_gem_points - 1, point._symbol), royalty));
    BOOST_CHECK_EQUAL(success(), gallery.createmosaic(_alice, 1, gallery.default_opus.name, asset(min_gem_points, point._symbol), royalty));
    
    produce_block();
    produce_block(fc::seconds(cfg::reward_mosaics_period - block_interval));
    
    BOOST_CHECK_EQUAL(success(), gallery.addtomosaic(1, asset(point.get_amount(_carol), point._symbol), false, _carol));

    produce_block();
    produce_block(fc::seconds(cfg::def_moderation_period - cfg::reward_mosaics_period - block_interval));
    BOOST_CHECK_EQUAL(errgallery.moderation_period, gallery.claim(1, _alice));
    
    produce_blocks(1);

    BOOST_CHECK_EQUAL(success(), gallery.claim(1, _carol)); //carol got nothing
    BOOST_CHECK_EQUAL(success(), gallery.claim(1, _alice));
    auto reward = point.get_supply() - supply;
    BOOST_TEST_MESSAGE("--- reward = " << reward);
    BOOST_CHECK_EQUAL(point.get_amount(_alice), init_amount + reward);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(provide_test, commun_gallery_tester) try {
    BOOST_TEST_MESSAGE("Provide test");
    init();
    int64_t init_amount = supply / 2;
    uint16_t fee = 5000;
    
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _alice, asset(init_amount, point._symbol)));
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _carol, asset(init_amount, point._symbol)));
    BOOST_CHECK_EQUAL(success(), point.open(_bob, point_code, _bob));
    
    BOOST_CHECK_EQUAL(errgallery.no_points_provided, gallery.provide(_alice, _carol, asset(0, point._symbol)));
    BOOST_CHECK_EQUAL(success(), gallery.provide(_alice, _bob, asset(init_amount, point._symbol), fee));
    BOOST_CHECK_EQUAL(success(), gallery.provide(_carol, _bob, asset(init_amount, point._symbol), fee));
    
    BOOST_CHECK_EQUAL(success(), gallery.createmosaic(_bob, 1, gallery.default_opus.name, asset(0, point._symbol), royalty, {std::make_pair(_alice, init_amount / 2)}));
    produce_block();
    produce_block(fc::seconds(cfg::reward_mosaics_period - block_interval));
    
    BOOST_CHECK_EQUAL(success(), gallery.addtomosaic(1, asset(0, point._symbol), false, _bob, {std::make_pair(_alice, init_amount / 2)}));
    produce_block();
    produce_block(fc::seconds(cfg::reward_mosaics_period - block_interval));
    
    //while creating this mosaic - the previous one receives a reward
    BOOST_CHECK_EQUAL(success(), gallery.createmosaic(_bob, 2, gallery.default_opus.name, asset(0, point._symbol), royalty, {std::make_pair(_carol, init_amount)}));
    
    produce_block();
    produce_block(fc::seconds(cfg::def_active_period - (2 * cfg::reward_mosaics_period) - block_interval));

    BOOST_CHECK_EQUAL(errgallery.overdrawn_balance, gallery.createmosaic(_bob, 3, gallery.default_opus.name, asset(0, point._symbol), royalty, {std::make_pair(_alice, init_amount)}));
    produce_block();
    BOOST_CHECK_EQUAL(success(), gallery.createmosaic(_bob, 3, gallery.default_opus.name, asset(0, point._symbol), royalty, {std::make_pair(_alice, init_amount)}));
    produce_block();
    auto reward = point.get_amount(_bob) * 2;
    BOOST_CHECK(reward > 0);
    BOOST_TEST_MESSAGE("--- reward = " << reward);
    BOOST_CHECK_EQUAL((point.get_amount(_alice) - init_amount), reward / 2); // fee == 50%
    
    produce_block(fc::seconds(cfg::forced_chopping_delay + cfg::reward_mosaics_period - block_interval));
    BOOST_CHECK_EQUAL(gallery.get_frozen(_carol), init_amount);
    
    produce_block(fc::seconds(cfg::reward_mosaics_period));
    
    BOOST_CHECK_EQUAL(success(), gallery.createmosaic(_bob, 4, gallery.default_opus.name, asset(reward / 2, point._symbol), royalty));
    BOOST_CHECK(point.get_amount(_bob) > reward / 2);
    
    //carol points are unfrozen now, but she did not receive a reward
    BOOST_CHECK_EQUAL(point.get_amount(_carol), init_amount);
    BOOST_CHECK_EQUAL(gallery.get_frozen(_carol), 0);    
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(reward_the_top_test, commun_gallery_tester) try {
    BOOST_TEST_MESSAGE("Reward the top");
    init();
    BOOST_CHECK_EQUAL(success(), community.setsysparams(point_code, community.sysparams()
        ("min_lead_rating", 1)));
    int mosaics_num = 50;
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _bob, asset(supply, point._symbol)));
    ctrl.prepare({_alice, _carol}, _bob);
    int64_t points_sum = 0;
    auto first_comm_mosaic = mosaics_num - cfg::default_comm_grades.size() + 1;
    for (int i = 1; i <= mosaics_num; i++) {
        int64_t cur_points = min_gem_points * i;
        BOOST_CHECK_EQUAL(success(), gallery.createmosaic(_bob, i, gallery.default_opus.name, asset(cur_points, point._symbol), royalty));
        if (i >= first_comm_mosaic) {
            points_sum += cur_points - gallery.default_mosaic_pledge;
        }
    }
    std::map<uint64_t, int64_t> ranked_mosaics;
    
    BOOST_CHECK_EQUAL(errgallery.not_a_leader(_bob), gallery.advise(_bob, {9 , 10}));
    
    //these advices should be replaced after the next action
    BOOST_CHECK_EQUAL(success(), gallery.advise(_alice, {9 , 10}));
    
    BOOST_CHECK_EQUAL(success(), gallery.advise(_alice, {1, 3, 50}));
    BOOST_CHECK_EQUAL(success(), gallery.advise(_carol, {1, 2}));
    ranked_mosaics[ 1] = cfg::default_lead_grades[0];
    ranked_mosaics[ 2] = cfg::default_lead_grades[1];
    ranked_mosaics[50] = cfg::default_lead_grades[2];
    ranked_mosaics[ 3] = cfg::default_lead_grades[3];
    
    BOOST_TEST_MESSAGE("--- points_sum" << points_sum);
    
    produce_block();
    produce_block(fc::seconds(cfg::reward_mosaics_period - block_interval));
    BOOST_CHECK_EQUAL(success(), gallery.addtomosaic(1, asset(min_gem_points, point._symbol), false, _bob));
    
    produce_block();
    produce_block(fc::seconds(cfg::reward_mosaics_period - block_interval));
    
    BOOST_CHECK_EQUAL(success(), gallery.addtomosaic(1, asset(min_gem_points, point._symbol), false, _bob));
    
    for (int i = first_comm_mosaic; i <= mosaics_num; i++) {
        int64_t cur_points = min_gem_points * i - gallery.default_mosaic_pledge;
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
    for (int i = 0; i < cfg::def_rewarded_mosaic_num; i++) {
        grades_sum += top_mosaics[i].second;
    }
    
    auto total_reward = point.get_supply() - supply;
    std::map<uint64_t, int64_t> rewards;
    auto left_reward  = total_reward;
    
    for (int i = 0; i < cfg::def_rewarded_mosaic_num; i++) {
        auto cur_reward = commun::safe_prop(total_reward, top_mosaics[i].second, grades_sum);
        rewards[top_mosaics[i].first] = cur_reward;
        left_reward -= cur_reward;
    }
    
    for (int i = 1; i <= mosaics_num; i++) {
        auto cur_reward = get_mosaic(_code, _point, i)["reward"].as<int64_t>();
        BOOST_CHECK_EQUAL(rewards[i], cur_reward);
        BOOST_TEST_MESSAGE("--- reward_" << i << " = " << cur_reward);
    }

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE(createmosaic_tests, commun_gallery_tester) try {
    BOOST_TEST_MESSAGE("createmosaic tests");
    init();
    int64_t init_amount = supply / 2;
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _alice, asset(init_amount, point._symbol)));
    BOOST_CHECK_EQUAL(errgallery.overdrawn_balance, gallery.createmosaic(_alice, 1, gallery.default_opus.name, asset(point.get_amount(_alice) + 1, point._symbol), royalty));
    BOOST_CHECK_EQUAL(errgallery.not_enough_for_mosaic, gallery.createmosaic(_alice, 1, gallery.default_opus.name, asset(min_gem_points - 1, point._symbol), royalty));
    BOOST_CHECK_EQUAL(success(), gallery.createmosaic(_alice, 1, gallery.default_opus.name, asset(min_gem_points, point._symbol), royalty));

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(addtomosaic_tests, commun_gallery_tester) try {
    BOOST_TEST_MESSAGE("addtomosaic tests");
    init();
    int64_t init_amount = supply / 2;
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _alice, asset(init_amount, point._symbol)));
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _carol, asset(init_amount, point._symbol)));
    BOOST_CHECK_EQUAL(errgallery.no_mosaic, gallery.addtomosaic(1, asset(point.get_amount(_carol), point._symbol), false, _carol));
    BOOST_CHECK_EQUAL(success(), gallery.createmosaic(_alice, 1, gallery.default_opus.name, asset(min_gem_points, point._symbol), royalty));
    BOOST_CHECK_EQUAL(success(), gallery.addtomosaic(1, asset(point.get_amount(_carol), point._symbol), false, _carol));

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(claim_tests, commun_gallery_tester) try {
    BOOST_TEST_MESSAGE("claim tests");
    int64_t supply  = 5000000000000;
    init();
    int64_t init_amount = supply / 2;
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _alice, asset(init_amount, point._symbol)));
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _carol, asset(init_amount, point._symbol)));
    BOOST_CHECK_EQUAL(success(), gallery.createmosaic(_alice, 1, gallery.default_opus.name, asset(min_gem_points, point._symbol), royalty));
    BOOST_CHECK_EQUAL(success(), gallery.addtomosaic(1, asset(point.get_amount(_carol), point._symbol), false, _carol));
    produce_block();
    produce_block(fc::seconds(cfg::def_moderation_period - (cfg::block_interval_ms / 1000)));
    BOOST_CHECK_EQUAL(errgallery.moderation_period, gallery.claim(1, _alice));

    produce_blocks(1);

    BOOST_CHECK_EQUAL(point.get_supply(), supply);
    BOOST_CHECK_EQUAL(success(), gallery.claim(1, _carol)); //carol got nothing

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
