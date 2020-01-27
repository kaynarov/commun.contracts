#include "commun.point_test_api.hpp"
#include "commun.ctrl_test_api.hpp"
#include "cyber.token_test_api.hpp"
#include "commun.list_test_api.hpp"
#include "commun.emit_test_api.hpp"
#include "../commun.emit/include/commun.emit/config.hpp"
#include "../commun.ctrl/include/commun.ctrl/config.hpp"
#include "contracts.hpp"

namespace cfg = commun::config;
using namespace eosio::testing;
using namespace eosio::chain;
using namespace fc;
static const auto point_code_str = "GLS";
static const auto _point = symbol(3, point_code_str);
static const auto point_code = _point.to_symbol_code();

static const int64_t block_interval = cfg::block_interval_ms / 1000;

const account_name _golos = N(golos);
const account_name _alice = N(alice);
const account_name _bob = N(bob);
const account_name _carol = N(carol);
const account_name _client = N(communcom);
const std::vector<account_name> leaders = {
    N(leadera), N(leaderb), N(leaderc), N(leaderd), N(leadere), N(leaderf),
    N(leaderg), N(leaderh), N(leaderi), N(leaderj), N(leaderk)};

class commun_ctrl_tester : public golos_tester {
protected:
    cyber_token_api token;
    commun_point_api point;
    commun_list_api community;
    commun_ctrl_api dapp_ctrl;
    commun_ctrl_api comm_ctrl;
    commun_emit_api emit;
public:
    commun_ctrl_tester()
        : golos_tester(cfg::control_name)
        , token({this, cfg::token_name, cfg::reserve_token})
        , point({this, cfg::point_name, _point})
        , community({this, cfg::list_name})
        , dapp_ctrl({this, cfg::control_name, symbol_code(), cfg::dapp_name})
        , comm_ctrl({this, cfg::control_name, _point.to_symbol_code(), _golos})
        , emit({this, cfg::emit_name})
    {
        create_accounts({cfg::dapp_name, _golos, _alice, _bob, _carol, _client,
            cfg::token_name, cfg::list_name, cfg::control_name, cfg::point_name, cfg::emit_name, cfg::gallery_name});
        create_accounts(leaders);
        produce_block();
        install_contract(cfg::control_name, contracts::ctrl_wasm(), contracts::ctrl_abi());
        install_contract(cfg::token_name, contracts::token_wasm(), contracts::token_abi());
        install_contract(cfg::point_name, contracts::point_wasm(), contracts::point_abi());
        install_contract(cfg::list_name, contracts::list_wasm(), contracts::list_abi());
        install_contract(cfg::emit_name, contracts::emit_wasm(), contracts::emit_abi());
        install_contract(cfg::gallery_name, contracts::gallery_wasm(), contracts::gallery_abi());

        set_authority(cfg::control_name, N(changepoints), create_code_authority({cfg::point_name}), "active");
        link_authority(cfg::control_name, cfg::control_name, N(changepoints), N(changepoints));

        set_authority(cfg::control_name, cfg::client_permission_name,
            authority (1, {}, {{.permission = {_client, "active"}, .weight = 1}}), "owner");
        link_authority(cfg::control_name, cfg::control_name, cfg::client_permission_name, N(emit));

        set_authority(cfg::emit_name, cfg::reward_perm_name, create_code_authority({_code}), "active");
        link_authority(cfg::emit_name, cfg::emit_name, cfg::reward_perm_name, N(issuereward));

        set_authority(cfg::emit_name, N(init), create_code_authority({cfg::list_name}), "active");
        link_authority(cfg::emit_name, cfg::emit_name, N(init), N(init));

        set_authority(cfg::control_name, N(init), create_code_authority({cfg::list_name}), "active");
        link_authority(cfg::control_name, cfg::control_name, N(init), N(init));

        set_authority(cfg::control_name, cfg::transfer_permission, create_code_authority({cfg::control_name}), "active");
        link_authority(cfg::control_name, cfg::point_name, cfg::transfer_permission, N(transfer));

        set_authority(cfg::gallery_name, N(init), create_code_authority({cfg::list_name}), "active");
        link_authority(cfg::gallery_name, cfg::gallery_name, N(init), N(init));

        set_authority(cfg::point_name, cfg::issue_permission, create_code_authority({cfg::emit_name}), "active");
        link_authority(cfg::point_name, cfg::point_name, cfg::issue_permission, N(issue));

    }
    struct errors : contract_error_messages {
        const string no_community = amsg("community not exists");
        const string no_balance = amsg("balance does not exist");

        const string not_a_leader(account_name leader) { return amsg((leader.to_string() + " is not a leader")); }
        const string approved = amsg("already approved");
        const string authorization_failed = amsg("transaction authorization failed");
        const string no_leaders = amsg("leaders num must be positive");
        const string votes_must_be_positive = amsg("max votes must be positive");
        const string inconsistent_threshold = amsg("inconsistent set of threshold parameters");
        const string no_threshold = amsg("required threshold must be positive");
        const string leaders_less = amsg("leaders num can't be less than required threshold");
        const string threshold_greater = amsg("required threshold can't be greater than leaders num");
        const string nothing_to_claim = amsg("nothing to claim");
        const string leader_not_found = amsg("leader not found");
        const string leader_not_active = amsg("leader not active");
        const string not_updated_flag = amsg("active flag not updated");
        const string voted = amsg("already voted");
        const string no_vote = amsg("there is no vote for this leader");

        const string pct_0 = amsg("pct can't be 0");
        const string pct_greater_100 = amsg("pct can't be greater than 100%");
        const string incorrect_pct = amsg("incorrect pct");
        const string votes_casted = amsg("all allowed votes already casted");
        const string power_casted = amsg("all power already casted");
        const string votes_greater_100 = amsg("all votes exceed 100%");

        const string must_be_deactivated = amsg("leader must be deactivated");
        const string no_votes = amsg("no votes");
        const string incorrect_count = amsg("incorrect count");

        const string there_are_votes = amsg("not possible to remove leader as there are votes");

        const string it_isnt_time_for_reward = amsg("it isn't time for reward");
    } err;

    transaction get_point_create_trx(const vector<permission_level>& auths, name issuer, asset initial_supply, asset maximum_supply, int16_t cw, int16_t fee) {
        fc::variants v;
        for (auto& level : auths) {
            v.push_back(fc::mutable_variant_object()("actor", level.actor)("permission", level.permission));
        }
        variant pretty_trx = fc::mutable_variant_object()
            ("expiration", "2020-01-01T00:30")
            ("ref_block_num", 2)
            ("ref_block_prefix", 3)
            ("max_net_usage_words", 0)
            ("max_cpu_usage_ms", 0)
            ("max_ram_kbytes", 0)
            ("max_storage_kbytes", 0)
            ("delay_sec", 0)
            ("actions", fc::variants({
            fc::mutable_variant_object()
                ("account", name(cfg::point_name))
                ("name", "create")
                ("authorization", v)
                ("data", fc::mutable_variant_object()
                    ("issuer", issuer)
                    ("initial_supply", initial_supply)
                    ("maximum_supply", maximum_supply)
                    ("cw", cw)
                    ("fee", fee)
                )}));
       transaction trx;
       abi_serializer::from_variant(pretty_trx, trx, get_resolver(), abi_serializer_max_time);
       return trx;
    }

    int64_t supply  = 5000000000000;
    int64_t reserve = 50000000000;

    void init() {
        BOOST_CHECK_EQUAL(success(), token.create(cfg::dapp_name, asset(reserve, token._symbol)));
        BOOST_CHECK_EQUAL(success(), token.issue(cfg::dapp_name, _golos, asset(reserve, token._symbol), ""));

        BOOST_CHECK_EQUAL(success(), point.create(_golos, asset(0, point._symbol), asset(supply * 2, point._symbol), 10000, 1));
        BOOST_CHECK_EQUAL(success(), community.create(cfg::list_name, point_code, "The community"));

        set_authority(_golos, cfg::transfer_permission, create_code_authority({cfg::emit_name}), cfg::active_name);
        link_authority(_golos, cfg::point_name, cfg::transfer_permission, N(transfer));

        BOOST_CHECK_EQUAL(success(), token.transfer(_golos, cfg::point_name, asset(reserve, token._symbol), cfg::restock_prefix + point_code_str));
        BOOST_CHECK_EQUAL(success(), point.issue(_golos, asset(supply, point._symbol), std::string(point_code_str) + " issue"));
        BOOST_CHECK_EQUAL(success(), point.open(_code));
    }
};

BOOST_AUTO_TEST_SUITE(commun_ctrl_tests)

BOOST_FIXTURE_TEST_CASE(basic_tests, commun_ctrl_tester) try {
    BOOST_TEST_MESSAGE("basic_tests");

    BOOST_CHECK_EQUAL(success(), token.create(_golos, asset(999999, token._symbol)));

    BOOST_CHECK_EQUAL(success(), community.setappparams(community.args()
        ("permission", config::active_name)("required_threshold", 11)));

    for (auto l : leaders) {
        BOOST_CHECK_EQUAL(success(), point.open(l, symbol_code{}));
        BOOST_CHECK_EQUAL(success(), token.issue(_golos, l, asset(42, token._symbol), ""));
        BOOST_CHECK_EQUAL(success(), token.transfer(l, cfg::point_name, asset(42, token._symbol), ""));
        BOOST_CHECK_EQUAL(base_tester::success(), dapp_ctrl.reg_leader(l, "localhost"));
        BOOST_CHECK_EQUAL(base_tester::success(), dapp_ctrl.vote_leader(l, l));
    }
    set_authority(cfg::dapp_name, cfg::active_name, create_code_authority({cfg::control_name}), "owner");
    set_authority(cfg::point_name, cfg::active_name,
                  authority (1, {}, {{.permission = {cfg::dapp_name, config::active_name}, .weight = 1}}), "owner");

    produce_block();

    asset init_supply(0, point._symbol);
    asset max_supply(999999, point._symbol);
    auto trx = get_point_create_trx({permission_level{cfg::point_name, cfg::active_name}},
        _golos, init_supply, max_supply, 5000, 0);

    BOOST_CHECK_EQUAL(err.not_a_leader(_alice), dapp_ctrl.propose(_alice, N(goloscreate), cfg::active_name, trx));
    BOOST_CHECK_EQUAL(success(), dapp_ctrl.propose(leaders[0], N(goloscreate), cfg::active_name, trx));
    for (size_t i = 0; i < 10; i++) {
        BOOST_CHECK_EQUAL(success(), dapp_ctrl.approve(leaders[0], N(goloscreate), leaders[i]));
    }
    produce_block();
    BOOST_CHECK_EQUAL(err.approved, dapp_ctrl.approve(leaders[0], N(goloscreate), leaders[9]));
    BOOST_CHECK_EQUAL(err.authorization_failed, dapp_ctrl.exec(leaders[0], N(goloscreate), _bob));

    produce_block();
    BOOST_CHECK(point.get_params().is_null());

    BOOST_CHECK_EQUAL(success(), dapp_ctrl.approve(leaders[0], N(goloscreate), leaders[10]));
    BOOST_CHECK_EQUAL(success(), dapp_ctrl.exec(leaders[0], N(goloscreate), _bob));

    CHECK_MATCHING_OBJECT(point.get_params(), mvo()
        ("cw", 5000)
        ("fee", 0)
        ("max_supply", max_supply)
        ("issuer", _golos.to_string())
    );

    //TODO: test governance of the created community

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(control_param_test, commun_ctrl_tester) try {
    BOOST_TEST_MESSAGE("control_param_test");

    BOOST_CHECK_EQUAL(err.no_leaders, community.setappparams(community.args()
        ("leaders_num", 0)));
    BOOST_CHECK_EQUAL(err.votes_must_be_positive, community.setappparams(community.args()
        ("max_votes", 0)));
    BOOST_CHECK_EQUAL(err.inconsistent_threshold, community.setappparams(community.args()
        ("permission", config::active_name)));
    BOOST_CHECK_EQUAL(err.inconsistent_threshold, community.setappparams(community.args()
        ("required_threshold", 11)));
    BOOST_CHECK_EQUAL(err.no_threshold, community.setappparams(community.args()
        ("permission", config::active_name)("required_threshold", 0)));
    BOOST_CHECK_EQUAL(err.leaders_less, community.setappparams(community.args()
        ("leaders_num", 10)("permission", config::active_name)("required_threshold", 11)));
    BOOST_CHECK_EQUAL(err.threshold_greater, community.setappparams(community.args()
        ("permission", config::active_name)("required_threshold", 22)));

    BOOST_CHECK_EQUAL(success(), community.setappparams(community.args()
        ("leaders_num", 10)));
    BOOST_CHECK_EQUAL(success(), community.setappparams(community.args()
        ("permission", config::active_name)("required_threshold", 10)));
    BOOST_CHECK_EQUAL(err.threshold_greater, community.setappparams(community.args()
        ("permission", config::active_name)("required_threshold", 11)));

    BOOST_CHECK_EQUAL(success(), community.setappparams(community.args()
        ("leaders_num", 21)("permission", N(twelve))("required_threshold", 12)));
    BOOST_CHECK_EQUAL(err.leaders_less, community.setappparams(community.args()
        ("leaders_num", 11)));
    BOOST_CHECK_EQUAL(success(), community.setappparams(community.args()
        ("leaders_num", 12)));
    BOOST_CHECK_EQUAL(success(), community.setappparams(community.args()
        ("leaders_num", 11)("permission", N(twelve))("required_threshold", 0)));

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(regleader_test, commun_ctrl_tester) try {
    BOOST_TEST_MESSAGE("regleader_test");
    BOOST_CHECK_EQUAL(err.no_community, comm_ctrl.reg_leader(_alice, "https://foo.bar"));
    init();

    BOOST_CHECK_EQUAL(success(), comm_ctrl.reg_leader(_alice, "https://foo.bar"));
    produce_block();
    BOOST_CHECK_EQUAL(success(), comm_ctrl.reg_leader(_alice, "https://foo.bar"));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(unregleader_test, commun_ctrl_tester) try {
    BOOST_TEST_MESSAGE("unregleader_test");
    BOOST_CHECK_EQUAL(err.no_community, comm_ctrl.unreg_leader(_alice));
    init();

    BOOST_CHECK_EQUAL(success(), comm_ctrl.reg_leader(_alice, "https://foo.bar"));
    BOOST_CHECK_EQUAL(success(), point.open(_bob));
    BOOST_CHECK_EQUAL(success(), comm_ctrl.vote_leader(_bob, _alice));
    BOOST_CHECK_EQUAL(err.there_are_votes, comm_ctrl.unreg_leader(_alice));

    BOOST_CHECK_EQUAL(success(), comm_ctrl.stop_leader(_alice));
    BOOST_CHECK_EQUAL(success(), comm_ctrl.clear_votes(_alice));
    BOOST_CHECK_EQUAL(success(), comm_ctrl.unreg_leader(_alice));
    produce_block();
    BOOST_CHECK_EQUAL(err.leader_not_found, comm_ctrl.unreg_leader(_alice));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(startleader_test, commun_ctrl_tester) try {
    BOOST_TEST_MESSAGE("startleader_test");
    BOOST_CHECK_EQUAL(err.no_community, comm_ctrl.start_leader(_alice));
    init();
    BOOST_CHECK_EQUAL(err.leader_not_found, comm_ctrl.start_leader(_alice));

    BOOST_CHECK_EQUAL(success(), comm_ctrl.reg_leader(_alice, "https://foo.bar"));
    BOOST_CHECK_EQUAL(err.not_updated_flag, comm_ctrl.start_leader(_alice));

    BOOST_CHECK_EQUAL(success(), comm_ctrl.stop_leader(_alice));
    BOOST_CHECK_EQUAL(success(), comm_ctrl.start_leader(_alice));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(stopleader_test, commun_ctrl_tester) try {
    BOOST_TEST_MESSAGE("stopleader_test");
    BOOST_CHECK_EQUAL(err.no_community, comm_ctrl.stop_leader(_alice));
    init();
    BOOST_CHECK_EQUAL(err.leader_not_found, comm_ctrl.stop_leader(_alice));

    BOOST_CHECK_EQUAL(success(), comm_ctrl.reg_leader(_alice, "https://foo.bar"));
    BOOST_CHECK_EQUAL(success(), comm_ctrl.stop_leader(_alice));
    produce_block();
    BOOST_CHECK_EQUAL(err.not_updated_flag, comm_ctrl.stop_leader(_alice));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(voteleader_test, commun_ctrl_tester) try {
    BOOST_TEST_MESSAGE("voteleader_test");
    BOOST_CHECK_EQUAL(success(), token.create(cfg::dapp_name, asset(9999999, token._symbol)));
    BOOST_CHECK_EQUAL(success(), token.issue(cfg::dapp_name, _golos, asset(9999999, token._symbol), ""));
    BOOST_CHECK_EQUAL(success(), point.create(_golos, asset(0, point._symbol), asset(9999999, point._symbol), 10000, 1));
    BOOST_CHECK_EQUAL(err.no_community, comm_ctrl.vote_leader(_carol, _alice));
    BOOST_CHECK_EQUAL(success(), community.create(cfg::list_name, point_code, "GOLOS"));
    BOOST_CHECK_EQUAL(success(), community.setparams(_golos, point_code, community.args()("max_votes", 3)));
    BOOST_CHECK_EQUAL(err.leader_not_found, comm_ctrl.vote_leader(_carol, _alice));
    BOOST_CHECK_EQUAL(success(), comm_ctrl.reg_leader(_alice, "localhost"));

    BOOST_CHECK_EQUAL(success(), comm_ctrl.stop_leader(_alice));
    BOOST_CHECK_EQUAL(err.leader_not_active, comm_ctrl.vote_leader(_carol, _alice));
    BOOST_CHECK_EQUAL(success(), comm_ctrl.start_leader(_alice));

    BOOST_CHECK_EQUAL(err.pct_0, comm_ctrl.vote_leader(_carol, _alice, 0));
    BOOST_CHECK_EQUAL(err.pct_greater_100, comm_ctrl.vote_leader(_carol, _alice, 110 * cfg::_1percent));
    BOOST_CHECK_EQUAL(err.incorrect_pct, comm_ctrl.vote_leader(_carol, _alice, 99 * cfg::_1percent));

    BOOST_CHECK_EQUAL(err.no_balance, comm_ctrl.vote_leader(_carol, _alice));
    BOOST_CHECK_EQUAL(success(), point.open(_carol));
    BOOST_CHECK_EQUAL(success(), comm_ctrl.vote_leader(_carol, _alice));
    produce_block();
    BOOST_CHECK_EQUAL(err.voted, comm_ctrl.vote_leader(_carol, _alice));

    BOOST_CHECK_EQUAL(success(), comm_ctrl.reg_leader(_bob, "localhost"));
    BOOST_CHECK_EQUAL(err.power_casted, comm_ctrl.vote_leader(_carol, _bob, 10 * cfg::_1percent));
    BOOST_CHECK_EQUAL(success(), comm_ctrl.unvote_leader(_carol, _alice));
    BOOST_CHECK_EQUAL(success(), comm_ctrl.vote_leader(_carol, _alice, 50  * cfg::_1percent));
    BOOST_CHECK_EQUAL(err.votes_greater_100, comm_ctrl.vote_leader(_carol, _bob, 60 * cfg::_1percent));
    BOOST_CHECK_EQUAL(success(), comm_ctrl.vote_leader(_carol, _bob, 50  * cfg::_1percent));
    produce_block();

    BOOST_CHECK_EQUAL(success(), comm_ctrl.reg_leader(_carol, "localhost"));
    BOOST_CHECK_EQUAL(err.power_casted, comm_ctrl.vote_leader(_carol, _carol, 10 * cfg::_1percent));
    BOOST_CHECK_EQUAL(success(), comm_ctrl.unvote_leader(_carol, _alice));
    BOOST_CHECK_EQUAL(success(), comm_ctrl.vote_leader(_carol, _alice, 10  * cfg::_1percent));
    BOOST_CHECK_EQUAL(success(), comm_ctrl.vote_leader(_carol, _carol, 10  * cfg::_1percent));

    BOOST_CHECK_EQUAL(success(), comm_ctrl.reg_leader(_golos, "localhost"));
    BOOST_CHECK_EQUAL(err.votes_casted, comm_ctrl.vote_leader(_carol, _golos, 10  * cfg::_1percent));

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(unvotelead_test, commun_ctrl_tester) try {
    BOOST_TEST_MESSAGE("unvotelead_test");
    BOOST_CHECK_EQUAL(err.no_community, comm_ctrl.unvote_leader(_carol, _alice));
    init();
    BOOST_CHECK_EQUAL(err.leader_not_found, comm_ctrl.unvote_leader(_carol, _alice));

    BOOST_CHECK_EQUAL(success(), comm_ctrl.reg_leader(_alice, "localhost"));
    BOOST_CHECK_EQUAL(err.no_vote, comm_ctrl.unvote_leader(_carol, _alice));

    BOOST_CHECK_EQUAL(success(), point.open(_carol));
    BOOST_CHECK_EQUAL(success(), point.issue(_bob, asset(1000, point._symbol), ""));
    BOOST_CHECK_EQUAL(success(), point.issue(_carol, asset(1000, point._symbol), ""));
    BOOST_CHECK_EQUAL(success(), comm_ctrl.vote_leader(_carol, _alice));
    BOOST_CHECK_EQUAL(success(), point.transfer(_bob, _carol, asset(700, point._symbol)));
    BOOST_CHECK_EQUAL(success(), comm_ctrl.unvote_leader(_carol, _alice));
    BOOST_CHECK_EQUAL(comm_ctrl.get_leader(_alice)["total_weight"], 0);
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(clearvotes_test, commun_ctrl_tester) try {
    BOOST_TEST_MESSAGE("clearvotes_test");
    BOOST_CHECK_EQUAL(success(), token.create(cfg::dapp_name, asset(9999999, token._symbol)));
    BOOST_CHECK_EQUAL(success(), point.create(_golos, asset(0, point._symbol), asset(9999999, point._symbol), 10000, 1));
    BOOST_CHECK_EQUAL(success(), community.create(cfg::list_name, point_code, "GOLOS"));
    BOOST_CHECK_EQUAL(success(), token.issue(cfg::dapp_name, _golos, asset(9999999, token._symbol), ""));
    BOOST_CHECK_EQUAL(success(), token.transfer(_golos, cfg::point_name, asset(9999999, token._symbol), cfg::restock_prefix + point_code_str));
    BOOST_CHECK_EQUAL(success(), point.open(_alice, symbol_code{}));
    BOOST_CHECK_EQUAL(err.leader_not_found, comm_ctrl.clear_votes(_alice));
    BOOST_CHECK_EQUAL(base_tester::success(), comm_ctrl.reg_leader(_alice, "localhost"));

    auto votes_num = cfg::max_clearvotes_count + cfg::max_clearvotes_count / 2 + 1;
    for (size_t u = 0; u < votes_num; u++) {
        auto user = user_name(u);
        create_accounts({user});
        BOOST_CHECK_EQUAL(success(), point.open(user, symbol_code{}));
        BOOST_CHECK_EQUAL(success(), point.issue(user, asset(42, point._symbol), ""));
        BOOST_CHECK_EQUAL(success(), comm_ctrl.vote_leader(user, _alice));
        produce_block();
    }

    BOOST_CHECK_EQUAL(votes_num, comm_ctrl.get_leader(_alice)["counter_votes"].as<uint64_t>());
    BOOST_CHECK_EQUAL(votes_num * 42, comm_ctrl.get_leader(_alice)["total_weight"].as<uint64_t>());

    BOOST_CHECK_EQUAL(err.must_be_deactivated, comm_ctrl.clear_votes(_alice, cfg::max_clearvotes_count / 2));
    BOOST_CHECK_EQUAL(success(), comm_ctrl.stop_leader(_alice));
    BOOST_CHECK_EQUAL(success(), comm_ctrl.clear_votes(_alice, cfg::max_clearvotes_count / 2));
    BOOST_CHECK_EQUAL(votes_num - cfg::max_clearvotes_count / 2, comm_ctrl.get_leader(_alice)["counter_votes"].as<uint64_t>());
    BOOST_CHECK_EQUAL((votes_num - cfg::max_clearvotes_count / 2) * 42, comm_ctrl.get_leader(_alice)["total_weight"].as<uint64_t>());

    BOOST_CHECK_EQUAL(success(), comm_ctrl.clear_votes(_alice));
    BOOST_CHECK_EQUAL(1, comm_ctrl.get_leader(_alice)["counter_votes"].as<uint64_t>());
    BOOST_CHECK_EQUAL(42, comm_ctrl.get_leader(_alice)["total_weight"].as<uint64_t>());
    produce_block();

    BOOST_CHECK_EQUAL(err.there_are_votes, comm_ctrl.unreg_leader(_alice));
    BOOST_CHECK_EQUAL(success(), comm_ctrl.clear_votes(_alice));
    BOOST_CHECK_EQUAL(success(), comm_ctrl.unreg_leader(_alice));

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(leaders_reward_test, commun_ctrl_tester) try {
    BOOST_TEST_MESSAGE("leaders_reward_test");
    supply  = 5000000000000;
    reserve = 50000000000;
    init();
    BOOST_CHECK_EQUAL(success(), point.setparams(_golos, point.args()("transfer_fee", 0)("min_transfer_fee_points", 0)));

    produce_block();
    BOOST_CHECK_EQUAL(success(), point.open(_bob));
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _bob, asset(supply, point._symbol)));

    comm_ctrl.prepare({_bob}, _bob);
    BOOST_CHECK_EQUAL(comm_ctrl.get_all_leaders().size(), 1);

    produce_block();
    produce_block(fc::seconds(cfg::def_reward_leaders_period - 3 * block_interval));
    BOOST_CHECK_EQUAL(success(), comm_ctrl.unvote_leader(_bob, _bob));
    BOOST_CHECK_EQUAL(supply, point.get_supply());
    produce_block();
    BOOST_CHECK_EQUAL(success(), comm_ctrl.vote_leader(_bob, _bob));
    auto emitted = point.get_supply() - supply;
    BOOST_TEST_MESSAGE("--- emitted = " << emitted);
    BOOST_CHECK(emitted > 0);
    auto reward = comm_ctrl.get_unclaimed(_bob);
    BOOST_TEST_MESSAGE("--- reward = " << reward);
    auto retained = comm_ctrl.get_retained();
    BOOST_TEST_MESSAGE("--- retained = " << retained);
    BOOST_CHECK_EQUAL(reward + retained, emitted);
    BOOST_CHECK_EQUAL(reward, emitted / cfg::def_comm_leaders_num);

    BOOST_CHECK_EQUAL(success(), comm_ctrl.claim(_bob));
    BOOST_CHECK_EQUAL(point.get_amount(_bob), reward + supply);
    BOOST_CHECK_EQUAL(comm_ctrl.get_unclaimed(_bob), 0);
    produce_block();
    BOOST_CHECK_EQUAL(err.nothing_to_claim, comm_ctrl.claim(_bob));
    BOOST_CHECK_EQUAL(err.leader_not_found, comm_ctrl.claim(_alice));

    supply += emitted;
    BOOST_CHECK_EQUAL(success(), comm_ctrl.unvote_leader(_bob, _bob));
    produce_block();
    produce_block(fc::seconds(cfg::def_reward_leaders_period - 3 * block_interval));
    BOOST_CHECK_EQUAL(success(), point.open(_alice));
    BOOST_CHECK_EQUAL(success(), point.open(_carol));
    comm_ctrl.prepare(std::vector<account_name>(leaders.begin(), leaders.begin() + cfg::def_comm_leaders_num), _alice,
        cfg::def_comm_leaders_num * 10 * cfg::_1percent);
    BOOST_CHECK_EQUAL(success(), comm_ctrl.vote_leader(_carol, leaders[0], 10 * cfg::_1percent));
    produce_block();
    auto to_transfer = asset(point.get_amount(_bob) / 3 + 10, point._symbol);
    BOOST_CHECK_EQUAL(success(), point.transfer(_bob, _alice, to_transfer));
    BOOST_CHECK_EQUAL(success(), point.transfer(_bob, _carol, to_transfer));
    for (size_t i = 0; i < cfg::def_comm_leaders_num; i++) {
        BOOST_CHECK_EQUAL(err.nothing_to_claim, comm_ctrl.claim(leaders[i]));
    }
    BOOST_CHECK_EQUAL(success(), comm_ctrl.vote_leader(_bob, _bob, 10 * cfg::_1percent));
    emitted = point.get_supply() - supply;
    int64_t reward_sum = 0;
    for (size_t i = 0; i < cfg::def_comm_leaders_num; i++) {
        reward = comm_ctrl.get_unclaimed(leaders[i]);
        BOOST_TEST_MESSAGE("--- reward_"<< i << " = " << reward);

        //leader[0]'s reward is twice as much, because carol voted for him
        BOOST_CHECK_EQUAL(reward, (emitted + retained) * (i ? 1 : 2) / (cfg::def_comm_leaders_num + 1));

        BOOST_CHECK_EQUAL(success(), comm_ctrl.claim(leaders[i]));
        BOOST_CHECK_EQUAL(point.get_amount(leaders[i]), reward);
        BOOST_CHECK_EQUAL(comm_ctrl.get_unclaimed(leaders[i]), 0);
        reward_sum += reward;
    }
    BOOST_TEST_MESSAGE("--- reward_sum = " << reward_sum);
    auto new_retained = comm_ctrl.get_retained();
    BOOST_TEST_MESSAGE("--- new_retained = " << new_retained);
    BOOST_CHECK_EQUAL(emitted + retained - reward_sum, new_retained);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(emit_test, commun_ctrl_tester) try {
    BOOST_TEST_MESSAGE("emit_test");
    BOOST_CHECK_EQUAL(err.no_community, comm_ctrl.emit(_alice, {_alice, cfg::active_name}));

    init();
    BOOST_CHECK_EQUAL(err.missing_auth(_code), comm_ctrl.emit(_alice, {_alice, cfg::active_name}));
    BOOST_CHECK_EQUAL(err.it_isnt_time_for_reward, comm_ctrl.emit(_client, {_code, cfg::client_permission_name}));

    produce_block();
    produce_block(fc::seconds(cfg::def_reward_leaders_period));
    BOOST_CHECK_EQUAL(success(), comm_ctrl.emit(_client, {_code, cfg::client_permission_name}));
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
