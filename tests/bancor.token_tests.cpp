#include "golos_tester.hpp"
#include "bancor.token_test_api.hpp"
#include "cyber.token_test_api.hpp"
#include "contracts.hpp"
#include "../bancor.token/include/bancor.token/config.hpp"


namespace cfg = commun::config;
using namespace eosio::testing;
using namespace eosio::chain;
using namespace fc;
static const auto token_code_str = "GLS";
static const auto _token = symbol(3, token_code_str);
static const auto bancor_token_name = "bancor.token"_n;

class bancor_token_tester : public golos_tester {
protected:
    cyber_token_api token;
    bancor_token_api bancor;

public:
    bancor_token_tester()
        : golos_tester(bancor_token_name)
        , token({this, cfg::token_name, cfg::reserve_token})
        , bancor({this, _code, _token})
    {
        create_accounts({_commun, _golos, _alice, _bob, _carol,
            cfg::token_name, bancor_token_name});
        produce_block();

        install_contract(_code, contracts::bancor_wasm(), contracts::bancor_abi());
        install_contract(cfg::token_name, contracts::token_wasm(), contracts::token_abi());
    }

    const account_name _commun = N(commun);
    const account_name _golos = N(golos);
    const account_name _alice = N(alice);
    const account_name _bob = N(bob);
    const account_name _carol = N(carol);

    struct errors: contract_error_messages {
        const string no_reserve = amsg("token has no reserve");
    } err;
};

BOOST_AUTO_TEST_SUITE(bancor_token_tests)

BOOST_FIXTURE_TEST_CASE(basic_tests, bancor_token_tester) try {
    BOOST_TEST_MESSAGE("Basic bancor token tests");
    int64_t supply = 25000;
    int64_t reserve = 100000;
    int64_t init_balance = 10000;
    double fee = 0.01;
    BOOST_CHECK_EQUAL(success(), token.create(_commun, asset(1000000, token._symbol)));
    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _carol, asset(reserve, token._symbol), ""));
    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _alice, asset(init_balance, token._symbol), ""));
    
    BOOST_CHECK_EQUAL(success(), bancor.create(_golos, asset(999999, bancor._symbol), 10000, fee * cfg::_100percent));
    BOOST_CHECK_EQUAL(err.no_reserve, bancor.issue(_golos, _golos, asset(supply, bancor._symbol), std::string(token_code_str) + " issue"));
    BOOST_CHECK_EQUAL(err.no_reserve, token.transfer(_carol, _code, asset(reserve, token._symbol), token_code_str));
    BOOST_CHECK_EQUAL(success(), token.transfer(_carol, _code, asset(reserve, token._symbol), cfg::restock_prefix + token_code_str));
    BOOST_CHECK_EQUAL(success(), bancor.issue(_golos, _golos, asset(supply, bancor._symbol), std::string(token_code_str) + " issue"));
    
    int64_t price = 5000;
    int64_t amount = price * supply / reserve;
    supply += amount;
    reserve += price;
    BOOST_TEST_MESSAGE("--- alice buys " << amount  << " for " << price);
    BOOST_CHECK_EQUAL(success(), token.transfer(_alice, _code, asset(price, token._symbol), token_code_str));
    CHECK_MATCHING_OBJECT(bancor.get_account(_alice), mvo()("balance", asset(amount, bancor._symbol).to_string()));
    
    int64_t amount_sent = amount / 2;
    int64_t price_sent = (amount_sent * reserve / supply) * (1.0 - fee);
    BOOST_TEST_MESSAGE("--- alice sends " << amount_sent  << " to bob ");
    BOOST_CHECK_EQUAL(success(), bancor.transfer(_alice, _bob, asset(amount_sent, bancor._symbol)));
    BOOST_TEST_MESSAGE("--- bob sells " << amount_sent << " for " << price_sent);
    BOOST_CHECK_EQUAL(success(), bancor.transfer(_bob, _golos, asset(amount_sent, bancor._symbol)));
    CHECK_MATCHING_OBJECT(token.get_account(_bob), mvo()("balance", asset(price_sent, token._symbol).to_string()));
    
    supply -= amount_sent;
    reserve -= price_sent;
    int64_t amount_sell = amount / 4;
    int64_t price_sell = (amount_sell * reserve / supply) * (1.0 - fee);
    BOOST_TEST_MESSAGE("--- alice sells " << amount_sell << " for " << price_sell);
    BOOST_CHECK_EQUAL(success(), bancor.transfer(_alice, _golos, asset(amount_sell, bancor._symbol)));
    CHECK_MATCHING_OBJECT(token.get_account(_alice), mvo()("balance", asset(price_sell + (init_balance - price), token._symbol).to_string()));

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(cw05_test, bancor_token_tester) try {
    BOOST_TEST_MESSAGE("Bancor token test: cw = 0.5");
    
    int64_t init_supply = 200000;
    int64_t supply = init_supply;
    int64_t reserve = init_supply / 2;
    int64_t balance = reserve;
    BOOST_CHECK_EQUAL(success(), token.create(_commun, asset(1000000, token._symbol)));
    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _golos, asset(reserve, token._symbol), ""));
    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _alice, asset(balance, token._symbol), ""));
    
    BOOST_CHECK_EQUAL(success(), bancor.create(_golos, asset(999999, bancor._symbol), 5000, 0));
    BOOST_CHECK_EQUAL(success(), token.transfer(_golos, _code, asset(reserve, token._symbol), cfg::restock_prefix + token_code_str));
    BOOST_CHECK_EQUAL(success(), bancor.issue(_golos, _golos, asset(supply, bancor._symbol), std::string(token_code_str) + " issue"));

    size_t steps  = 7;
    int64_t bancor_balance = 0;
    int64_t price = balance / steps;
    for(size_t i = 0; i < steps; i++) {
        int64_t amount =  supply * sqrt(1.0 + static_cast<double>(price) / reserve) - supply;
        supply += amount;
        reserve += price;
        bancor_balance += amount;
        balance -= price;
        BOOST_TEST_MESSAGE("--- alice buys " << amount  << " for " << price);
        BOOST_CHECK_EQUAL(success(), token.transfer(_alice, _code, asset(price, token._symbol), token_code_str));
        CHECK_MATCHING_OBJECT(bancor.get_account(_alice), mvo()("balance", asset(bancor_balance, bancor._symbol).to_string()));
        CHECK_MATCHING_OBJECT(token.get_account(_alice), mvo()("balance", asset(balance, token._symbol).to_string()));
    }
    
    bancor_balance += init_supply;
    BOOST_CHECK_EQUAL(success(), bancor.transfer(_golos, _alice, asset(init_supply, bancor._symbol)));
    
    int64_t sell_step = bancor_balance / (steps - 1);
    for(size_t i = 0; i < steps; i++) {
        int64_t amount = std::min(sell_step, bancor_balance);
        int64_t price = reserve * (1.0 - std::pow(1.0 - static_cast<double>(amount) / supply, 2.0));
        supply -= amount;
        reserve -= price;
        bancor_balance -= amount;
        balance += price;
        BOOST_TEST_MESSAGE("--- alice sells " << amount << " for " << price);
        BOOST_CHECK_EQUAL(success(), bancor.transfer(_alice, _golos, asset(amount, bancor._symbol)));
        CHECK_MATCHING_OBJECT(bancor.get_account(_alice), mvo()("balance", asset(bancor_balance, bancor._symbol).to_string()));
        CHECK_MATCHING_OBJECT(token.get_account(_alice), mvo()("balance", asset(balance, token._symbol).to_string()));
    }
    BOOST_CHECK_EQUAL(reserve, 0);
    
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
