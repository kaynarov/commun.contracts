#include "golos_tester.hpp"
#include "commun_posting_test_api.hpp"
#include "contracts.hpp"
#include <commun/config.hpp>


namespace cfg = commun::config;
using namespace eosio::testing;
using namespace eosio::chain;
using namespace fc;
static const auto point_code_str = "GLS";

class commun_publication_tester : public golos_tester {
protected:
    commun_posting_api post;

    std::vector<account_name> _users;

public:
    commun_publication_tester()
        : golos_tester(cfg::publish_name)
        , post({this, cfg::publish_name, symbol(0, point_code_str).to_symbol_code()})
        , _users{_code, N(jackiechan), N(brucelee), N(chucknorris)} {
        create_accounts(_users);
        produce_block();
        install_contract(cfg::publish_name, contracts::commun_publication_wasm(), contracts::commun_publication_abi());
    }

    void init() {
        BOOST_CHECK_EQUAL(success(), post.init_default_params());
    }

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

    BOOST_CHECK_EQUAL(success(), post.create_msg({N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(success(), post.create_msg({N(jackiechan), "child"}, {N(brucelee), "permlink"}));

    BOOST_TEST_MESSAGE("--- fail then delete non-existing post and post with child");
    BOOST_CHECK_EQUAL(err.no_permlink, post.delete_msg({N(jackiechan), "permlink1"}));
    BOOST_CHECK_EQUAL(err.delete_children, post.delete_msg({N(brucelee), "permlink"}));
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
