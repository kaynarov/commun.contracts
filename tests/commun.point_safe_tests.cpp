#include "golos_tester.hpp"
#include "commun.point_test_api.hpp"
#include "cyber.token_test_api.hpp"
#include "contracts.hpp"
#include "../commun.point/include/commun.point/config.hpp"


namespace cfg = commun::config;
using namespace eosio::testing;
using namespace eosio::chain;
using namespace fc;
static const auto _point = symbol(3, "GLS");
static const auto _point2 = symbol(3, "TEST");

class commun_point_safe_tester : public golos_tester {
protected:
    cyber_token_api token;
    commun_point_api point;
    commun_point_api point2;

public:
    commun_point_safe_tester()
        : golos_tester(cfg::point_name)
        , token({this, cfg::token_name, cfg::reserve_token})
        , point({this, _code, _point})
        , point2({this, _code, _point2}) {
        create_accounts({_dapp, _gls_com, _tst_com, _alice, _bob, _carol, cfg::token_name, cfg::point_name});
        produce_block();
        install_contract(cfg::token_name, contracts::token_wasm(), contracts::token_abi());
        install_contract(_code, contracts::point_wasm(), contracts::point_abi());
    }

protected:
    const uint32_t _delay = 60;
    double _total = 1000;
    double _supply = 25;
    double _reserve = 100;
    double _fee = 0.01;

public:
    void init() {
        BOOST_CHECK_EQUAL(success(), token.create(_dapp, token.make_asset(_total)));
        BOOST_CHECK_EQUAL(success(), token.issue(_dapp, _carol, token.make_asset(_reserve * 2), ""));

        const auto init_points = [&](commun_point_api& point, name owner, double fee) {
            BOOST_CHECK_EQUAL(success(), point.create(owner, 0, _total, 10000, fee * cfg::_100percent));
            BOOST_CHECK_EQUAL(success(), point.setparams(owner, fee * cfg::_100percent, fee ? 1 : 0));
            BOOST_CHECK_EQUAL(success(),
                token.transfer(_carol, _code, token.make_asset(_reserve), point.restock_memo()));
            BOOST_CHECK_EQUAL(success(), point.issue(owner, _supply));
        };

        init_points(point, _gls_com, 0);
        init_points(point2, _tst_com, _fee);
    }

    const account_name _dapp = cfg::dapp_name;
    const account_name _gls_com = N(golos); // community (points with fee)
    const account_name _tst_com = N(test);  // community (points without fee)
    const account_name _alice = N(alice);
    const account_name _bob = N(bob);
    const account_name _carol = N(carol);
    const account_name _nobody = N(nobody); // not existing account

    struct errors: contract_error_messages {
        const string symbol_precision = amsg("symbol precision mismatch");
        const string unlock_lt0 = amsg("unlock amount must be >= 0");
        const string unlock_lte0 = amsg("unlock amount must be > 0");
        const string lock_lt0 = amsg("lock amount must be >= 0");
        const string unlocked_eq0 = amsg("nothing to lock");
        const string delay_lte0 = amsg("delay must be > 0");
        const string delay_gt_max = amsg("delay must be <= " + std::to_string(cfg::safe_max_delay));
        const string trusted_eq_owner = amsg("trusted and owner must be different accounts");
        const string trusted_not_exists = amsg("trusted account does not exist");
        const string already_enabled = amsg("Safe already enabled");
        const string have_mods = amsg("Can't enable safe with existing delayed mods");
        const string disabled = amsg("Safe disabled");
        const string same_delay = amsg("Can't set same delay");
        const string same_trusted = amsg("Can't set same trusted");
        const string empty_mod_id = amsg("mod_id must not be empty");
        const string have_mod_id = amsg("mod_id must be empty for trusted action");
        const string same_mod_id = amsg("Safe mod with the same id is already exists");
        const string lock_gt_unlocked = amsg("lock must be <= unlocked");
        const string nothing_set = amsg("delay and/or trusted must be set");
        const string mod_not_exists = amsg("Safe mod not found");
        const string still_locked = amsg("Safe change is time locked");
        const string nothing_to_apply = amsg("Change has no effect and can be cancelled");
        const string global_lock = amsg("balance locked in safe");
        const string mod_global_lock = amsg("Safe locked globally");
        const string balance_lock = amsg("overdrawn safe unlocked balance");
        const string balance_over = amsg("overdrawn balance");
        const string unlocked_over = amsg("unlocked overflow");
        const string period_le0 = amsg("period must be > 0");
        const string period_gt_max = amsg("period must be <= " + std::to_string(cfg::safe_max_delay));
        const string period_le_cur = amsg("new unlock time must be greater than current");
    } err;
};

BOOST_AUTO_TEST_SUITE(commun_point_safe)

BOOST_FIXTURE_TEST_CASE(enable, commun_point_safe_tester) try {
    BOOST_TEST_MESSAGE("Enable the safe");
    init();
    BOOST_CHECK(point.get_safe(_bob).is_null());
    BOOST_TEST_MESSAGE("--- fail on bad params");
    BOOST_CHECK_EQUAL(err.unlock_lt0, point.enable_safe(_bob, -1, 0));
    BOOST_CHECK_EQUAL(err.unlock_lt0, point.enable_safe(_bob, -point.satoshi(), 0));
    BOOST_CHECK_EQUAL(err.delay_lte0, point.enable_safe(_bob, 0, 0));
    BOOST_CHECK_EQUAL(err.delay_gt_max, point.enable_safe(_bob, 0, cfg::safe_max_delay + 1));
    BOOST_CHECK_EQUAL(err.trusted_eq_owner, point.enable_safe(_bob, 0, _delay, _bob));
    BOOST_CHECK_EQUAL(err.trusted_not_exists, point.enable_safe(_bob, 0, _delay, _nobody));
    const asset bad_asset{0, point.bad_sym()};
    BOOST_CHECK_EQUAL(err.symbol_precision, point.enable_safe(_bob, bad_asset, _delay));

    BOOST_TEST_MESSAGE("--- success on valid params");
    BOOST_CHECK_EQUAL(success(), point.enable_safe(_bob, 0, _delay));
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(0, _delay));
    produce_block();

    BOOST_TEST_MESSAGE("--- fail if already created");
    BOOST_CHECK_EQUAL(err.already_enabled, point.enable_safe(_bob, 0, _delay));
    BOOST_CHECK_EQUAL(err.already_enabled, point.enable_safe(_bob, 100500, cfg::safe_max_delay));

    BOOST_TEST_MESSAGE("--- success when create for different points with over-limit and max delay");
    BOOST_CHECK_EQUAL(success(), point2.enable_safe(_bob, 100500, cfg::safe_max_delay));
    CHECK_MATCHING_OBJECT(point2.get_safe(_bob), point2.make_safe(100500, cfg::safe_max_delay));

    // enable_safe with existing delayed mods checked in "disable safe test"
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(disable, commun_point_safe_tester) try {
    BOOST_TEST_MESSAGE("Disable the safe");
    const auto mod_id = N(mod);
    init();

    BOOST_TEST_MESSAGE("--- fail if not enabled");
    BOOST_CHECK_EQUAL(err.disabled, point.disable_safe(_bob, {}));
    BOOST_CHECK_EQUAL(err.disabled, point.disable_safe(_bob, mod_id));

    BOOST_TEST_MESSAGE("--- enable for " << point.name());
    BOOST_CHECK_EQUAL(success(), point.enable_safe(_bob, 0, _delay));
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(0, _delay));
    produce_block();

    BOOST_TEST_MESSAGE("--- still fail for " << point2.name());
    BOOST_CHECK_EQUAL(err.disabled, point2.disable_safe(_bob, {}));
    BOOST_CHECK_EQUAL(err.disabled, point2.disable_safe(_bob, mod_id));

    BOOST_TEST_MESSAGE("--- fail on bad params");
    BOOST_CHECK_EQUAL(err.empty_mod_id, point.disable_safe(_bob, {}));

    BOOST_TEST_MESSAGE("--- success on valid params");
    BOOST_CHECK_EQUAL(success(), point.disable_safe(_bob, mod_id));
    CHECK_MATCHING_OBJECT(point.get_safe_mod(_bob, mod_id), point.make_safe_mod(mod_id, 0, 0));
    BOOST_CHECK_EQUAL(err.still_locked, point.apply_safe_mod(_bob, mod_id));
    produce_block();

    BOOST_TEST_MESSAGE("--- fail if same mod_id and success if other");
    const auto mod2_id = N(mod2);
    BOOST_CHECK_EQUAL(success(), point.disable_safe(_bob, mod2_id));
    produce_block();
    BOOST_CHECK_EQUAL(err.same_mod_id, point.disable_safe(_bob, mod_id));
    BOOST_CHECK_EQUAL(err.same_mod_id, point.disable_safe(_bob, mod2_id));

    BOOST_TEST_MESSAGE("--- wait and disable");
    const auto blocks = commun::seconds_to_blocks(_delay);
    produce_blocks(blocks - 1 - 2); // 2 for two produce_block() calls
    BOOST_CHECK_EQUAL(err.still_locked, point.apply_safe_mod(_bob, mod_id));
    produce_block();
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod(_bob, mod_id));
    BOOST_CHECK_EQUAL(err.disabled, point.disable_safe(_bob, mod_id));
    BOOST_CHECK(point.get_safe(_bob).is_null());
    BOOST_CHECK(point.get_safe_mod(_bob, mod_id).is_null());

    BOOST_TEST_MESSAGE("--- fail on re-enabling with existing mod");
    BOOST_CHECK_EQUAL(err.have_mods, point.enable_safe(_bob, 0, _delay));

    BOOST_TEST_MESSAGE("--- success when existing mod is for other symbol");
    BOOST_CHECK_EQUAL(success(), point2.enable_safe(_bob, 0, _delay));

    BOOST_TEST_MESSAGE("--- success when no more mods");
    BOOST_CHECK_EQUAL(success(), point.cancel_safe_mod(_bob, mod2_id));
    BOOST_CHECK_EQUAL(success(), point.enable_safe(_bob, 0, _delay));
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(0, _delay));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(lock_unlock, commun_point_safe_tester) try {
    BOOST_TEST_MESSAGE("Unlock and lock the safe");
    init();

    BOOST_TEST_MESSAGE("--- fail if not enabled");
    const double one = 1;
    const auto mod_id = N(mod);
    BOOST_CHECK_EQUAL(err.disabled, point.unlock_safe(_bob, {}, one));
    BOOST_CHECK_EQUAL(err.disabled, point.unlock_safe(_bob, mod_id, one));
    BOOST_CHECK_EQUAL(err.disabled, point.lock_safe(_bob, one));

    BOOST_TEST_MESSAGE("--- fail if negative amount");
    BOOST_CHECK_EQUAL(err.unlock_lte0, point.unlock_safe(_bob, {}, -1));
    BOOST_CHECK_EQUAL(err.unlock_lte0, point.unlock_safe(_bob, mod_id, -1));
    BOOST_CHECK_EQUAL(err.lock_lt0, point.lock_safe(_bob, -1));
    BOOST_TEST_MESSAGE("--- fail if unlock with wrong precision");
    const asset bad{1, point.bad_sym()};
    BOOST_CHECK_EQUAL(err.symbol_precision, point.unlock_safe(_bob, {}, bad));
    BOOST_CHECK_EQUAL(err.symbol_precision, point.unlock_safe(_bob, mod_id, bad));
    BOOST_CHECK_EQUAL(err.symbol_precision, point.lock_safe(_bob, bad));
    BOOST_CHECK_EQUAL(err.symbol_precision, point.lock_safe(_bob, asset{1, point.bad_sym(-1)}));

    BOOST_TEST_MESSAGE("--- enable 1 point unlocked for " << point.name());
    BOOST_CHECK_EQUAL(success(), point.enable_safe(_bob, one, _delay));
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(one, _delay));
    BOOST_TEST_MESSAGE("--- enable full locked for " << point2.name());
    BOOST_CHECK_EQUAL(success(), point2.enable_safe(_bob, 0, _delay));
    produce_block();

    BOOST_TEST_MESSAGE("--- fail if unlock with empty id");
    BOOST_CHECK_EQUAL(err.empty_mod_id, point.unlock_safe(_bob, {}, one));
    BOOST_TEST_MESSAGE("--- fail if lock too much");
    const auto satoshi = point.satoshi();
    BOOST_CHECK_EQUAL(err.lock_gt_unlocked, point.lock_safe(_bob, one + satoshi));
    BOOST_CHECK_EQUAL(err.unlocked_eq0, point2.lock_safe(_bob, point2.satoshi()));

    BOOST_TEST_MESSAGE("--- success if lock satoshi");
    BOOST_CHECK_EQUAL(success(), point.lock_safe(_bob, satoshi));
    BOOST_TEST_MESSAGE("--- success if lock half");
    const auto half = one / 2;
    BOOST_CHECK_EQUAL(success(), point.lock_safe(_bob, half));
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(one - half - satoshi, _delay));
    produce_block();
    BOOST_CHECK_EQUAL(err.lock_gt_unlocked, point.lock_safe(_bob, half));
    BOOST_TEST_MESSAGE("--- success if lock remainig");
    BOOST_CHECK_EQUAL(success(), point.lock_safe(_bob, half - satoshi));
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(0, _delay));
    BOOST_TEST_MESSAGE("--- fail if lock one more satoshi");
    BOOST_CHECK_EQUAL(err.unlocked_eq0, point.lock_safe(_bob));
    BOOST_CHECK_EQUAL(err.unlocked_eq0, point.lock_safe(_bob, satoshi));
    BOOST_CHECK_EQUAL(err.unlocked_eq0, point2.lock_safe(_bob));

    BOOST_TEST_MESSAGE("--- success if schedule valid unlock");
    const auto mod2_id = N(mod2);
    BOOST_CHECK(point.get_safe_mod(_bob, mod_id).is_null());
    BOOST_CHECK(point.get_safe_mod(_bob, mod2_id).is_null());
    BOOST_CHECK_EQUAL(success(), point.unlock_safe(_bob, mod_id, one));
    BOOST_CHECK_EQUAL(success(), point.unlock_safe(_bob, mod2_id, one));
    CHECK_MATCHING_OBJECT(point.get_safe_mod(_bob, mod_id), point.make_safe_mod(mod_id, one));
    CHECK_MATCHING_OBJECT(point.get_safe_mod(_bob, mod2_id), point.make_safe_mod(mod2_id, one));
    BOOST_CHECK_EQUAL(err.same_mod_id, point.unlock_safe(_bob, mod_id, half));

    BOOST_TEST_MESSAGE("--- fail to apply");
    BOOST_CHECK_EQUAL(err.still_locked, point.apply_safe_mod(_bob, mod_id));
    BOOST_TEST_MESSAGE("--- wait half time and add one more unlock");
    const auto blocks = commun::seconds_to_blocks(_delay);
    const auto half_blocks = blocks/2;
    produce_blocks(half_blocks);
    BOOST_CHECK_EQUAL(err.still_locked, point.apply_safe_mod(_bob, mod_id));
    const auto fat_id = N(fatmod);
    BOOST_CHECK_EQUAL(success(), point.unlock_safe(_bob, fat_id, 100500));

    BOOST_TEST_MESSAGE("--- wait to 1 block before 1st unlocks end ensure it's locked");
    const auto remaining_blocks = blocks - half_blocks;
    produce_blocks(remaining_blocks - 1);
    BOOST_CHECK_EQUAL(err.still_locked, point.apply_safe_mod(_bob, mod_id));
    BOOST_TEST_MESSAGE("--- wait 1 more block and unlock");
    produce_block();
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod(_bob, mod_id));
    BOOST_CHECK_EQUAL(err.still_locked, point.apply_safe_mod(_bob, fat_id));
    BOOST_CHECK(point.get_safe_mod(_bob, mod_id).is_null());
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(one, _delay));

    BOOST_TEST_MESSAGE("--- wait fat unlock time and check both fat and mod2 applied");
    produce_blocks(blocks - remaining_blocks - 1);
    BOOST_CHECK_EQUAL(err.still_locked, point.apply_safe_mod(_bob, fat_id));
    produce_block();
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod(_bob, fat_id));
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod(_bob, mod2_id));
    BOOST_CHECK(point.get_safe_mod(_bob, fat_id).is_null());
    BOOST_CHECK(point.get_safe_mod(_bob, mod2_id).is_null());
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(one+one+100500, _delay));

    BOOST_TEST_MESSAGE("--- lock all");
    BOOST_CHECK_EQUAL(success(), point.lock_safe(_bob));
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(0, _delay));
    produce_block();
    BOOST_CHECK_EQUAL(err.unlocked_eq0, point.lock_safe(_bob));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(modify, commun_point_safe_tester) try {
    BOOST_TEST_MESSAGE("Modify the safe");
    init();
    const auto mod_id = N(mod);
    const auto delay1 = _delay / 2;
    const auto delay2 = _delay * 2;

    BOOST_TEST_MESSAGE("--- fail on bad params");
    BOOST_CHECK_EQUAL(err.nothing_set, point.modify_safe(_bob, mod_id));
    BOOST_CHECK_EQUAL(err.delay_lte0, point.modify_safe(_bob, mod_id, 0));
    BOOST_CHECK_EQUAL(err.delay_gt_max, point.modify_safe(_bob, mod_id, cfg::safe_max_delay + 1));
    BOOST_CHECK_EQUAL(err.trusted_eq_owner, point.modify_safe(_bob, mod_id, {}, _bob));
    BOOST_CHECK_EQUAL(err.trusted_not_exists, point.modify_safe(_bob, mod_id, {}, _nobody));

    BOOST_TEST_MESSAGE("--- fail if not enabled");
    BOOST_CHECK_EQUAL(err.disabled, point.modify_safe(_bob, mod_id, delay1));
    BOOST_CHECK_EQUAL(err.disabled, point.modify_safe(_bob, mod_id, delay1, _alice));
    BOOST_CHECK_EQUAL(err.disabled, point.modify_safe(_bob, mod_id, {}, _alice));

    BOOST_TEST_MESSAGE("--- enable full locked for " << point.name());
    BOOST_CHECK_EQUAL(success(), point.enable_safe(_bob, 0, _delay, _alice));
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(0, _delay, _alice));
    produce_block();

    BOOST_TEST_MESSAGE("--- success with enabled and correct params");
    BOOST_CHECK_EQUAL(success(), point.modify_safe(_bob, mod_id, _delay, _alice));
    CHECK_MATCHING_OBJECT(point.get_safe_mod(_bob, mod_id), point.make_safe_mod(mod_id, 0, _delay, _alice));

    BOOST_TEST_MESSAGE("--- prepare mods to check all change cases");
    BOOST_TEST_MESSAGE("------ 1 param: same delay, same trusted, other delay, other trusted");
    const auto mod_delay0 = N(mod.delay);
    const auto mod_delay1 = N(mod.delay1);
    const auto mod_delay2 = N(mod.delay2);
    const auto mod_trust1 = N(mod.trust1);
    const auto mod_trust2 = N(mod.trust2);
    BOOST_CHECK_EQUAL(success(), point.modify_safe(_bob, mod_delay0, _delay));
    BOOST_CHECK_EQUAL(success(), point.modify_safe(_bob, mod_delay1, delay1));
    BOOST_CHECK_EQUAL(success(), point.modify_safe(_bob, mod_delay2, delay2));
    BOOST_CHECK_EQUAL(success(), point.modify_safe(_bob, mod_trust1, {}, _alice));
    BOOST_CHECK_EQUAL(success(), point.modify_safe(_bob, mod_trust2, {}, _carol));
    CHECK_MATCHING_OBJECT(point.get_safe_mod(_bob, mod_delay0), point.make_safe_mod(mod_delay0, 0, _delay));
    CHECK_MATCHING_OBJECT(point.get_safe_mod(_bob, mod_delay1), point.make_safe_mod(mod_delay1, 0, delay1));
    CHECK_MATCHING_OBJECT(point.get_safe_mod(_bob, mod_delay2), point.make_safe_mod(mod_delay2, 0, delay2));
    CHECK_MATCHING_OBJECT(point.get_safe_mod(_bob, mod_trust1), point.make_safe_mod(mod_trust1, 0, {}, _alice));
    CHECK_MATCHING_OBJECT(point.get_safe_mod(_bob, mod_trust2), point.make_safe_mod(mod_trust2, 0, {}, _carol));
    BOOST_TEST_MESSAGE("------ 2 params: same both, same delay, same trusted, other both");
    const auto mod_d2t2 = N(mod.d2t2);
    const auto mod_d1t2 = N(mod.d1t2);
    const auto mod_d1t1 = N(mod.d1t1);
    const auto mod_dmt2 = N(mod.dmt2);
    BOOST_CHECK_EQUAL(success(), point.modify_safe(_bob, mod_d2t2, delay2, _carol));
    BOOST_CHECK_EQUAL(success(), point.modify_safe(_bob, mod_d1t2, _delay, _carol));
    BOOST_CHECK_EQUAL(success(), point.modify_safe(_bob, mod_d1t1, _delay, _alice));
    BOOST_CHECK_EQUAL(success(), point.modify_safe(_bob, mod_dmt2, cfg::safe_max_delay, _carol));
    CHECK_MATCHING_OBJECT(point.get_safe_mod(_bob, mod_d2t2), point.make_safe_mod(mod_d2t2, 0, delay2, _carol));
    CHECK_MATCHING_OBJECT(point.get_safe_mod(_bob, mod_dmt2), point.make_safe_mod(mod_dmt2, 0, cfg::safe_max_delay, _carol));

    BOOST_TEST_MESSAGE("--- Wait 1 block before unlock time");
    auto blocks = commun::seconds_to_blocks(_delay);
    produce_blocks(blocks - 1);
    BOOST_CHECK_EQUAL(err.still_locked, point.apply_safe_mod(_bob, mod_id));
    BOOST_CHECK_EQUAL(err.same_mod_id, point.modify_safe(_bob, mod_id, _delay));

    BOOST_TEST_MESSAGE("--- Wait 1 more block and check changes");
    produce_block();
    BOOST_TEST_MESSAGE("------ fail if 2 params are the same as in current safe");
    BOOST_CHECK_EQUAL(err.nothing_to_apply, point.apply_safe_mod(_bob, mod_id));
    BOOST_TEST_MESSAGE("------ fail if 1 param and same delay");
    BOOST_CHECK_EQUAL(err.nothing_to_apply, point.apply_safe_mod(_bob, mod_delay0));
    BOOST_TEST_MESSAGE("------ fail if 1 param asd same trusted");
    BOOST_CHECK_EQUAL(err.nothing_to_apply, point.apply_safe_mod(_bob, mod_trust1));
    BOOST_TEST_MESSAGE("------ success if 1 param differs from value in current safe");
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod(_bob, mod_delay2));
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(0, delay2, _alice));
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod(_bob, mod_trust2));
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(0, delay2, _carol));
    BOOST_CHECK(point.get_safe_mod(_bob, mod_delay2).is_null());
    BOOST_CHECK(point.get_safe_mod(_bob, mod_trust2).is_null());
    BOOST_TEST_MESSAGE("------ fail if 2 params are the same as in current safe");
    BOOST_CHECK_EQUAL(err.nothing_to_apply, point.apply_safe_mod(_bob, mod_d2t2));
    BOOST_TEST_MESSAGE("------ success if 1 of 2 params differs from value in current safe");
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod(_bob, mod_d1t2));
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(0, _delay, _carol));
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod(_bob, mod_d1t1));
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(0, _delay, _alice));
    BOOST_TEST_MESSAGE("------ success if both params differ");
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod(_bob, mod_dmt2));
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(0, cfg::safe_max_delay, _carol));
    BOOST_CHECK(point.get_safe_mod(_bob, mod_dmt2).is_null());

    BOOST_TEST_MESSAGE("--- reduce delay");
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod(_bob, mod_delay1));
    BOOST_TEST_MESSAGE("--- reuse old mod id to create new mod and ensure it's applied at time");
    BOOST_CHECK_EQUAL(success(), point.modify_safe(_bob, mod_delay1, delay2));
    const auto blocks2 = commun::seconds_to_blocks(delay1);
    produce_blocks(blocks2 - 1);
    BOOST_CHECK_EQUAL(err.still_locked, point.apply_safe_mod(_bob, mod_delay1));
    produce_block();
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod(_bob, mod_delay1));

    BOOST_TEST_MESSAGE("--- apply mod with increased delay and check it's applied at time");
    BOOST_CHECK_EQUAL(success(), point.modify_safe(_bob, mod_delay1, _delay, name{}));
    const auto blocks3 = commun::seconds_to_blocks(delay2);
    produce_blocks(blocks3 - 1);
    BOOST_CHECK_EQUAL(err.still_locked, point.apply_safe_mod(_bob, mod_delay1));
    produce_block();
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod(_bob, mod_delay1));
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(0, _delay));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(apply_cancel, commun_point_safe_tester) try {
    BOOST_TEST_MESSAGE("Apply/cancel safe mod");
    init();
    const auto modify_mod = N(mod);
    const auto unlock_mod = N(unlock);
    const auto killit_mod = N(.k..i...ll); // test exotic name

    const auto create_mods = [&]() {
        // create all mod types
        BOOST_CHECK_EQUAL(success(), point.unlock_safe(_bob, unlock_mod, 1));
        BOOST_CHECK_EQUAL(success(), point.modify_safe(_bob, modify_mod, _delay, _carol));
        BOOST_CHECK_EQUAL(success(), point.disable_safe(_bob, killit_mod));
        CHECK_MATCHING_OBJECT(point.get_safe_mod(_bob, unlock_mod), point.make_safe_mod(unlock_mod, 1));
        CHECK_MATCHING_OBJECT(point.get_safe_mod(_bob, modify_mod), point.make_safe_mod(modify_mod, 0, _delay, _carol));
        CHECK_MATCHING_OBJECT(point.get_safe_mod(_bob, killit_mod), point.make_safe_mod(killit_mod, 0, 0));
    };

    BOOST_TEST_MESSAGE("--- fail on non-existing mods");
    BOOST_CHECK_EQUAL(err.mod_not_exists, point.apply_safe_mod(_bob, _bob));
    BOOST_CHECK_EQUAL(err.mod_not_exists, point.cancel_safe_mod(_bob, _bob));

    BOOST_CHECK_EQUAL(success(), point.enable_safe(_bob, 0, _delay, _alice));
    create_mods();
    BOOST_CHECK_EQUAL(err.mod_not_exists, point.apply_safe_mod(_bob, _bob));
    BOOST_CHECK_EQUAL(err.mod_not_exists, point.cancel_safe_mod(_bob, _bob));

    BOOST_TEST_MESSAGE("--- success on cancel existing");
    BOOST_CHECK_EQUAL(success(), point.cancel_safe_mod(_bob, unlock_mod));
    BOOST_CHECK_EQUAL(success(), point.cancel_safe_mod(_bob, modify_mod));
    BOOST_CHECK_EQUAL(success(), point.cancel_safe_mod(_bob, killit_mod));
    BOOST_CHECK(point.get_safe_mod(_bob, unlock_mod).is_null());
    BOOST_CHECK(point.get_safe_mod(_bob, modify_mod).is_null());
    BOOST_CHECK(point.get_safe_mod(_bob, killit_mod).is_null());
    produce_block();

    BOOST_TEST_MESSAGE("--- success on trusted apply");
    create_mods();
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod2(_bob, unlock_mod, _alice));
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod2(_bob, modify_mod, _alice));
    BOOST_CHECK_EQUAL(err.still_locked, point.apply_safe_mod2(_bob, killit_mod, _alice));
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod2(_bob, killit_mod, _carol));
    BOOST_CHECK(point.get_safe_mod(_bob, unlock_mod).is_null());
    BOOST_CHECK(point.get_safe_mod(_bob, modify_mod).is_null());
    BOOST_CHECK(point.get_safe_mod(_bob, killit_mod).is_null());
    produce_block();
    BOOST_CHECK_EQUAL(success(), point.enable_safe(_bob, 0, _delay));

    BOOST_TEST_MESSAGE("--- success on delayed apply");
    create_mods();
    const auto blocks = commun::seconds_to_blocks(_delay);
    produce_blocks(blocks - 1);
    BOOST_CHECK_EQUAL(err.still_locked, point.apply_safe_mod(_bob, unlock_mod));
    BOOST_CHECK_EQUAL(err.still_locked, point.apply_safe_mod(_bob, modify_mod));
    BOOST_CHECK_EQUAL(err.still_locked, point.apply_safe_mod(_bob, killit_mod));
    produce_block();
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod(_bob, unlock_mod));
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod(_bob, modify_mod));
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod(_bob, killit_mod));
    BOOST_CHECK(point.get_safe_mod(_bob, unlock_mod).is_null());
    BOOST_CHECK(point.get_safe_mod(_bob, modify_mod).is_null());
    BOOST_CHECK(point.get_safe_mod(_bob, killit_mod).is_null());
    produce_block();

    BOOST_TEST_MESSAGE("--- fail apply on disabled safe");
    BOOST_CHECK_EQUAL(success(), point.enable_safe(_bob, 0, _delay, _alice));
    produce_block();
    create_mods();
    BOOST_CHECK_EQUAL(success(), point.disable_safe2(_bob, _alice));
    const auto try_apply_all = [&]() {
        BOOST_CHECK_EQUAL(err.disabled, point.apply_safe_mod(_bob, unlock_mod));
        BOOST_CHECK_EQUAL(err.disabled, point.apply_safe_mod(_bob, modify_mod));
        BOOST_CHECK_EQUAL(err.disabled, point.apply_safe_mod(_bob, killit_mod));
        BOOST_CHECK_EQUAL(err.disabled, point.apply_safe_mod2(_bob, unlock_mod, _alice));
        BOOST_CHECK_EQUAL(err.disabled, point.apply_safe_mod2(_bob, modify_mod, _alice));
        BOOST_CHECK_EQUAL(err.disabled, point.apply_safe_mod2(_bob, killit_mod, _alice));
    };
    try_apply_all();
    BOOST_TEST_MESSAGE("--- fail apply on disabled safe after delay");
    produce_blocks(blocks);
    try_apply_all();
    BOOST_TEST_MESSAGE("--- success cancel on disabled safe");
    BOOST_CHECK_EQUAL(success(), point.cancel_safe_mod(_bob, unlock_mod));
    BOOST_CHECK_EQUAL(success(), point.cancel_safe_mod(_bob, modify_mod));
    BOOST_CHECK_EQUAL(success(), point.cancel_safe_mod(_bob, killit_mod));
    BOOST_CHECK(point.get_safe_mod(_bob, unlock_mod).is_null());
    BOOST_CHECK(point.get_safe_mod(_bob, modify_mod).is_null());
    BOOST_CHECK(point.get_safe_mod(_bob, killit_mod).is_null());

    // change params requirements on apply already tested in "modify"
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(trusted, commun_point_safe_tester) try {
    BOOST_TEST_MESSAGE("Instant actions with trusted account");
    init();
    const auto mod_id = N(mod);

    BOOST_CHECK_EQUAL(success(), point.enable_safe(_bob, 0, _delay, _alice));
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(0, _delay, _alice));
    BOOST_TEST_MESSAGE("--- fail if have mod_id");
    BOOST_CHECK_EQUAL(err.have_mod_id, point.unlock_safe2(_bob, 1, _alice, mod_id));
    BOOST_CHECK_EQUAL(err.have_mod_id, point.modify_safe2(_bob, _delay + 1, {}, _alice, mod_id));
    BOOST_CHECK_EQUAL(err.have_mod_id, point.disable_safe2(_bob, _alice, mod_id));
    BOOST_TEST_MESSAGE("--- fail if bad unlock");
    BOOST_CHECK_EQUAL(err.symbol_precision, point.unlock_safe2(_bob, asset(1, point.bad_sym()), _alice));
    BOOST_CHECK_EQUAL(err.unlock_lte0, point.unlock_safe2(_bob, 0, _alice));
    BOOST_CHECK_EQUAL(err.unlock_lte0, point.unlock_safe2(_bob, -1, _alice));
    BOOST_TEST_MESSAGE("--- fail if not changed modify");
    BOOST_CHECK_EQUAL(err.nothing_set, point.modify_safe2(_bob, {}, {}, _alice));
    BOOST_CHECK_EQUAL(err.same_trusted, point.modify_safe2(_bob, {}, _alice, _alice));
    BOOST_CHECK_EQUAL(err.same_delay, point.modify_safe2(_bob, _delay, {}, _alice));
    BOOST_CHECK_EQUAL(err.same_delay, point.modify_safe2(_bob, _delay, _alice, _alice));
    BOOST_TEST_MESSAGE("--- fail if bad delay");
    BOOST_CHECK_EQUAL(err.delay_lte0, point.modify_safe2(_bob, 0, {}, _alice));
    BOOST_CHECK_EQUAL(err.delay_lte0, point.modify_safe2(_bob, 0, _carol, _alice));
    BOOST_CHECK_EQUAL(err.delay_gt_max, point.modify_safe2(_bob, cfg::safe_max_delay+1, {}, _alice));
    BOOST_CHECK_EQUAL(err.delay_gt_max, point.modify_safe2(_bob, cfg::safe_max_delay+1, _carol, _alice));
    BOOST_TEST_MESSAGE("--- fail if bad trusted");
    BOOST_CHECK_EQUAL(err.trusted_eq_owner, point.modify_safe2(_bob, {}, _bob, _alice));
    BOOST_CHECK_EQUAL(err.trusted_eq_owner, point.modify_safe2(_bob, 60, _bob, _alice));
    BOOST_CHECK_EQUAL(err.trusted_not_exists, point.modify_safe2(_bob, {}, _nobody, _alice));
    BOOST_CHECK_EQUAL(err.trusted_not_exists, point.modify_safe2(_bob, 60, _nobody, _alice));
    BOOST_TEST_MESSAGE("--- fail if bad delay and trusted");
    BOOST_CHECK_EQUAL(err.delay_lte0, point.modify_safe2(_bob, 0, _bob, _alice));
    BOOST_CHECK_EQUAL(err.delay_lte0, point.modify_safe2(_bob, 0, _nobody, _alice));
    BOOST_CHECK_EQUAL(err.delay_gt_max, point.modify_safe2(_bob, cfg::safe_max_delay+1, _bob, _alice));
    BOOST_CHECK_EQUAL(err.delay_gt_max, point.modify_safe2(_bob, cfg::safe_max_delay+1, _nobody, _alice));

    BOOST_TEST_MESSAGE("--- success on unlock");
    BOOST_CHECK_EQUAL(success(), point.unlock_safe2(_bob, 1, _alice));
    double unlocked = 1;
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(unlocked, _delay, _alice));
    BOOST_CHECK_EQUAL(success(), point.unlock_safe2(_bob, 100500, _alice));
    unlocked += 100500;
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(unlocked, _delay, _alice));
    BOOST_TEST_MESSAGE("--- success on modify");
    BOOST_CHECK_EQUAL(success(), point.modify_safe2(_bob, _delay*2, {}, _alice));
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(unlocked, _delay*2, _alice));
    BOOST_CHECK_EQUAL(success(), point.modify_safe2(_bob, cfg::safe_max_delay, {}, _alice));
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(unlocked, cfg::safe_max_delay, _alice));
    BOOST_CHECK_EQUAL(success(), point.modify_safe2(_bob, {}, _carol, _alice));
    BOOST_CHECK_EQUAL(err.empty_mod_id, point.modify_safe2(_bob, _delay, _alice, _alice));
    BOOST_CHECK_EQUAL(success(), point.modify_safe2(_bob, _delay, _alice, _carol));
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(unlocked, _delay, _alice));
    BOOST_TEST_MESSAGE("--- success on disable");
    BOOST_CHECK_EQUAL(success(), point.disable_safe2(_bob, _alice));
    BOOST_CHECK(point.get_safe(_bob).is_null());

    BOOST_TEST_MESSAGE("--- remove trusted");
    produce_block();
    BOOST_CHECK_EQUAL(success(), point.enable_safe(_bob, 0, _delay, _alice));
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(0, _delay, _alice));
    BOOST_CHECK_EQUAL(success(), point.modify_safe2(_bob, {}, name{}, _alice));
    CHECK_MATCHING_OBJECT(point.get_safe(_bob), point.make_safe(0, _delay));
    BOOST_CHECK_EQUAL(err.empty_mod_id, point.modify_safe2(_bob, {}, _alice, _alice));
    BOOST_CHECK_EQUAL(err.empty_mod_id, point.unlock_safe2(_bob, 1, _alice));
    BOOST_CHECK_EQUAL(err.empty_mod_id, point.disable_safe2(_bob, _alice));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(safe, commun_point_safe_tester) try {
    BOOST_TEST_MESSAGE("Limit transfers with enabled safe");
    init();
    const auto mod_id = N(mod);
    const double money = 100;
    BOOST_CHECK_EQUAL(success(), point.issue(_gls_com, 2*money));
    BOOST_CHECK_EQUAL(success(), point2.issue(_tst_com, 2*money));
    BOOST_CHECK_EQUAL(success(), point.transfer(_gls_com, _alice, money));
    BOOST_CHECK_EQUAL(success(), point.transfer(_gls_com, _bob, money));
    BOOST_CHECK_EQUAL(success(), point2.transfer(_tst_com, _alice, money));
    BOOST_CHECK_EQUAL(success(), point2.transfer(_tst_com, _bob, money));
    auto alices = money;

    const auto showBalances = [&]() {
        BOOST_TEST_MESSAGE("------ alices safe: " <<
            fc::json::to_string(point.get_safe(_alice)) << " / " << fc::json::to_string(point2.get_safe(_alice)));
        BOOST_TEST_MESSAGE("------ alices balance: " <<
            fc::json::to_string(point.get_account(_alice)) << " / " << fc::json::to_string(point2.get_account(_alice)));
        BOOST_TEST_MESSAGE("------ bobs balance: " <<
            fc::json::to_string(point.get_account(_bob)) << " / " << fc::json::to_string(point2.get_account(_bob)));
    };
    showBalances();

    BOOST_TEST_MESSAGE("--- transfer without safe");
    const double half = money / 2;
    alices -= half;
    BOOST_CHECK_EQUAL(success(), point.transfer(_alice, _bob, half));
    BOOST_CHECK_EQUAL(success(), point2.transfer(_alice, _bob, half));
    showBalances();

    BOOST_TEST_MESSAGE("--- alice enables safe for " << point.name());
    BOOST_CHECK_EQUAL(success(), point.enable_safe(_alice, 0, _delay, _bob));

    BOOST_TEST_MESSAGE("--- transfer out blocked by the safe");
    const double tenth = money / 10;
    const auto satoshi = point.satoshi();
    BOOST_CHECK_EQUAL(err.balance_lock, point.transfer(_alice, _bob, tenth));
    BOOST_CHECK_EQUAL(err.balance_lock, point.transfer(_alice, _bob, satoshi));
    BOOST_CHECK_EQUAL(err.balance_lock, point.retire(_alice, tenth));
    BOOST_CHECK_EQUAL(err.balance_lock, point.retire(_alice, satoshi));
    BOOST_CHECK_EQUAL(success(), point2.transfer(_alice, _bob, tenth));
    showBalances();

    BOOST_TEST_MESSAGE("--- transfer in succeed");
    alices += tenth;
    BOOST_CHECK_EQUAL(success(), point.transfer(_bob, _alice, tenth));
    BOOST_CHECK_EQUAL(success(), point2.transfer(_bob, _alice, tenth));
    showBalances();

    BOOST_TEST_MESSAGE("--- prepare unlocked points");
    BOOST_CHECK_EQUAL(success(), point.unlock_safe2(_alice, tenth, _bob));
    BOOST_CHECK_EQUAL(success(), point2.enable_safe(_alice, tenth, _delay, _bob));
    produce_block();

    showBalances();
    BOOST_TEST_MESSAGE("--- fail to transfer because of fee");
    BOOST_CHECK_EQUAL(err.balance_lock, point.transfer(_alice, _bob, tenth + satoshi));
    BOOST_CHECK_EQUAL(err.balance_lock, point2.transfer(_alice, _bob, tenth));
    const auto magic = 10; // not important for this test, so use magic number (hard to calculate precisely because of rounding)
    const auto satoshi2 = point2.satoshi();
    const auto max_allowed = (tenth + satoshi2*magic) / (1 + _fee);
    BOOST_CHECK_EQUAL(err.balance_lock, point2.transfer(_alice, _bob, max_allowed + satoshi2));

    BOOST_TEST_MESSAGE("--- success when transfer unlocked");
    alices -= tenth;
    BOOST_CHECK_EQUAL(success(), point.transfer(_alice, _bob, tenth));
    const auto one = tenth/10;
    BOOST_CHECK_EQUAL(success(), point2.transfer(_alice, _bob, one));
    BOOST_CHECK_EQUAL(success(), point2.transfer(_alice, _bob, max_allowed - one * (1 + _fee)));
    showBalances();
    BOOST_TEST_MESSAGE("--- fail when out of unlocked");
    BOOST_CHECK_EQUAL(err.balance_lock, point.transfer(_alice, _bob, satoshi));
    BOOST_CHECK_EQUAL(err.balance_lock, point2.transfer(_alice, _bob, satoshi2));
    BOOST_CHECK_EQUAL(err.balance_lock, point.retire(_alice, satoshi));
    BOOST_CHECK_EQUAL(err.balance_lock, point2.retire(_alice, satoshi2));

    BOOST_TEST_MESSAGE("--- fail when out of balance with enough unlocked");
    BOOST_CHECK_EQUAL(success(), point.unlock_safe2(_alice, money*100, _bob));
    BOOST_CHECK_EQUAL(err.balance_over, point.transfer(_alice, _bob, money));
    BOOST_CHECK_EQUAL(err.balance_over, point.transfer(_alice, _bob, alices + satoshi));
    BOOST_CHECK_EQUAL(err.balance_over, point.retire(_alice, alices + satoshi));
    BOOST_CHECK_EQUAL(success(), point.transfer(_alice, _bob, alices));
    showBalances();
    produce_block();

    BOOST_TEST_MESSAGE("--- unlock and then lock");
    BOOST_CHECK_EQUAL(success(), point.transfer(_bob, _alice, half));
    BOOST_CHECK_EQUAL(success(), point.lock_safe(_alice));
    BOOST_CHECK_EQUAL(success(), point.unlock_safe2(_alice, half, _bob));
    BOOST_CHECK_EQUAL(success(), point.transfer(_alice, _bob, tenth));
    BOOST_CHECK_EQUAL(success(), point.lock_safe(_alice, tenth));
    BOOST_CHECK_EQUAL(err.balance_lock, point.transfer(_alice, _bob, half - tenth));
    const auto max = half - 2 * tenth;
    BOOST_CHECK_EQUAL(err.balance_lock, point.transfer(_alice, _bob, max + satoshi));
    BOOST_CHECK_EQUAL(err.balance_lock, point.retire(_alice, max + satoshi));
    BOOST_CHECK_EQUAL(success(), point.transfer(_alice, _bob, max));
    showBalances();
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(global_lock, commun_point_safe_tester) try {
    BOOST_TEST_MESSAGE("Globally locked balance");
    init();
    BOOST_TEST_MESSAGE("--- issue points");
    const double money = 100;
    BOOST_CHECK_EQUAL(success(), point.issue(_gls_com, 2*money));
    BOOST_CHECK_EQUAL(success(), point2.issue(_tst_com, 2*money));
    BOOST_CHECK_EQUAL(success(), point.transfer(_gls_com, _alice, money));
    BOOST_CHECK_EQUAL(success(), point.transfer(_gls_com, _bob, money));
    BOOST_CHECK_EQUAL(success(), point2.transfer(_tst_com, _alice, money));
    BOOST_CHECK_EQUAL(success(), point2.transfer(_tst_com, _bob, money));
    BOOST_TEST_MESSAGE("--- enable safe for " << point.name() << "; no safe for " << point2.name());
    BOOST_CHECK_EQUAL(success(), point.enable_safe(_alice, money / 2, _delay, _bob));
    BOOST_TEST_MESSAGE("--- transfer/withdraw/retire allowed");
    const double tenth = money / 10;
    const auto satoshi = point.satoshi();
    BOOST_CHECK_EQUAL(success(), point.transfer(_alice, _bob, tenth));
    BOOST_CHECK_EQUAL(success(), point2.transfer(_alice, _bob, tenth));
    BOOST_CHECK_EQUAL(success(), point.retire(_alice, tenth));
    BOOST_CHECK_EQUAL(success(), point2.retire(_alice, tenth));

    BOOST_TEST_MESSAGE("--- global lock fails if bad period");
    BOOST_CHECK_EQUAL(err.period_le0, point.global_lock(_alice, 0));
    BOOST_CHECK_EQUAL(err.period_gt_max, point.global_lock(_alice, cfg::safe_max_delay + 1));
    BOOST_TEST_MESSAGE("--- succeeds if increase lock period");
    auto locked = 30;
    BOOST_CHECK_EQUAL(success(), point.global_lock(_alice, 1));
    BOOST_CHECK_EQUAL(success(), point.global_lock(_alice, 3));
    BOOST_CHECK_EQUAL(success(), point.global_lock(_alice, locked));
    produce_block(); locked -= 3;
    BOOST_TEST_MESSAGE("--- fails if decrease lock period");
    BOOST_CHECK_EQUAL(err.period_le_cur, point.global_lock(_alice, 1));
    BOOST_CHECK_EQUAL(err.period_le_cur, point.global_lock(_alice, locked));
    locked++;
    BOOST_CHECK_EQUAL(success(), point.global_lock(_alice, locked));
    locked = _delay*2;
    BOOST_CHECK_EQUAL(success(), point.global_lock(_alice, locked));
    BOOST_TEST_MESSAGE("--- transfers disallowed on both " << point.name() << " & " << point2.name());
    BOOST_CHECK_EQUAL(err.global_lock, point.transfer(_alice, _bob, tenth));
    BOOST_CHECK_EQUAL(err.global_lock, point2.transfer(_alice, _bob, tenth));
    BOOST_CHECK_EQUAL(err.global_lock, point.retire(_alice, tenth));
    BOOST_CHECK_EQUAL(err.global_lock, point2.retire(_alice, tenth));
    BOOST_TEST_MESSAGE("--- delayed mods disallowed when locked globally");
    BOOST_CHECK_EQUAL(success(), point.unlock_safe(_alice, N(unlock), tenth));
    BOOST_CHECK_EQUAL(success(), point.modify_safe(_alice, N(mod), _delay/2));
    BOOST_CHECK_EQUAL(success(), point.disable_safe(_alice, N(off)));
    BOOST_CHECK_EQUAL(success(), point.unlock_safe(_alice, N(u2), 1));
    BOOST_CHECK_EQUAL(success(), point.modify_safe(_alice, N(m2), _delay*2));
    auto blocks = commun::seconds_to_blocks(_delay);
    produce_blocks(blocks);
    locked -= blocks*3;
    BOOST_CHECK_EQUAL(err.global_lock, point.transfer(_alice, _bob, tenth));
    BOOST_CHECK_EQUAL(err.global_lock, point2.transfer(_alice, _bob, tenth));
    BOOST_CHECK_EQUAL(err.global_lock, point.retire(_alice, tenth));
    BOOST_CHECK_EQUAL(err.global_lock, point2.retire(_alice, tenth));
    BOOST_CHECK_EQUAL(err.mod_global_lock, point.apply_safe_mod(_alice, N(unlock)));
    BOOST_CHECK_EQUAL(err.mod_global_lock, point.apply_safe_mod(_alice, N(mod)));
    BOOST_CHECK_EQUAL(err.mod_global_lock, point.apply_safe_mod(_alice, N(off)));
    BOOST_TEST_MESSAGE("--- but still allowed with trusted");
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod2(_alice, N(unlock), _bob));
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod2(_alice, N(mod), _bob));
    BOOST_CHECK_EQUAL(success(), point.unlock_safe2(_alice, tenth, _bob));
    BOOST_CHECK_EQUAL(success(), point.modify_safe2(_alice, _delay, {}, _bob));
    BOOST_TEST_MESSAGE("--- wait 1 block before global unlock and ensure it's still locked");
    blocks = commun::seconds_to_blocks(locked);
    produce_blocks(blocks - 1);
    BOOST_CHECK_EQUAL(err.global_lock, point.transfer(_alice, _bob, tenth));
    BOOST_CHECK_EQUAL(err.global_lock, point2.transfer(_alice, _bob, tenth));
    BOOST_CHECK_EQUAL(err.global_lock, point.retire(_alice, tenth));
    BOOST_CHECK_EQUAL(err.global_lock, point2.retire(_alice, tenth));
    BOOST_CHECK_EQUAL(err.mod_global_lock, point.apply_safe_mod(_alice, N(u2)));
    BOOST_CHECK_EQUAL(err.mod_global_lock, point.apply_safe_mod(_alice, N(m2)));
    BOOST_CHECK_EQUAL(err.mod_global_lock, point.apply_safe_mod(_alice, N(off)));
    BOOST_TEST_MESSAGE("--- delayed mods and transfers allowed after global unlock");
    produce_block();
    BOOST_CHECK_EQUAL(success(), point.transfer(_alice, _bob, tenth));
    BOOST_CHECK_EQUAL(success(), point2.transfer(_alice, _bob, tenth));
    BOOST_CHECK_EQUAL(success(), point.retire(_alice, tenth));
    BOOST_CHECK_EQUAL(success(), point2.retire(_alice, tenth));
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod(_alice, N(u2)));
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod(_alice, N(m2)));
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod(_alice, N(off)));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(unlock_overflow, commun_point_safe_tester) try {
    BOOST_TEST_MESSAGE("Unlocked overflow");
    init();
    BOOST_TEST_MESSAGE("--- enable safes");
    const asset max{asset::max_amount, point._symbol};
    BOOST_CHECK_EQUAL(success(), point.enable_safe(_bob, 0, _delay, _alice));
    BOOST_CHECK_EQUAL(success(), point.enable_safe(_alice, max, _delay, _bob));
    BOOST_TEST_MESSAGE("--- bob unlocks until limit");
    const asset qtr{(asset::max_amount + 1) / 4, point._symbol};
    BOOST_CHECK_EQUAL(success(), point.unlock_safe2(_bob, qtr, _alice));
    produce_block();
    BOOST_CHECK_EQUAL(success(), point.unlock_safe2(_bob, qtr, _alice));
    produce_block();
    BOOST_CHECK_EQUAL(success(), point.unlock_safe2(_bob, qtr, _alice));
    produce_block();
    BOOST_CHECK_EQUAL(err.unlocked_over, point.unlock_safe2(_bob, qtr, _alice));
    const auto satoshi = point.satoshi_asset();
    BOOST_CHECK_EQUAL(success(), point.unlock_safe2(_bob, qtr - satoshi, _alice));

    BOOST_TEST_MESSAGE("--- alice fails to unlock because already unlocked max");
    BOOST_CHECK_EQUAL(err.unlocked_over, point.unlock_safe2(_alice, max, _bob));
    BOOST_CHECK_EQUAL(err.unlocked_over, point.unlock_safe2(_alice, qtr, _bob));
    BOOST_CHECK_EQUAL(err.unlocked_over, point.unlock_safe2(_alice, 1, _bob));
    BOOST_CHECK_EQUAL(err.unlocked_over, point.unlock_safe2(_alice, satoshi, _bob));
    BOOST_TEST_MESSAGE("------ delayed fails too");
    BOOST_CHECK_EQUAL(success(), point.unlock_safe(_alice, N(max), max));
    BOOST_CHECK_EQUAL(success(), point.unlock_safe(_alice, N(one), 1));
    BOOST_CHECK_EQUAL(success(), point.unlock_safe(_alice, N(sat), satoshi));
    const auto blocks = commun::seconds_to_blocks(_delay);
    produce_blocks(blocks);
    BOOST_CHECK_EQUAL(err.unlocked_over, point.apply_safe_mod(_alice, N(max)));
    BOOST_CHECK_EQUAL(err.unlocked_over, point.apply_safe_mod(_alice, N(one)));
    BOOST_CHECK_EQUAL(err.unlocked_over, point.apply_safe_mod(_alice, N(sat)));
    BOOST_TEST_MESSAGE("--- success if lock some amount");
    BOOST_CHECK_EQUAL(success(), point.lock_safe(_alice, 1));
    BOOST_CHECK_EQUAL(success(), point.apply_safe_mod(_alice, N(one)));
    BOOST_CHECK_EQUAL(err.unlocked_over, point.apply_safe_mod(_alice, N(sat)));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(reserve_token, commun_point_safe_tester) try {
    BOOST_TEST_MESSAGE("Reserve token in points");
    init();
    commun_point_api reserve{this, _code, symbol{0, ""}}; // can't use symbol{}, because it creates "4,SYS" symbol
    BOOST_TEST_MESSAGE("--- issue and deposit");
    const double money = 100;
    BOOST_CHECK_EQUAL(success(), reserve.open(_bob));
    BOOST_CHECK_EQUAL(success(), token.issue(_dapp, _bob, token.make_asset(money), ""));
    BOOST_CHECK_EQUAL(success(), token.transfer(_bob, cfg::point_name, token.make_asset(money), ""));

    BOOST_TEST_MESSAGE("--- successful withdraw when not locked");
    const auto tenth = money / 10;
    BOOST_CHECK_EQUAL(success(), reserve.withdraw(_bob, token.make_asset(tenth)));
    produce_block();
    BOOST_TEST_MESSAGE("--- fail to withdraw when locked globally");
    BOOST_CHECK_EQUAL(success(), reserve.global_lock(_bob, _delay));
    BOOST_CHECK_EQUAL(err.global_lock, reserve.withdraw(_bob, token.make_asset(tenth)));
    const auto blocks = commun::seconds_to_blocks(_delay);
    produce_blocks(blocks - 1);
    BOOST_CHECK_EQUAL(err.global_lock, reserve.withdraw(_bob, token.make_asset(tenth)));
    BOOST_TEST_MESSAGE("--- succes to withdraw when global lock expired");
    produce_block();
    BOOST_CHECK_EQUAL(success(), reserve.withdraw(_bob, token.make_asset(tenth)));
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
