#include "golos_tester.hpp"
#include "commun.token_test_api.hpp"
#include "commun_list_test_api.hpp"
#include "contracts.hpp"
#include "../common/config.hpp"


namespace cfg = commun::config;
using namespace eosio::testing;
using namespace eosio::chain;
using namespace fc;
static const auto token_code_str = "GLS";
static const auto _token = symbol(3, token_code_str);

class commun_list_tester : public golos_tester {
protected:
    commun_token_api token;

public:
    commun_list_tester()
        : golos_tester(cfg::commun_list_name)
        , token({this, cfg::commun_token_name, _token})
    {
        create_accounts({_commun, _golos, _alice, _bob, _carol, _nicolas,
            cfg::registrar_name, cfg::token_name, cfg::bancor_name, cfg::commun_list_name});
        produce_block();
        install_contract(cfg::commun_list_name, contracts::commun_list_wasm(), contracts::commun_list_abi());
        install_contract(cfg::commun_token_name, contracts::commun_token_wasm(), contracts::commun_token_abi());
    }

    const account_name _commun = N(commun);
    const account_name _golos = N(golos);
    const account_name _alice = N(alice);
    const account_name _bob = N(bob);
    const account_name _carol = N(carol);
    const account_name _nicolas = N(nicolas);

    void create_token() {
        token.create( N(alice), token.make_asset(1000));
        auto stats = token.get_stats();
        BOOST_TEST_MESSAGE(fc::json::to_string(stats));
        produce_blocks(1);
    };
    
    auto get_cur_time() {
        return control->head_block_time().time_since_epoch();
    };

    struct errors: contract_error_messages {
        const string community_exists = amsg("insufficient bid");
    } err;
};

BOOST_AUTO_TEST_SUITE(community_list_tests)

BOOST_FIXTURE_TEST_CASE(create_commun_list, commun_list_tester) try {
    create_token();
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
