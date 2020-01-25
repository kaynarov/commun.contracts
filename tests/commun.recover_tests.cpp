#include <commun.recover_test_api.hpp>
#include <commun.point_test_api.hpp>
#include "contracts.hpp"
#include <commun/config.hpp>

namespace std {

  template<class _Traits>
    basic_ostream<char, _Traits>&
    operator<<(basic_ostream<char, _Traits>& __out, const fc::time_point& __s)
    { return (__out << static_cast<std::string>(__s)); }

  template<class _Traits>
    basic_ostream<char, _Traits>&
    operator<<(basic_ostream<char, _Traits>& __out, const fc::time_point_sec& __s)
    { return (__out << static_cast<fc::time_point>(__s)); }

}  // namespace std

namespace cfg = commun::config;
using namespace eosio::testing;
using namespace eosio::chain;
using namespace fc;

static const int64_t block_interval = cfg::block_interval_ms / 1000;

static const auto point_code_str = "GLS";
static const auto _point = symbol(3, point_code_str);

class commun_recover_tester : public golos_tester {
protected:
    commun_recover_api recover;
    commun_point_api point;

public:
    commun_recover_tester()
        : golos_tester(cfg::recover_name)
        , recover(this, cfg::recover_name)
        , point(this, cfg::point_name, _point)
    {
        create_accounts({cfg::recover_name, cfg::point_name, _alice, _bob});
        produce_block();
        install_contract(cfg::recover_name, contracts::recover_wasm(), contracts::recover_abi());
        install_contract(cfg::point_name, contracts::point_wasm(), contracts::point_abi());

        set_authority(_alice, cfg::owner_name,
            authority(1, {{.key = get_public_key(_alice, "owner"), .weight = 1}},
                    {{.permission = {cfg::recover_name, cfg::code_name}, .weight = 1}}), "");
    }

    const account_name _alice = N(alice);
    const account_name _bob = N(bob);

    struct errors: contract_error_messages {
        const string norequest = amsg("Request for change owner key doesn't exists");
        const string emptyrecover = amsg("Specify at least one key for recovery");
        const string tooearly = amsg("Change owner too early");
        const string recoverunavailable = amsg("Key recovery for this account is not available");
        const string no_changes = amsg("No params changed");
    } err;

    action_result check_auth(account_name account, name auth, const private_key_type& key) {
        transaction_trace_ptr trace = nullptr;
        try {
            trace = push_reqauth(account, {{account, auth}}, {key});
        } catch (const fc::exception& ex) {
            edump((ex.to_detail_string()));
            return error(ex.top_message()); // top_message() is assumed by many tests; otherwise they fail
            //return error(ex.to_detail_string());
        }
        produce_block();
        BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trace->id));
        return success();
    }
};


BOOST_AUTO_TEST_SUITE(commun_recover_tests)

BOOST_FIXTURE_TEST_CASE(empty_recovery, commun_recover_tester) try {
    BOOST_TEST_MESSAGE("empty_recovery");
    BOOST_CHECK_EQUAL(err.emptyrecover, recover.recover(_alice, public_key_type(), public_key_type()));
    BOOST_CHECK_EQUAL(err.emptyrecover, recover.recover(_bob, public_key_type(), public_key_type()));
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE(recovery_unavailable, commun_recover_tester) try {
    BOOST_TEST_MESSAGE("recovery_unavailable");
    BOOST_CHECK_EQUAL(err.recoverunavailable, recover.recover(_bob, get_public_key(_bob, "active2"), public_key_type()));
    BOOST_CHECK_EQUAL(err.recoverunavailable, recover.recover(_bob, public_key_type(), get_public_key(_bob, "owner2")));
    BOOST_CHECK_EQUAL(err.recoverunavailable, recover.recover(_bob, get_public_key(_bob, "active2"), get_public_key(_bob, "owner2")));
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE(recover_active_authority, commun_recover_tester) try {
    BOOST_TEST_MESSAGE("recover_active_authority");

    BOOST_CHECK_EQUAL(success(), check_auth(_alice, cfg::active_name, get_private_key(_alice, "active")));
    
    BOOST_CHECK_EQUAL(success(), recover.recover(_alice, get_public_key(_alice, "active2"), public_key_type()));
    BOOST_CHECK_EQUAL(success(), check_auth(_alice, cfg::active_name, get_private_key(_alice, "active2")));
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE(point_global_lock_integration, commun_recover_tester) try {
    BOOST_TEST_MESSAGE("point_global_lock_integration");
    time_point unlock_time;

    // 1. No global lock
    BOOST_CHECK(point.get_global_lock(_alice).is_null());

    BOOST_CHECK_EQUAL(success(), recover.recover(_alice, get_public_key(_alice, "active"), public_key_type()));
    unlock_time = control->pending_block_time() + fc::seconds(cfg::def_recover_delay);
    BOOST_CHECK_EQUAL(unlock_time, point.get_global_lock(_alice)["unlocks"].as_time_point());

    produce_block();

    // 2. Global lock earlier then current_time plus recover_delay
    BOOST_CHECK_EQUAL(success(), point.global_lock(_alice, cfg::def_recover_delay));
    unlock_time = control->pending_block_time() + fc::seconds(cfg::def_recover_delay);
    BOOST_CHECK_EQUAL(unlock_time, point.get_global_lock(_alice)["unlocks"].as_time_point());

    produce_blocks(5);
    BOOST_CHECK_EQUAL(success(), recover.recover(_alice, get_public_key(_alice, "active"), public_key_type()));
    unlock_time = control->pending_block_time() + fc::seconds(cfg::def_recover_delay);
    BOOST_CHECK_EQUAL(unlock_time, point.get_global_lock(_alice)["unlocks"].as_time_point());

    // 3. Global lock later then current_time plus recover_delay
    BOOST_CHECK_EQUAL(success(), point.global_lock(_alice, cfg::def_recover_delay+1200));
    unlock_time = control->pending_block_time() + fc::seconds(cfg::def_recover_delay+1200);
    BOOST_CHECK_EQUAL(unlock_time, point.get_global_lock(_alice)["unlocks"].as_time_point());

    produce_blocks(5);
    BOOST_CHECK_EQUAL(success(), recover.recover(_alice, get_public_key(_alice, "active"), public_key_type()));
    // unlock_time has not changed
    BOOST_CHECK_EQUAL(unlock_time, point.get_global_lock(_alice)["unlocks"].as_time_point());

} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE(recover_owner_authority, commun_recover_tester) try {
    BOOST_TEST_MESSAGE("recover_owner_authority");

    BOOST_CHECK_EQUAL(success(), check_auth(_alice, cfg::owner_name, get_private_key(_alice, "owner")));
    BOOST_CHECK_EQUAL(success(), recover.recover(_alice, public_key_type(), get_public_key(_alice, "owner2")));
    BOOST_CHECK_EQUAL(success(), check_auth(_alice, cfg::owner_name, get_private_key(_alice, "owner")));

    produce_block(fc::seconds(cfg::def_recover_delay - 2 * block_interval));
    BOOST_CHECK_EQUAL(err.tooearly, recover.applyowner(_alice));

    produce_block();
    BOOST_CHECK_EQUAL(success(), recover.applyowner(_alice));
    BOOST_CHECK_EQUAL(success(), check_auth(_alice, cfg::owner_name, get_private_key(_alice, "owner2")));
} FC_LOG_AND_RETHROW()


BOOST_FIXTURE_TEST_CASE(cancel_owner_authority, commun_recover_tester) try {
    BOOST_TEST_MESSAGE("cancel_owner_authority");

    BOOST_CHECK_EQUAL(success(), check_auth(_alice, cfg::owner_name, get_private_key(_alice, "owner")));
    BOOST_CHECK_EQUAL(success(), recover.recover(_alice, public_key_type(), get_public_key(_alice, "owner2")));

    BOOST_CHECK_EQUAL(success(), recover.cancelowner(_alice));
    produce_block();

    BOOST_CHECK_EQUAL(err.norequest, recover.applyowner(_alice));
    BOOST_CHECK_EQUAL(err.norequest, recover.cancelowner(_alice));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(change_params, commun_recover_tester) try {
    BOOST_TEST_MESSAGE("change_params");

    BOOST_CHECK_EQUAL(err.no_changes, recover.setparams(_code, recover.args()));
    BOOST_CHECK_EQUAL(err.no_changes, recover.setparams(_code,
            recover.args()("recover_delay", cfg::def_recover_delay)));

    uint64_t recover_delay = cfg::def_recover_delay/2;
    BOOST_CHECK_EQUAL(success(), recover.setparams(_code,
            recover.args()("recover_delay", recover_delay)));
    produce_block();

    BOOST_TEST_MESSAGE("check changed params");
    BOOST_CHECK_EQUAL(success(), recover.recover(_alice, public_key_type(), get_public_key(_alice, "owner2")));
    BOOST_CHECK_EQUAL(success(), check_auth(_alice, cfg::owner_name, get_private_key(_alice, "owner")));

    produce_block(fc::seconds(recover_delay - 2*block_interval));
    BOOST_CHECK_EQUAL(err.tooearly, recover.applyowner(_alice));

    produce_block();
    BOOST_CHECK_EQUAL(success(), recover.applyowner(_alice));
    BOOST_CHECK_EQUAL(success(), check_auth(_alice, cfg::owner_name, get_private_key(_alice, "owner2")));
} FC_LOG_AND_RETHROW()


BOOST_AUTO_TEST_SUITE_END()
