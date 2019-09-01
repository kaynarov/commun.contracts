#include "golos_tester.hpp"
#include "commun_posting_test_api.hpp"
#include "commun.point_test_api.hpp"
#include "cyber.token_test_api.hpp"
#include "../commun.point/include/commun.point/config.hpp"
#include "../commun.emit/include/commun.emit/config.hpp"
#include "../commun.gallery/include/commun.gallery/config.hpp"
#include "contracts.hpp"
#include <commun/config.hpp>

namespace cfg = commun::config;
using namespace eosio::testing;
using namespace eosio::chain;
using namespace fc;
static const auto point_code_str = "GLS";
static const auto _point = symbol(3, point_code_str);
using commun::config::commun_point_name;
using commun::config::commun_emit_name;

class commun_publication_tester : public golos_tester {
protected:
    cyber_token_api token;
    commun_point_api point;
    commun_posting_api post;

    std::vector<account_name> _users;

public:
    commun_publication_tester()
        : golos_tester(cfg::publish_name)
        , token({this, cfg::token_name, cfg::reserve_token})
        , point({this, commun_point_name, _point})
        , post({this, cfg::publish_name, symbol(0, point_code_str).to_symbol_code()})
        , _users{_code, N(jackiechan), N(brucelee), N(chucknorris)} {
        create_accounts(_users);
        create_accounts({_commun, _golos, cfg::token_name, commun_point_name, commun_emit_name});
        produce_block();
        install_contract(commun_point_name, contracts::point_wasm(), contracts::point_abi());
        install_contract(commun_emit_name, contracts::emit_wasm(), contracts::emit_abi());
        install_contract(cfg::token_name, contracts::token_wasm(), contracts::token_abi());
        install_contract(cfg::publish_name, contracts::commun_publication_wasm(), contracts::commun_publication_abi());
        
        set_authority(commun_emit_name, cfg::mscsreward_perm_name, create_code_authority({_code}), "active");
        link_authority(commun_emit_name, commun_emit_name, cfg::mscsreward_perm_name, N(mscsreward));
        
        std::vector<account_name> transfer_perm_accs{_code, commun_emit_name};
        std::sort(transfer_perm_accs.begin(), transfer_perm_accs.end());
        set_authority(commun_point_name, cfg::issue_permission, create_code_authority({commun_emit_name}), "active");
        set_authority(commun_point_name, cfg::transfer_permission, create_code_authority(transfer_perm_accs), "active");
        
        link_authority(commun_point_name, commun_point_name, cfg::issue_permission, N(issue));
        link_authority(commun_point_name, commun_point_name, cfg::transfer_permission, N(transfer));
    }

    void init() {
        supply  = 5000000000000;
        reserve = 50000000000;
        uint16_t royalty = 2500;
        
        uint16_t annual_emission_rate = 1000;
        uint16_t leaders_reward_prop = 1000;
        
        BOOST_CHECK_EQUAL(success(), token.create(_commun, asset(reserve, token._symbol)));
        BOOST_CHECK_EQUAL(success(), token.issue(_commun, _golos, asset(reserve, token._symbol), ""));

        BOOST_CHECK_EQUAL(success(), point.create(_golos, asset(supply * 2, point._symbol), 10000, 1));
        BOOST_CHECK_EQUAL(success(), point.set_freezer(commun::config::commun_gallery_name));
        
        BOOST_CHECK_EQUAL(success(), push_action(commun_emit_name, N(create), commun_emit_name, mvo()
            ("commun_symbol", point._symbol)("annual_emission_rate", annual_emission_rate)("leaders_reward_prop", leaders_reward_prop)));
        
        BOOST_CHECK_EQUAL(success(), token.transfer(_golos, commun_point_name, asset(reserve, token._symbol), cfg::restock_prefix + point_code_str));
        BOOST_CHECK_EQUAL(success(), point.issue(_golos, _golos, asset(supply, point._symbol), std::string(point_code_str) + " issue"));
        BOOST_CHECK_EQUAL(success(), point.open(_code, point._symbol, _code));
        
        BOOST_CHECK_EQUAL(success(), post.init_default_params());
    }
    
    const account_name _commun = N(commun);
    const account_name _golos = N(golos);
    int64_t supply;
    int64_t reserve;

    struct errors: contract_error_messages {
        const string delete_children       = amsg("You can't delete comment with child comments.");
        const string no_permlink           = amsg("Permlink doesn't exist.");
        const string max_comment_depth     = amsg("publication::create_message: level > MAX_COMMENT_DEPTH");
        const string max_cmmnt_dpth_less_0 = amsg("Max comment depth must be greater than 0.");
        const string parent_no_message     = amsg("Parent message doesn't exist");
    } err;
};

BOOST_AUTO_TEST_SUITE(commun_publication_tests)

BOOST_FIXTURE_TEST_CASE(delete_message, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Delete message testing.");
    init();
    BOOST_CHECK_EQUAL(success(), point.open(N(brucelee), point._symbol, N(brucelee)));
    BOOST_CHECK_EQUAL(success(), point.open(N(jackiechan), point._symbol, N(jackiechan)));
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, N(brucelee), asset(supply / 2, point._symbol)));
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, N(jackiechan), asset(supply / 2, point._symbol)));

    BOOST_CHECK_EQUAL(success(), post.create_msg({N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(success(), post.create_msg({N(jackiechan), "child"}, {N(brucelee), "permlink"}));

    BOOST_TEST_MESSAGE("--- fail then delete non-existing post and post with child");
    BOOST_CHECK_EQUAL(err.no_permlink, post.delete_msg({N(jackiechan), "permlink1"}));
    BOOST_CHECK_EQUAL(err.delete_children, post.delete_msg({N(brucelee), "permlink"}));
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
