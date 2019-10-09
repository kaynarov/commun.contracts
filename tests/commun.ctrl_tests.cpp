#include "commun.point_test_api.hpp"
#include "commun.ctrl_test_api.hpp"
#include "cyber.token_test_api.hpp"
#include "commun.list_test_api.hpp"
#include "contracts.hpp"

namespace cfg = commun::config;
using namespace eosio::testing;
using namespace eosio::chain;
using namespace fc;
static const auto point_code_str = "GLS";
static const auto _point = symbol(3, point_code_str);
static const auto point_code = _point.to_symbol_code();

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
public:
    commun_ctrl_tester()
        : golos_tester(cfg::control_name)
        , token({this, cfg::token_name, cfg::reserve_token})
        , point({this, cfg::point_name, _point})
        , community({this, cfg::list_name})
        , dapp_ctrl({this, cfg::control_name, symbol_code(), cfg::dapp_name})
        , comm_ctrl({this, cfg::control_name, _point.to_symbol_code(), _golos})
    {
        create_accounts({cfg::dapp_name, _golos, _alice, _bob, _carol,
            cfg::token_name, cfg::list_name, cfg::control_name, cfg::point_name});
        create_accounts(leaders);
        produce_block();
        install_contract(cfg::control_name, contracts::ctrl_wasm(), contracts::ctrl_abi());
        install_contract(cfg::token_name, contracts::token_wasm(), contracts::token_abi());
        install_contract(cfg::point_name, contracts::point_wasm(), contracts::point_abi());
        install_contract(cfg::list_name, contracts::list_wasm(), contracts::list_abi());
        
        set_authority(cfg::control_name, N(changepoints), create_code_authority({cfg::point_name}), "active");
        link_authority(cfg::control_name, cfg::control_name, N(changepoints), N(changepoints));
        
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

BOOST_AUTO_TEST_SUITE_END()
