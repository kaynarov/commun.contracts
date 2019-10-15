#include "commun.point_test_api.hpp"
#include "commun.ctrl_test_api.hpp"
#include "cyber.token_test_api.hpp"
#include "commun.list_test_api.hpp"
#include "commun.emit_test_api.hpp"
#include "../commun.emit/include/commun.emit/config.hpp"
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
        create_accounts({cfg::dapp_name, _golos, _alice, _bob, _carol,
            cfg::token_name, cfg::list_name, cfg::control_name, cfg::point_name, cfg::emit_name});
        create_accounts(leaders);
        produce_block();
        install_contract(cfg::control_name, contracts::ctrl_wasm(), contracts::ctrl_abi());
        install_contract(cfg::token_name, contracts::token_wasm(), contracts::token_abi());
        install_contract(cfg::point_name, contracts::point_wasm(), contracts::point_abi());
        install_contract(cfg::list_name, contracts::list_wasm(), contracts::list_abi());
        install_contract(cfg::emit_name, contracts::emit_wasm(), contracts::emit_abi());
        
        set_authority(cfg::control_name, N(changepoints), create_code_authority({cfg::point_name}), "active");
        link_authority(cfg::control_name, cfg::control_name, N(changepoints), N(changepoints));
        
        set_authority(cfg::emit_name, cfg::reward_perm_name, create_code_authority({_code}), "active");
        link_authority(cfg::emit_name, cfg::emit_name, cfg::reward_perm_name, N(issuereward));
        
        set_authority(cfg::emit_name, N(create), create_code_authority({cfg::list_name}), "active");
        link_authority(cfg::emit_name, cfg::emit_name, N(create), N(create));
        
        set_authority(cfg::control_name, N(create), create_code_authority({cfg::list_name}), "active");
        link_authority(cfg::control_name, cfg::control_name, N(create), N(create));
        
        set_authority(cfg::point_name, cfg::issue_permission, create_code_authority({cfg::emit_name}), "active");
        set_authority(cfg::point_name, cfg::transfer_permission, create_code_authority({cfg::emit_name}), "active");

        link_authority(cfg::point_name, cfg::point_name, cfg::issue_permission, N(issue));
        link_authority(cfg::point_name, cfg::point_name, cfg::transfer_permission, N(transfer));
        
    }
    struct errors : contract_error_messages {
        const string not_a_leader(account_name leader) { return amsg((leader.to_string() + " is not a leader")); }
        const string approved = amsg("already approved");
        const string authorization_failed = amsg("transaction authorization failed");
        const string no_leaders = amsg("leaders num must be positive");
        const string no_votes = amsg("max votes must be positive");
        const string inconsistent_threshold = amsg("inconsistent set of threshold parameters");
        const string no_threshold = amsg("required threshold must be positive");
        const string leaders_less = amsg("leaders num can't be less than required threshold");
        const string threshold_greater = amsg("required threshold can't be greater than leaders num");
        const string nothing_to_claim = amsg("nothing to claim");
        const string leader_not_found = amsg("leader not found");
        
    } err;
    
    transaction get_point_create_trx(const vector<permission_level>& auths, name issuer, asset maximum_supply, int16_t cw, int16_t fee) {
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
                    ("maximum_supply", maximum_supply)
                    ("cw", cw)
                    ("fee", fee)
                )}));
       transaction trx;
       abi_serializer::from_variant(pretty_trx, trx, get_resolver(), abi_serializer_max_time);
       return trx;
    }
};

BOOST_AUTO_TEST_SUITE(ctrl_tests)

BOOST_FIXTURE_TEST_CASE(basic_tests, commun_ctrl_tester) try {
    BOOST_TEST_MESSAGE("basic_tests");
    
    BOOST_CHECK_EQUAL(success(), token.create(_golos, asset(42, token._symbol)));
    BOOST_CHECK_EQUAL(success(), token.issue(_golos, _bob, asset(42, token._symbol), ""));
    
    BOOST_CHECK_EQUAL(success(), community.setappparams(community.args()
        ("permission", config::active_name)("required_threshold", 11)));
    
    BOOST_CHECK_EQUAL(success(), point.open(_bob));
    BOOST_CHECK_EQUAL(success(), token.transfer(_bob, cfg::point_name, asset(42, token._symbol), ""));
    
    dapp_ctrl.prepare(leaders, _bob);
    set_authority(cfg::dapp_name, cfg::active_name, create_code_authority({cfg::control_name}), "owner");
    set_authority(cfg::point_name, cfg::active_name,
                  authority (1, {}, {{.permission = {cfg::dapp_name, config::active_name}, .weight = 1}}), "owner");
    
    produce_block();
    
    asset max_supply(999999, point._symbol);
    auto trx = get_point_create_trx({permission_level{cfg::point_name, cfg::active_name}},
        _golos, max_supply, 5000, 0);

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
    BOOST_CHECK_EQUAL(err.no_votes, community.setappparams(community.args()
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

BOOST_FIXTURE_TEST_CASE(leaders_reward_test, commun_ctrl_tester) try {
    BOOST_TEST_MESSAGE("leaders_reward_test");
    
    int64_t supply  = 5000000000000;
    int64_t reserve = 50000000000;
    
    BOOST_CHECK_EQUAL(success(), token.create(cfg::dapp_name, asset(reserve, token._symbol)));
    BOOST_CHECK_EQUAL(success(), token.issue(cfg::dapp_name, _golos, asset(reserve, token._symbol), ""));
    
    BOOST_CHECK_EQUAL(success(), point.create(_golos, asset(supply * 2, point._symbol), 10000, 1));
    BOOST_CHECK_EQUAL(success(), community.create(cfg::list_name, point_code, "GOLOS"));
    
    BOOST_CHECK_EQUAL(success(), token.transfer(_golos, cfg::point_name, asset(reserve, token._symbol), cfg::restock_prefix + point_code_str));
    BOOST_CHECK_EQUAL(success(), point.issue(_golos, _golos, asset(supply, point._symbol), std::string(point_code_str) + " issue"));
    BOOST_CHECK_EQUAL(success(), point.open(_code, point_code, _code));
    
    produce_block();
    BOOST_CHECK_EQUAL(success(), point.open(_bob, point_code));
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _bob, asset(supply, point._symbol)));
    
    comm_ctrl.prepare({_bob}, _bob);
    BOOST_CHECK_EQUAL(comm_ctrl.get_all_leaders().size(), 1);
    
    produce_block();
    produce_block(fc::seconds(cfg::reward_leaders_period - 3 * block_interval));
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
    produce_block(fc::seconds(cfg::reward_leaders_period - 3 * block_interval));
    BOOST_CHECK_EQUAL(success(), point.open(_alice, point_code));
    comm_ctrl.prepare(std::vector<account_name>(leaders.begin(), leaders.begin() + cfg::def_comm_leaders_num), _alice);
    produce_block();
    BOOST_CHECK_EQUAL(success(), point.transfer(_bob, _alice, asset(point.get_amount(_bob) / 2 + 1, point._symbol)));
    for (size_t i = 0; i < cfg::def_comm_leaders_num; i++) {
        BOOST_CHECK_EQUAL(err.nothing_to_claim, comm_ctrl.claim(leaders[i]));
    }
    BOOST_CHECK_EQUAL(success(), comm_ctrl.vote_leader(_bob, _bob));
    emitted = point.get_supply() - supply;
    int64_t reward_sum = 0;
    for (size_t i = 0; i < cfg::def_comm_leaders_num; i++) {
        reward = comm_ctrl.get_unclaimed(leaders[i]);
        BOOST_TEST_MESSAGE("--- reward_"<< i << " = " << reward);
        BOOST_CHECK_EQUAL(reward, (emitted + retained) / cfg::def_comm_leaders_num);
        
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

BOOST_AUTO_TEST_SUITE_END()
