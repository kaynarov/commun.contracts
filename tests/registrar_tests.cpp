#include "golos_tester.hpp"
#include "bancor.token_test_api.hpp"
#include "eosio.token_test_api.hpp"
#include "registrar_test_api.hpp"
#include "contracts.hpp"
#include "../bancor.token/include/bancor.token/config.hpp"
#include "../registrar/config.hpp"

namespace cfg = commun::config;
using namespace eosio::testing;
using namespace eosio::chain;
using namespace fc;
static const auto token_code_str = "GLS";
static const auto token2_code_str = "BOLIVAR";
static const auto _token = symbol(3, token_code_str);
static const auto _token2 = symbol(2, token2_code_str);

class registrar_tester : public golos_tester {
protected:
    eosio_token_api token;
    bancor_token_api bancor;
    registrar_api registrar;

public:
    registrar_tester()
        : golos_tester(cfg::registrar_name)
        , token({this, cfg::token_name, cfg::reserve_token})
        , bancor({this, cfg::bancor_name, _token})
        , registrar({this, _code})
    {
        create_accounts({_commun, _golos, _alice, _bob, _carol, _nicolas,
            cfg::registrar_name, cfg::token_name, cfg::bancor_name});
        produce_block();
        install_contract(_commun, contracts::reserve_emit_wasm(), contracts::reserve_emit_abi());
        install_contract(cfg::bancor_name, contracts::bancor_wasm(), contracts::bancor_abi());
        install_contract(cfg::token_name, contracts::token_wasm(), contracts::token_abi());
        install_contract(_code, contracts::registrar_wasm(), contracts::registrar_abi());
        bancor.add_creators({_code});
    }
    
    void set_default_params() { BOOST_CHECK_EQUAL(success(), registrar.set_param(registrar.default_params(token._symbol))); }

    const account_name _commun = N(commun);
    const account_name _golos = N(golos);
    const account_name _alice = N(alice);
    const account_name _bob = N(bob);
    const account_name _carol = N(carol);
    const account_name _nicolas = N(nicolas);
    
    auto get_cur_time() { return control->head_block_time().time_since_epoch();};

    struct errors: contract_error_messages {
        const string insufficient_bid = amsg("insufficient bid");
        const string no_key = amsg("unable to find key");
        const string not_closed = amsg("auction is not closed yet");
        const string closed = amsg("this auction has already closed");
        const string already_exists = amsg("bancor token already exists");
    } err;
};

asset next_bid(asset& prev) { return prev + asset(prev.get_amount() / cfg::bid_increment_denom + 1, prev.get_symbol()); }
asset& inc_bid(asset& arg)  { arg = next_bid(arg); return arg; }

BOOST_AUTO_TEST_SUITE(registrar_tests)

BOOST_FIXTURE_TEST_CASE(basic_tests, registrar_tester) try {
    BOOST_TEST_MESSAGE("Basic registrar tests");
    set_default_params();
    asset bid_a(100, token._symbol);
    asset bid_b(10, token._symbol);
    asset bid_c(1000, token._symbol);
    asset bid_b2(1500, token._symbol);
    asset bid_c2(2000, token._symbol);
    asset supply = bid_a + bid_b + bid_c + bid_b2 + bid_c2;
    BOOST_CHECK_EQUAL(success(), registrar.checkwin());

    BOOST_CHECK_EQUAL(success(), token.create(_commun, asset(1000000, token._symbol)));
    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _alice, bid_a, ""));
    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _bob,   bid_b + bid_b2, ""));
    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _carol, bid_c + bid_c2, ""));
    BOOST_TEST_MESSAGE("--- alice bids " << bid_a);
    BOOST_CHECK_EQUAL(success(), token.transfer(_alice, _code, bid_a, cfg::bid_prefix + token_code_str));
    BOOST_TEST_MESSAGE("--- bob is trying to bid " << bid_b);
    BOOST_CHECK_EQUAL(err.insufficient_bid, token.transfer(_bob, _code, bid_b, cfg::bid_prefix + token_code_str));
    asset insufficient_bid_c = next_bid(bid_a) - asset(1, token._symbol);
    BOOST_TEST_MESSAGE("--- carol is trying to bid " << insufficient_bid_c);
    BOOST_CHECK_EQUAL(err.insufficient_bid, token.transfer(_carol, _code, insufficient_bid_c, cfg::bid_prefix + token_code_str));
    BOOST_TEST_MESSAGE("--- carol bids " << bid_c);
    BOOST_CHECK_EQUAL(success(), token.transfer(_carol, _code, bid_c, cfg::bid_prefix + token_code_str));
    BOOST_CHECK_EQUAL(success(), token.close(_alice, token._symbol));
    BOOST_CHECK_EQUAL(err.no_key, registrar.claimrefund(_alice, bancor._symbol.to_symbol_code()));
    BOOST_CHECK_EQUAL(success(), token.open(_alice, token._symbol, _alice));
    BOOST_CHECK_EQUAL(success(), registrar.claimrefund(_alice, bancor._symbol.to_symbol_code()));
    BOOST_CHECK_EQUAL(err.not_closed, registrar.create(_carol, asset(100500, bancor._symbol), 10000, 0));
    
    BOOST_TEST_MESSAGE("--- waiting");
    
    produce_blocks(cfg::min_time_from_last_bid_start * 1000 / cfg::block_interval_ms + 1);
    BOOST_TEST_MESSAGE("--- bob is trying to bid " << bid_b2);
    BOOST_CHECK_EQUAL(err.closed, token.transfer(_bob, _code, bid_b2, cfg::bid_prefix + token_code_str));
    produce_block();
    BOOST_TEST_MESSAGE("--- carol sets price " << asset(bid_b2.get_amount() * 2, token._symbol));
    BOOST_CHECK_EQUAL(success(), registrar.setprice(_carol, bancor._symbol.to_symbol_code(), asset(bid_b2.get_amount() * 2, token._symbol)));
    BOOST_TEST_MESSAGE("--- bob is trying to bid " << bid_b2);
    BOOST_CHECK_EQUAL(err.insufficient_bid, token.transfer(_bob, _code, bid_b2, cfg::bid_prefix + token_code_str));
    produce_block();
    BOOST_TEST_MESSAGE("--- carol sets price " << bid_b2);
    BOOST_CHECK_EQUAL(success(), registrar.setprice(_carol, bancor._symbol.to_symbol_code(), bid_b2));
    BOOST_TEST_MESSAGE("--- bob bets " << bid_b2);
    BOOST_CHECK_EQUAL(success(), token.transfer(_bob, _code, bid_b2, cfg::bid_prefix + token_code_str));
    produce_block();
    BOOST_CHECK_EQUAL(token.get_account(_carol)["balance"].as<asset>(), bid_c2 + bid_b2);
    
    BOOST_CHECK_EQUAL(token.get_stats()["supply"].as<asset>(), supply);
    BOOST_TEST_MESSAGE("--- bob creates token");
    BOOST_CHECK_EQUAL(success(), registrar.create(_bob, asset(100500, bancor._symbol), 10000, 0));
    BOOST_TEST_MESSAGE("--- carol is trying to bid " << bid_c2);
    BOOST_CHECK_EQUAL(err.already_exists, token.transfer(_carol, _code, bid_c2, cfg::bid_prefix + token_code_str));
    produce_block();
    asset fee(static_cast<eosio::chain::int128_t>(bid_c.get_amount()) * cfg::bancor_creation_fee / cfg::_100percent, token._symbol);
    supply -= fee;
    BOOST_CHECK_EQUAL(token.get_stats()["supply"].as<asset>(), supply);
    BOOST_CHECK_EQUAL(bancor.get_stats()["reserve"].as<asset>(), bid_c - fee);
    
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(bids_queue_test, registrar_tester) try {
    BOOST_TEST_MESSAGE("Bids queue test");
    set_default_params();
    asset bid(100, token._symbol);
    asset bid2(10, token._symbol);
    BOOST_CHECK_EQUAL(success(), registrar.checkwin());

    BOOST_CHECK_EQUAL(success(), token.create(_commun, asset(999999, token._symbol)));
    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _alice,   asset(100500, token._symbol), ""));
    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _bob,     asset(100500, token._symbol), ""));
    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _carol,   asset(100500, token._symbol), ""));
    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _nicolas, asset(100500, token._symbol), ""));
    
    BOOST_TEST_MESSAGE("--- alice bids on " << token_code_str);
    BOOST_CHECK_EQUAL(success(), token.transfer(_alice, _code, bid, cfg::bid_prefix + token_code_str));
    BOOST_TEST_MESSAGE("--- alice bids on " << token2_code_str);
    BOOST_CHECK_EQUAL(success(), token.transfer(_alice, _code, bid2, cfg::bid_prefix + token2_code_str));
    BOOST_TEST_MESSAGE("--- bob bids on " << token_code_str);
    BOOST_CHECK_EQUAL(success(), token.transfer(_bob, _code, inc_bid(bid), cfg::bid_prefix + token_code_str));
    BOOST_TEST_MESSAGE("--- nicolas bids on " << token2_code_str);
    BOOST_CHECK_EQUAL(success(), token.transfer(_nicolas, _code, inc_bid(bid2), cfg::bid_prefix + token2_code_str));
    BOOST_TEST_MESSAGE("--- waiting");
    produce_blocks(cfg::min_time_from_last_bid_start * 1000 / cfg::block_interval_ms);
    BOOST_TEST_MESSAGE("--- carol bids on " << token_code_str);
    BOOST_CHECK_EQUAL(success(), token.transfer(_carol, _code, inc_bid(bid), cfg::bid_prefix + token_code_str));
    BOOST_TEST_MESSAGE("--- nicolas is trying to create " << token2_code_str);
    BOOST_CHECK_EQUAL(err.not_closed, registrar.create(_nicolas, asset(100500100500, _token2), 5000, 1000));
    BOOST_TEST_MESSAGE("--- waiting");
    produce_blocks(cfg::min_time_from_last_bid_step * 1000 / cfg::block_interval_ms);
    BOOST_TEST_MESSAGE("--- nicolas is trying to create " << token2_code_str);
    BOOST_CHECK_EQUAL(err.not_closed, registrar.create(_nicolas, asset(100500100500, _token2), 5000, 1000));
    produce_block();
    BOOST_TEST_MESSAGE("--- nicolas creates " << token2_code_str);
    BOOST_CHECK_EQUAL(success(), registrar.create(_nicolas, asset(100500100500, _token2), 5000, 1000));
    BOOST_TEST_MESSAGE("--- waiting");
    produce_blocks(cfg::min_time_from_last_win * 1000 / cfg::block_interval_ms - 1);
    BOOST_TEST_MESSAGE("--- carol is trying to create " << token_code_str);
    BOOST_CHECK_EQUAL(err.not_closed, registrar.create(_carol, asset(100500, bancor._symbol), 10000, 0));
    produce_block();
    BOOST_TEST_MESSAGE("--- carol creates " << token_code_str);
    BOOST_CHECK_EQUAL(success(), registrar.create(_carol, asset(100500, bancor._symbol), 10000, 0));

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(third_party_create_token_test, registrar_tester) try {
    BOOST_TEST_MESSAGE("Third party create token test");
    set_default_params();
    asset bid_a(100, token._symbol);
    asset bid_b(200, token._symbol);
    asset supply = bid_a + bid_b;
    BOOST_CHECK_EQUAL(success(), registrar.checkwin());
    BOOST_CHECK_EQUAL(success(), token.create(_commun, asset(1000000, token._symbol)));
    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _alice, bid_a, ""));
    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _bob,   bid_b, ""));
    BOOST_TEST_MESSAGE("--- alice bids");
    BOOST_CHECK_EQUAL(success(), token.transfer(_alice, _code, bid_a, cfg::bid_prefix + token_code_str));
    BOOST_TEST_MESSAGE("--- third party create token");
    BOOST_CHECK_EQUAL(success(), bancor.create(_carol, asset(100500, bancor._symbol), 10000, 0));
    BOOST_TEST_MESSAGE("--- bob is trying to bid");
    BOOST_CHECK_EQUAL(err.already_exists, token.transfer(_bob, _code, bid_b, cfg::bid_prefix + token_code_str));
    produce_block();
    BOOST_TEST_MESSAGE("--- alice is trying to create token");
    BOOST_CHECK_EQUAL(err.not_closed, registrar.create(_alice, asset(100500, bancor._symbol), 10000, 0));
    BOOST_TEST_MESSAGE("--- waiting");
    produce_blocks(cfg::min_time_from_last_bid_start * 1000 / cfg::block_interval_ms);
    BOOST_CHECK_EQUAL(token.get_account(_alice)["balance"].as<asset>(), asset(0, token._symbol));
    BOOST_TEST_MESSAGE("--- alice is trying to create token");
    BOOST_CHECK_EQUAL(success(), registrar.create(_alice, asset(100500, bancor._symbol), 10000, 0));
    BOOST_TEST_MESSAGE("--- and gets a refund");
    produce_block();
    BOOST_CHECK_EQUAL(token.get_account(_alice)["balance"].as<asset>(), bid_a);
    BOOST_CHECK_EQUAL(bancor.get_stats()["reserve"].as<asset>(), asset(0, token._symbol));
    BOOST_CHECK_EQUAL(token.get_stats()["supply"].as<asset>(), supply);

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
