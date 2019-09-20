#include "golos_tester.hpp"
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

class commun_emit_tester : public golos_tester {
protected:
    cyber_token_api token;
    commun_point_api point;
    commun_emit_api emit;

public:
    commun_emit_tester()
        : golos_tester(cfg::commun_emit_name)
        , token({this, cfg::token_name, cfg::reserve_token})
        , point({this, cfg::commun_point_name, _point})
        , emit({this, _code})
    {
        create_accounts({_code, _commun, _golos, _alice, _bob, _carol,
            cfg::token_name, cfg::commun_point_name});
        produce_block();
        install_contract(cfg::token_name, contracts::token_wasm(), contracts::token_abi());
        install_contract(cfg::commun_point_name, contracts::point_wasm(), contracts::point_abi());
        install_contract(_code, contracts::emit_wasm(), contracts::emit_abi());

        set_authority(cfg::commun_emit_name, cfg::reward_perm_name, create_code_authority({_code}), "active");
        link_authority(cfg::commun_emit_name, cfg::commun_emit_name, cfg::reward_perm_name, N(issuereward));

        set_authority(cfg::commun_point_name, cfg::issue_permission, create_code_authority({cfg::commun_emit_name}), "active");
        set_authority(cfg::commun_point_name, cfg::transfer_permission, create_code_authority({cfg::commun_emit_name}), "active");
        link_authority(cfg::commun_point_name, cfg::commun_point_name, cfg::issue_permission, N(issue));
        link_authority(cfg::commun_point_name, cfg::commun_point_name, cfg::transfer_permission, N(transfer));
    }

    void init() {
        int64_t reserve = 100000;
        BOOST_CHECK_EQUAL(success(), point.create(_golos, asset(999999999, point._symbol), 10000, 0));

        BOOST_CHECK_EQUAL(success(), token.create(_commun, asset(1000000, token._symbol)));
        BOOST_CHECK_EQUAL(success(), token.issue(_commun, _carol, asset(reserve, token._symbol), ""));
        BOOST_CHECK_EQUAL(success(), token.transfer(_carol, cfg::commun_point_name, asset(reserve, token._symbol), cfg::restock_prefix + point_code_str));
    }

    const account_name _commun = N(commun);
    const account_name _golos = N(golos);
    const account_name _alice = N(alice);
    const account_name _bob = N(bob);
    const account_name _carol = N(carol);
    static constexpr int64_t seconds_per_year = int64_t(365)*24*60*60;

    struct errors: contract_error_messages {
        const string no_point = amsg("point with symbol does not exist");
        const string symbol_precision = amsg("symbol precision mismatch");
        const string already_exists = amsg("already exists");
        const string no_emitter = amsg("emitter does not exists, create it before issue");
        const string too_early_emit = amsg("SYSTEM: untimely claim reward");
        const string wrong_annual_rate = amsg("annual_emission_rate must be between 0.01% and 100% (1-10000)");
        const string wrong_leaders_prop = amsg("leaders_reward_prop must be between 0% and 100% (0-10000)");
        const string no_account(name acc) { return amsg(acc.to_string() +  " contract does not exists"); }
    } err;
};

BOOST_AUTO_TEST_SUITE(commun_emit_tests)

BOOST_FIXTURE_TEST_CASE(create_tests, commun_emit_tester) try {
    BOOST_TEST_MESSAGE("create tests");
    auto fake_sym = symbol(0, point_code_str);

    BOOST_CHECK_EQUAL(err.no_point, emit.create(point._symbol, cfg::_100percent, cfg::_100percent));
    init();
    BOOST_CHECK_EQUAL(err.symbol_precision, emit.create(fake_sym, cfg::_100percent, cfg::_100percent));
    BOOST_CHECK_EQUAL(err.wrong_annual_rate, emit.create(point._symbol, 0, cfg::_100percent));
    BOOST_CHECK_EQUAL(err.wrong_annual_rate, emit.create(point._symbol, cfg::_100percent+1, cfg::_100percent));
    BOOST_CHECK_EQUAL(err.wrong_leaders_prop, emit.create(point._symbol, cfg::_100percent, cfg::_100percent+1));
    BOOST_CHECK_EQUAL(success(), emit.create(point._symbol, cfg::_100percent, 0));
    produce_block();
    BOOST_CHECK_EQUAL(err.already_exists, emit.create(point._symbol, cfg::_100percent, cfg::_100percent));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(issuereward_tests, commun_emit_tester) try {
    BOOST_TEST_MESSAGE("issuereward tests");
    BOOST_CHECK_EQUAL(err.no_emitter, emit.issuereward(point._symbol, false));
    init();
    BOOST_CHECK_EQUAL(success(), emit.create(point._symbol, cfg::_100percent, cfg::_100percent));

    BOOST_CHECK_EQUAL(err.too_early_emit, emit.issuereward(point._symbol, false));
    BOOST_CHECK_EQUAL(err.too_early_emit, emit.issuereward(point._symbol, true));

    BOOST_TEST_MESSAGE("-- waiting for mosaics reward");
    produce_blocks(commun::seconds_to_blocks(cfg::reward_mosaics_period));
    BOOST_CHECK_EQUAL(err.no_account(cfg::commun_gallery_name), emit.issuereward(point._symbol, false));
    create_accounts({cfg::commun_gallery_name});
    BOOST_CHECK_EQUAL(success(), emit.issuereward(point._symbol, false));
    BOOST_CHECK_EQUAL(err.too_early_emit, emit.issuereward(point._symbol, true));

    BOOST_TEST_MESSAGE("-- waiting for leaders reward");
    produce_blocks(commun::seconds_to_blocks(cfg::reward_leaders_period - cfg::reward_mosaics_period));
    BOOST_CHECK_EQUAL(err.no_account(cfg::commun_ctrl_name), emit.issuereward(point._symbol, true));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(basic_tests, commun_emit_tester) try {
    BOOST_TEST_MESSAGE("basic tests");
    create_accounts({cfg::commun_gallery_name});
    init();
    int64_t supply = 100000000;
    double annual_rate = 0.5;
    double leaders_rate = 0.25;
    BOOST_CHECK_EQUAL(success(), emit.create(point._symbol, annual_rate*cfg::_100percent, leaders_rate*cfg::_100percent));
    BOOST_CHECK_EQUAL(success(), point.issue(_golos, _golos, asset(supply, point._symbol), "issue"));
    BOOST_CHECK_EQUAL(success(), point.open(cfg::commun_gallery_name, point._symbol, cfg::commun_gallery_name));

    int64_t cont_emission = supply * int64_t(std::log(1.0 + annual_rate) * cfg::_100percent) / cfg::_100percent;
    int64_t period_amount = cont_emission * cfg::reward_mosaics_period / seconds_per_year;
    int64_t mosaic_amount = period_amount * (1.0 - leaders_rate);
    BOOST_TEST_MESSAGE("-- waiting for mosaics reward");
    produce_blocks(commun::seconds_to_blocks(cfg::reward_mosaics_period));
    BOOST_CHECK_EQUAL(success(), emit.issuereward(point._symbol, false));
    BOOST_CHECK_EQUAL(mosaic_amount, point.get_amount(cfg::commun_gallery_name));
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
