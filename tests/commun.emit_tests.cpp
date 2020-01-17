#include "golos_tester.hpp"
#include "commun.list_test_api.hpp"
#include "commun.point_test_api.hpp"
#include "commun.emit_test_api.hpp"
#include "cyber.token_test_api.hpp"
#include "contracts.hpp"
#include "../commun.point/include/commun.point/config.hpp"
#include "../commun.emit/include/commun.emit/config.hpp"

namespace cfg = commun::config;
using namespace eosio::testing;
using namespace eosio::chain;
using namespace fc;
static const auto point_code_str = "GLS";
static const auto _point = symbol(3, point_code_str);
static const auto point_code = _point.to_symbol_code();

static const int64_t block_interval = cfg::block_interval_ms / 1000;

class commun_emit_tester : public golos_tester {
protected:
    cyber_token_api token;
    commun_list_api community;
    commun_point_api point;
    commun_emit_api emit;

public:
    commun_emit_tester()
        : golos_tester(cfg::emit_name)
        , token({this, cfg::token_name, cfg::reserve_token})
        , community({this, cfg::list_name})
        , point({this, cfg::point_name, _point})
        , emit({this, _code})
    {
        create_accounts({_code, _commun, _golos, _alice, _bob, _carol,
            cfg::token_name, cfg::point_name, cfg::list_name, cfg::control_name, cfg::gallery_name});
        produce_block();
        install_contract(cfg::token_name, contracts::token_wasm(), contracts::token_abi());
        install_contract(cfg::list_name, contracts::list_wasm(), contracts::list_abi());
        install_contract(cfg::point_name, contracts::point_wasm(), contracts::point_abi());
        install_contract(cfg::gallery_name, contracts::gallery_wasm(), contracts::gallery_abi());
        install_contract(_code, contracts::emit_wasm(), contracts::emit_abi());

        set_authority(cfg::control_name, N(changepoints), create_code_authority({cfg::point_name}), "active");
        link_authority(cfg::control_name, cfg::control_name, N(changepoints), N(changepoints));

        set_authority(cfg::emit_name, cfg::reward_perm_name, create_code_authority({_code}), "active");
        link_authority(cfg::emit_name, cfg::emit_name, cfg::reward_perm_name, N(issuereward));

        set_authority(cfg::emit_name, N(init), create_code_authority({cfg::list_name}), "active");
        link_authority(cfg::emit_name, cfg::emit_name, N(init), N(init));

        set_authority(cfg::control_name, N(init), create_code_authority({cfg::list_name}), "active");
        link_authority(cfg::control_name, cfg::control_name, N(init), N(init));

        set_authority(cfg::gallery_name, N(init), create_code_authority({cfg::list_name}), "active");
        link_authority(cfg::gallery_name, cfg::gallery_name, N(init), N(init));

        set_authority(cfg::point_name, cfg::issue_permission, create_code_authority({cfg::emit_name}), "active");
        link_authority(cfg::point_name, cfg::point_name, cfg::issue_permission, N(issue));
    }

    void init() {
        set_authority(_golos, cfg::transfer_permission, create_code_authority({cfg::emit_name}), cfg::active_name);
        link_authority(_golos, cfg::point_name, cfg::transfer_permission, N(transfer));

        BOOST_CHECK_EQUAL(success(), point.create(_golos, asset(0, point._symbol), asset(supply * 2, point._symbol), 10000, 0));
        BOOST_CHECK_EQUAL(success(), token.create(_commun, asset(reserve * 2, token._symbol)));
        BOOST_CHECK_EQUAL(success(), token.issue(_commun, _carol, asset(reserve, token._symbol), ""));
        BOOST_CHECK_EQUAL(success(), token.transfer(_carol, cfg::point_name, asset(reserve, token._symbol), cfg::restock_prefix + point_code_str));
        BOOST_CHECK_EQUAL(success(), point.issue(_golos, asset(supply, point._symbol), std::string(point_code_str) + " issue"));
    }

    const account_name _commun = cfg::dapp_name;
    const account_name _golos = N(golos);
    const account_name _alice = N(alice);
    const account_name _bob = N(bob);
    const account_name _carol = N(carol);
    static constexpr int64_t seconds_per_year = int64_t(365)*24*60*60;
    static constexpr int64_t supply  = 5000000000000;
    static constexpr int64_t reserve = 100000;

    struct errors: contract_error_messages {
        const string no_community = amsg("community not exists");
        const string already_exists = amsg("already exists");
        const string no_emitter = amsg("emitter does not exists, create it before issue");
        const string wrong_annual_rate = amsg("incorrect emission rate");
        const string wrong_leaders_prop = amsg("incorrect leaders percent");
        const string no_account(name acc) { return amsg(acc.to_string() +  " contract does not exists"); }
    } err;
};

BOOST_AUTO_TEST_SUITE(commun_emit_tests)

BOOST_FIXTURE_TEST_CASE(init_tests, commun_emit_tester) try {
    BOOST_TEST_MESSAGE("init tests");

    BOOST_CHECK_EQUAL(err.no_community, emit.init(point_code));
    init();
    BOOST_CHECK_EQUAL(success(), community.create(cfg::list_name, point_code, "golos"));
    auto now = produce_block();
    BOOST_CHECK(!emit.get_stat(point_code).is_null());
    CHECK_MATCHING_OBJECT(emit.get_stat(point_code)["reward_receivers"][int(0)], mvo()
        ("contract", cfg::control_name)
        ("time", now->timestamp.to_time_point())
    );
    CHECK_MATCHING_OBJECT(emit.get_stat(point_code)["reward_receivers"][int(1)], mvo()
        ("contract", cfg::gallery_name)
        ("time", now->timestamp.to_time_point())
    );

    BOOST_CHECK_EQUAL(err.wrong_annual_rate, community.setparams( _golos, point_code, community.args()("emission_rate", 0)));
    BOOST_CHECK_EQUAL(err.wrong_annual_rate, community.setparams( _golos, point_code, community.args()("emission_rate", cfg::_100percent+1)));
    BOOST_CHECK_EQUAL(err.wrong_annual_rate, community.setparams( _golos, point_code, community.args()("emission_rate", cfg::_1percent * 2)));
    BOOST_CHECK_EQUAL(err.wrong_annual_rate, community.setparams( _golos, point_code, community.args()("emission_rate", cfg::_1percent * 55)));
    BOOST_CHECK_EQUAL(err.wrong_annual_rate, community.setparams( _golos, point_code, community.args()("emission_rate", cfg::_1percent * 22)));

    BOOST_CHECK_EQUAL(err.wrong_leaders_prop, community.setparams( _golos, point_code, community.args()("leaders_percent", cfg::_100percent+1)));
    BOOST_CHECK_EQUAL(err.wrong_leaders_prop, community.setparams( _golos, point_code, community.args()("leaders_percent", 0)));
    BOOST_CHECK_EQUAL(err.wrong_leaders_prop, community.setparams( _golos, point_code, community.args()("leaders_percent", cfg::_1percent * 11)));

    BOOST_CHECK_EQUAL(success(), community.setparams( _golos, point_code, community.args()
        ("emission_rate", cfg::_1percent)
        ("leaders_percent", cfg::_1percent)));
    BOOST_CHECK_EQUAL(success(), community.setparams( _golos, point_code, community.args()
        ("emission_rate", cfg::_1percent * 5)
        ("leaders_percent", cfg::_1percent)));
    BOOST_CHECK_EQUAL(success(), community.setparams( _golos, point_code, community.args()
        ("emission_rate", cfg::_1percent * 50)
        ("leaders_percent", cfg::_1percent * 10)));

    produce_block();
    BOOST_CHECK_EQUAL(err.already_exists, emit.init(point_code));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(issuereward_tests, commun_emit_tester) try {
    BOOST_TEST_MESSAGE("issuereward tests");
    BOOST_CHECK_EQUAL(err.no_emitter, emit.issuereward(point_code, cfg::gallery_name));
    init();
    BOOST_CHECK_EQUAL(success(), community.create(cfg::list_name, point_code, "golos"));
    BOOST_CHECK_EQUAL(success(), community.setparams( _golos, point_code, community.args()
        ("emission_rate", cfg::_1percent * 50)
        ("leaders_percent", cfg::_1percent * 10)));

    auto init_supply = point.get_supply();
    BOOST_CHECK_EQUAL(success(), emit.issuereward(point_code, cfg::gallery_name));
    BOOST_CHECK_EQUAL(success(), emit.issuereward(point_code, cfg::control_name));
    BOOST_CHECK_EQUAL(init_supply, point.get_supply());

    BOOST_TEST_MESSAGE("-- waiting for mosaics reward");
    produce_block();
    produce_block(fc::seconds(cfg::def_reward_mosaics_period - block_interval * 2));
    BOOST_CHECK_EQUAL(success(), emit.issuereward(point_code, cfg::gallery_name));
    BOOST_CHECK_EQUAL(init_supply, point.get_supply());
    produce_block();
    BOOST_CHECK_EQUAL(success(), point.open(cfg::gallery_name));
    BOOST_CHECK_EQUAL(success(), point.open(cfg::control_name));
    BOOST_CHECK_EQUAL(success(), emit.issuereward(point_code, cfg::gallery_name));
    auto new_supply = point.get_supply();
    BOOST_CHECK(new_supply > init_supply);
    BOOST_CHECK_EQUAL(success(), emit.issuereward(point_code, cfg::control_name));
    BOOST_CHECK_EQUAL(new_supply, point.get_supply());

    BOOST_TEST_MESSAGE("-- waiting for leaders reward");
    produce_block();
    produce_block(fc::seconds(cfg::def_reward_leaders_period - cfg::def_reward_mosaics_period - block_interval * 2));
    BOOST_CHECK_EQUAL(success(), emit.issuereward(point_code, cfg::control_name));
    BOOST_CHECK_EQUAL(new_supply, point.get_supply());
    produce_block();
    BOOST_CHECK_EQUAL(success(), emit.issuereward(point_code, cfg::control_name));
    BOOST_CHECK(point.get_supply() > new_supply);
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(basic_tests, commun_emit_tester) try {
    BOOST_TEST_MESSAGE("basic tests");
    init();
    double annual_rate = 0.5;
    double leaders_rate = 0.1;
    BOOST_CHECK_EQUAL(success(), community.create(cfg::list_name, point_code, "golos"));
    BOOST_CHECK_EQUAL(success(), community.setparams( _golos, point_code, community.args()
        ("emission_rate", uint16_t(annual_rate * cfg::_100percent))
        ("leaders_percent", uint16_t(leaders_rate*cfg::_100percent))));
    BOOST_CHECK_EQUAL(success(), point.open(cfg::gallery_name));

    int64_t cont_emission = supply * int64_t(std::log(1.0 + annual_rate) * cfg::_100percent) / cfg::_100percent;
    int64_t period_amount = cont_emission * cfg::def_reward_mosaics_period / seconds_per_year;
    int64_t mosaic_amount = period_amount * (1.0 - leaders_rate);
    BOOST_TEST_MESSAGE("-- waiting for mosaics reward");
    produce_block();
    produce_block(fc::seconds(cfg::def_reward_mosaics_period - block_interval));
    BOOST_CHECK_EQUAL(success(), emit.issuereward(point_code, cfg::gallery_name));
    BOOST_CHECK_EQUAL(mosaic_amount, point.get_amount(cfg::gallery_name));
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
