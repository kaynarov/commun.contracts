#include <commun.recover_test_api.hpp>
#include "contracts.hpp"
#include <commun/config.hpp>


namespace cfg = commun::config;
using namespace eosio::testing;
using namespace eosio::chain;
using namespace fc;

static const int64_t block_interval = cfg::block_interval_ms / 1000;

class commun_recover_tester : public golos_tester {
protected:
    commun_recover_api recover;

public:
    commun_recover_tester()
        : golos_tester(cfg::recover_name)
        , recover(this, cfg::recover_name)
    {
        create_accounts({cfg::recover_name, _alice, _bob});
        produce_block();
        install_contract(cfg::recover_name, contracts::recover_wasm(), contracts::recover_abi());

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
