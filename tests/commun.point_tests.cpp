#include "golos_tester.hpp"
#include "commun.point_test_api.hpp"
#include "cyber.token_test_api.hpp"
#include "contracts.hpp"
#include "../commun.point/include/commun.point/config.hpp"


namespace cfg = commun::config;
using namespace eosio::testing;
using namespace eosio::chain;
using namespace fc;
static const auto point_code_str = "GLS";
static const auto _point = symbol(3, point_code_str);
static const auto point_code = _point.to_symbol_code();

class commun_point_tester : public golos_tester {
protected:
    cyber_token_api token;
    commun_point_api point;

public:
    commun_point_tester()
        : golos_tester(cfg::point_name)
        , token({this, cfg::token_name, cfg::reserve_token})
        , point({this, _code, _point}) {
        create_accounts({_commun, _golos, _alice, _bob, _carol,
            cfg::token_name, cfg::point_name, cfg::gallery_name});
        produce_block();
        install_contract(cfg::token_name, contracts::token_wasm(), contracts::token_abi());
        install_contract(_code, contracts::point_wasm(), contracts::point_abi());
    }

    void init() {
        int64_t supply = 25000;
        int64_t reserve = 100000;
        double fee = 0.01;
        BOOST_CHECK_EQUAL(success(), token.create(_commun, asset(1000000, token._symbol)));
        BOOST_CHECK_EQUAL(success(), token.issue(_commun, _carol, asset(reserve, token._symbol), ""));
        BOOST_CHECK_EQUAL(success(), point.create(_golos, asset(999999, point._symbol), 10000, fee * cfg::_100percent));
        BOOST_CHECK_EQUAL(success(), point.setparams(_golos, 0, 0));
        BOOST_CHECK_EQUAL(success(), token.transfer(_carol, _code, asset(reserve, token._symbol), cfg::restock_prefix + point_code_str));
        BOOST_CHECK_EQUAL(success(), point.issue(_golos, _golos, asset(supply, point._symbol), "issue"));
    }

    const account_name _commun = cfg::dapp_name;
    const account_name _golos = N(golos);
    const account_name _alice = N(alice);
    const account_name _bob = N(bob);
    const account_name _carol = N(carol);

    struct errors: contract_error_messages {
        const string invalid_symbol = amsg("invalid symbol name");
        const string invalid_supply = amsg("invalid supply");
        const string wrong_issuer = amsg("issuer account does not exist");
        const string wrong_owner = amsg("owner account does not exist");
        const string max_supply_not_positive = amsg("max-supply must be positive");
        const string invalid_cw = amsg("connector weight must be between 0.01% and 100% (1-10000)");
        const string invalid_fee = amsg("fee must be between 0% and 100% (0-10000)");
        const string point_already_exists = amsg("point already exists");
        const string issuer_already_has_point = amsg("issuer already has a point");
        const string memo_too_long = amsg("memo has more than 256 bytes");
        const string no_point_symbol = amsg("point with symbol does not exist, create it before issue");
        const string no_symbol = amsg("symbol does not exist");
        const string no_auth = amsg("missing required signature");
        const string no_reserve = amsg("no reserve");
        const string no_balance = amsg("no balance object found");
        const string balance_of_from_not_opened = amsg("balance of from not opened");
        const string overdrawn_balance = amsg("overdrawn balance");
        const string tokens_cost_zero_points = amsg("these tokens cost zero points");
        const string invalid_quantity = amsg("invalid quantity");
        const string quantity_not_positive = amsg("must issue positive quantity");
        const string symbol_precision = amsg("symbol precision mismatch");
        const string quantity_exceeds_supply = amsg("quantity exceeds available supply");
        const string issuer_cant_close = amsg("issuer can't close");
        const string balance_not_exists = amsg("Balance row already deleted or never existed. Action won't have any effect.");
        const string to_not_exists = amsg("to account does not exist");
        const string freezer_not_exists = amsg("freezer account does not exist");
        const string not_reserve_symbol = amsg("invalid reserve token symbol");

        const string min_transfer_fee_points_negative = amsg("min_transfer_fee_points cannot be negative");
        const string min_transfer_fee_points_zero = amsg("min_transfer_fee_points cannot be 0 if transfer_fee set");
        const string transfer_fee_gt_100 = amsg("transfer_fee can't be greater than 100%");
    } err;
};

BOOST_AUTO_TEST_SUITE(commun_point_tests)

BOOST_FIXTURE_TEST_CASE(basic_tests, commun_point_tester) try {
    BOOST_TEST_MESSAGE("Basic point tests");
    int64_t supply = 25000;
    int64_t reserve = 100000;
    int64_t init_balance = 10000;
    double fee = 0.01;
    BOOST_CHECK_EQUAL(success(), token.create(_commun, asset(1000000, token._symbol)));
    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _carol, asset(reserve, token._symbol), ""));
    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _alice, asset(init_balance, token._symbol), ""));

    BOOST_CHECK_EQUAL(success(), point.create(_golos, asset(999999, point._symbol), 10000, fee * cfg::_100percent));
    BOOST_CHECK_EQUAL(success(), point.setparams(_golos, 0, 0));
    BOOST_CHECK_EQUAL(err.no_reserve, point.issue(_golos, _golos, asset(supply, point._symbol), std::string(point_code_str) + " issue"));
    BOOST_CHECK_EQUAL(err.no_reserve, token.transfer(_carol, _code, asset(reserve, token._symbol), point_code_str));
    BOOST_CHECK_EQUAL(success(), token.transfer(_carol, _code, asset(reserve, token._symbol), cfg::restock_prefix + point_code_str));
    BOOST_CHECK_EQUAL(success(), point.issue(_golos, _golos, asset(supply, point._symbol), std::string(point_code_str) + " issue"));

    int64_t price = 5000;
    int64_t amount = price * supply / reserve;
    supply += amount;
    reserve += price;
    BOOST_TEST_MESSAGE("--- alice buys " << amount  << " for " << price);
    BOOST_CHECK_EQUAL(success(), point.open(_alice, point_code, _alice));
    BOOST_CHECK_EQUAL(success(), token.transfer(_alice, _code, asset(price, token._symbol), point_code_str));
    BOOST_CHECK_EQUAL(point.get_amount(_alice), amount);

    int64_t amount_sent = amount / 2;
    int64_t price_sent = (amount_sent * reserve / supply) * (1.0 - fee);
    BOOST_TEST_MESSAGE("--- alice sends " << amount_sent  << " to bob ");
    BOOST_CHECK_EQUAL(success(), point.transfer(_alice, _bob, asset(amount_sent, point._symbol)));
    BOOST_TEST_MESSAGE("--- bob sells " << amount_sent << " for " << price_sent);
    BOOST_CHECK_EQUAL(success(), point.transfer(_bob, _code, asset(amount_sent, point._symbol)));
    CHECK_MATCHING_OBJECT(token.get_account(_bob), mvo()("balance", asset(price_sent, token._symbol).to_string()));

    supply -= amount_sent;
    reserve -= price_sent;
    int64_t amount_sell = amount / 4;
    int64_t price_sell = (amount_sell * reserve / supply) * (1.0 - fee);
    BOOST_TEST_MESSAGE("--- alice sells " << amount_sell << " for " << price_sell);
    BOOST_CHECK_EQUAL(success(), point.transfer(_alice, _code, asset(amount_sell, point._symbol)));
    CHECK_MATCHING_OBJECT(token.get_account(_alice), mvo()("balance", asset(price_sell + (init_balance - price), token._symbol).to_string()));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(cw05_test, commun_point_tester) try {
    BOOST_TEST_MESSAGE("Bancor token test: cw = 0.5");

    int64_t init_supply = 200000;
    int64_t supply = init_supply;
    int64_t reserve = init_supply / 2;
    int64_t balance = reserve;
    BOOST_CHECK_EQUAL(success(), token.create(_commun, asset(1000000, token._symbol)));
    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _golos, asset(reserve, token._symbol), ""));
    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _alice, asset(balance, token._symbol), ""));

    BOOST_CHECK_EQUAL(success(), point.create(_golos, asset(999999, point._symbol), 5000, 0));
    BOOST_CHECK_EQUAL(success(), point.setparams(_golos, 0, 0));
    BOOST_CHECK_EQUAL(success(), token.transfer(_golos, _code, asset(reserve, token._symbol), cfg::restock_prefix + point_code_str));
    BOOST_CHECK_EQUAL(success(), point.issue(_golos, _golos, asset(supply, point._symbol), std::string(point_code_str) + " issue"));

    size_t steps  = 7;
    int64_t point_balance = 0;
    int64_t price = balance / steps;
    BOOST_CHECK_EQUAL(success(), point.open(_alice, point_code, _alice));
    for (size_t i = 0; i < steps; i++) {
        int64_t amount =  supply * sqrt(1.0 + static_cast<double>(price) / reserve) - supply;
        supply += amount;
        reserve += price;
        point_balance += amount;
        balance -= price;
        BOOST_TEST_MESSAGE("--- alice buys " << amount  << " for " << price);
        BOOST_CHECK_EQUAL(success(), token.transfer(_alice, _code, asset(price, token._symbol), point_code_str));
        BOOST_CHECK_EQUAL(point.get_amount(_alice), point_balance);
        CHECK_MATCHING_OBJECT(token.get_account(_alice), mvo()("balance", asset(balance, token._symbol).to_string()));
        produce_block();
    }

    point_balance += init_supply;
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _alice, asset(init_supply, point._symbol)));

    int64_t sell_step = point_balance / (steps - 1);
    for (size_t i = 0; i < steps; i++) {
        int64_t amount = std::min(sell_step, point_balance);
        int64_t price = reserve * (1.0 - std::pow(1.0 - static_cast<double>(amount) / supply, 2.0));
        supply -= amount;
        reserve -= price;
        point_balance -= amount;
        balance += price;
        BOOST_TEST_MESSAGE("--- alice sells " << amount << " for " << price);
        BOOST_CHECK_EQUAL(success(), point.transfer(_alice, _code, asset(amount, point._symbol)));
        BOOST_CHECK_EQUAL(point.get_amount(_alice), point_balance);
        CHECK_MATCHING_OBJECT(token.get_account(_alice), mvo()("balance", asset(balance, token._symbol).to_string()));
        produce_block();
    }
    BOOST_CHECK_EQUAL(reserve, 0);
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(create_tests, commun_point_tester) try {
    BOOST_TEST_MESSAGE("create tests");
    auto max_supply = asset(999999, point._symbol);
    auto another = symbol(3, "ANOTHER");

    BOOST_CHECK_EQUAL(err.max_supply_not_positive, point.create(_golos, asset(-999999, point._symbol), 5000, 0));
    BOOST_CHECK_EQUAL(err.invalid_cw, point.create(_golos, max_supply, 0, 0));
    BOOST_CHECK_EQUAL(err.invalid_cw, point.create(_golos, max_supply, 10001, 0));
    BOOST_CHECK_EQUAL(err.invalid_fee, point.create(_golos, max_supply, 5000, 10001));
    // TODO: invalid symbol and asset
    BOOST_CHECK_EQUAL(err.wrong_issuer, point.create(N(notexist), max_supply, 5000, 0));

    BOOST_CHECK_EQUAL(success(), point.create(_golos, max_supply, 5000, 0));
    produce_block();
    BOOST_CHECK_EQUAL(err.point_already_exists, point.create(_golos, max_supply, 5000, 0));
    BOOST_CHECK_EQUAL(err.point_already_exists, point.create(_alice, max_supply, 5000, 0));
    BOOST_CHECK_EQUAL(err.issuer_already_has_point, point.create(_golos, asset(1000, another), 5000, 0));
    BOOST_CHECK_EQUAL(success(), point.create(_alice, asset(1000, another), 5000, 0));
    produce_block();

    CHECK_MATCHING_OBJECT(point.get_params(), mvo()
        ("cw", 5000)
        ("fee", 0)
        ("max_supply", "999.999 GLS")
        ("issuer", _golos.to_string())
    );
    CHECK_MATCHING_OBJECT(point.get_params(another), mvo()
        ("cw", 5000)
        ("fee", 0)
        ("max_supply", "1.000 ANOTHER")
        ("issuer", _alice.to_string())
    );
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(issue_tests, commun_point_tester) try {
    BOOST_TEST_MESSAGE("issue tests");
    auto max_supply = asset(999999, point._symbol);
    auto supply =  asset(25000, point._symbol);
    auto reserve = asset(100000, token._symbol);
    BOOST_CHECK_EQUAL(err.no_point_symbol, point.issue(_golos, _golos, supply, "issue"));
    BOOST_CHECK_EQUAL(success(), point.create(_golos, max_supply, 10000, 0));
    BOOST_CHECK_EQUAL(err.no_reserve, point.issue(_golos, _golos, supply, "issue"));

    BOOST_CHECK_EQUAL(success(), token.create(_commun, asset(1000000, token._symbol)));
    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _code, reserve, cfg::restock_prefix + point_code_str));

    BOOST_CHECK_EQUAL(err.quantity_not_positive, point.issue(_golos, _golos, asset(-25000, point._symbol), "issue"));
    BOOST_CHECK_EQUAL(err.quantity_exceeds_supply, point.issue(_golos, _golos, asset(9999990, point._symbol), "issue"));
    BOOST_CHECK_EQUAL(err.symbol_precision, point.issue(_golos, _golos, asset(supply.get_amount(), symbol(0, "GLS")), "issue"));

    BOOST_CHECK_EQUAL(success(), point.issue(_golos, _golos, supply, "issue"));
    CHECK_MATCHING_OBJECT(point.get_stats(), mvo()
        ("supply", "25.000 GLS")
        ("reserve", "10.0000 COMMUN")
    );
    BOOST_CHECK_EQUAL(point.get_amount(_golos), supply.get_amount());
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(open_tests, commun_point_tester) try {
    BOOST_TEST_MESSAGE("open tests");

    BOOST_CHECK_EQUAL(err.no_symbol, point.open(_alice, point_code, _alice));
    init();
    BOOST_CHECK_EQUAL(err.wrong_owner, point.open(N(notexist), point_code, _alice));
    BOOST_CHECK_EQUAL(success(), point.open(_alice, point_code, _alice));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(close_tests, commun_point_tester) try {
    BOOST_TEST_MESSAGE("close tests");

    init();
    BOOST_CHECK_EQUAL(err.issuer_cant_close, point.close(_golos, point_code));
    BOOST_CHECK_EQUAL(err.balance_not_exists, point.close(_alice, point_code));
    BOOST_CHECK_EQUAL(success(), point.open(_alice, point_code, _alice));
    BOOST_CHECK_EQUAL(success(), point.close(_alice, point_code));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(transfer_tests, commun_point_tester) try {
    BOOST_TEST_MESSAGE("transfer tests");

    init();
    BOOST_CHECK_EQUAL(err.to_not_exists, point.transfer(_golos, N(notexist), asset(1000, point._symbol)));

    BOOST_CHECK_EQUAL(0, point.get_amount(_alice));
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _alice, asset(1000, point._symbol)));
    BOOST_CHECK_EQUAL(1000, point.get_amount(_alice));
    BOOST_CHECK_EQUAL(success(), point.transfer(_alice, _bob, asset(700, point._symbol)));
    BOOST_CHECK_EQUAL(300, point.get_amount(_alice));
    BOOST_CHECK_EQUAL(700, point.get_amount(_bob));

    BOOST_CHECK_EQUAL(err.freezer_not_exists, point.setfreezer(N(notexist)));
    BOOST_CHECK_EQUAL(success(), point.setfreezer(cfg::gallery_name));
    BOOST_CHECK_EQUAL(point.get_global_params()["point_freezer"], cfg::gallery_name.to_string());

    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _carol, asset(2000, token._symbol), ""));
    BOOST_CHECK_EQUAL(err.balance_of_from_not_opened, token.transfer(_carol, _code, asset(1000, token._symbol), point_code_str));
    BOOST_CHECK_EQUAL(success(), point.open(_carol, point_code, _carol));
    BOOST_CHECK_EQUAL(success(), token.transfer(_carol, _code, asset(1000, token._symbol), point_code_str));
    BOOST_CHECK_EQUAL(err.balance_of_from_not_opened, token.transfer(_carol, _code, asset(1000, token._symbol), ""));
    BOOST_CHECK_EQUAL(success(), point.open(_carol, symbol_code(), _carol));
    BOOST_CHECK_EQUAL(success(), token.transfer(_carol, _code, asset(1000, token._symbol), ""));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(transfer_fee_tests, commun_point_tester) try {
    BOOST_TEST_MESSAGE("transfer with fee tests");
    BOOST_CHECK_EQUAL(err.no_symbol, point.setparams(_golos, cfg::_100percent, 1));

    BOOST_CHECK_EQUAL(success(), token.create(_commun, asset(1+1000, token._symbol)));

    BOOST_CHECK_EQUAL(success(), point.create(_golos, asset(999999, point._symbol), 10000, 0));

    BOOST_CHECK_EQUAL(cfg::def_transfer_fee, 10);
    CHECK_MATCHING_OBJECT(point.get_params(), mvo()
        ("transfer_fee", cfg::def_transfer_fee)
        ("min_transfer_fee_points", cfg::def_min_transfer_fee_points)
    );

    BOOST_CHECK_EQUAL(err.min_transfer_fee_points_negative, point.setparams(_golos, 10000, -1));
    BOOST_CHECK_EQUAL(err.min_transfer_fee_points_zero, point.setparams(_golos, 10000, 0));
    BOOST_CHECK_EQUAL(err.transfer_fee_gt_100, point.setparams(_golos, cfg::_100percent+1, 1));
    BOOST_CHECK_EQUAL(success(), point.setparams(_golos, cfg::_100percent, 1));
    CHECK_MATCHING_OBJECT(point.get_params(), mvo()
        ("transfer_fee", cfg::_100percent)
        ("min_transfer_fee_points", 1)
    );
    BOOST_CHECK_EQUAL(success(), point.setparams(_golos, 10*cfg::_1percent, 1));

    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _carol, asset(1, token._symbol), ""));
    BOOST_CHECK_EQUAL(success(), token.transfer(_carol, _code, asset(1, token._symbol), cfg::restock_prefix + point_code_str));
    BOOST_CHECK_EQUAL(success(), point.issue(_golos, _golos, asset(1000, point._symbol), std::string(point_code_str) + " issue"));
    BOOST_CHECK_EQUAL(point.get_supply(), 1000);
    BOOST_CHECK_EQUAL(success(), point.transfer(_golos, _alice, asset(1000, point._symbol)));
    BOOST_CHECK_EQUAL(point.get_amount(_alice), 1000); // no fee if from == issuer

    BOOST_CHECK_EQUAL(success(), point.transfer(_alice, _bob, asset(500, point._symbol)));
    BOOST_CHECK_EQUAL(point.get_amount(_alice), 450);
    BOOST_CHECK_EQUAL(point.get_amount(_bob), 500);
    BOOST_CHECK_EQUAL(point.get_supply(), 450+500);

    BOOST_CHECK_EQUAL(success(), point.setparams(_golos, 0, 0));
    BOOST_CHECK_EQUAL(success(), point.transfer(_bob, _alice, asset(500, point._symbol)));
    BOOST_CHECK_EQUAL(point.get_amount(_alice), 450+500);
    BOOST_CHECK_EQUAL(point.get_amount(_bob), 0);
    BOOST_CHECK_EQUAL(point.get_supply(), 450+500);

    BOOST_CHECK_EQUAL(success(), point.setparams(_golos, cfg::_1percent, 1));
    BOOST_CHECK_EQUAL(err.overdrawn_balance, point.transfer(_alice, _bob, asset(450+500, point._symbol)));

    BOOST_CHECK_EQUAL(success(), point.setparams(_golos, cfg::_1percent, 300));
    BOOST_CHECK_EQUAL(success(), point.transfer(_alice, _bob, asset(1, point._symbol)));
    BOOST_CHECK_EQUAL(point.get_amount(_alice), 450+500-301);
    BOOST_CHECK_EQUAL(point.get_amount(_bob), 1);
    BOOST_CHECK_EQUAL(point.get_supply(), 450+500-300);

    auto _prefixtest = N(c.prefixtest);
    create_accounts({_prefixtest});
    BOOST_CHECK_EQUAL(success(), point.open(_prefixtest, point_code, _prefixtest));
    BOOST_CHECK_EQUAL(success(), point.transfer(_bob, _prefixtest, asset(1, point._symbol)));
    BOOST_CHECK_EQUAL(point.get_amount(_prefixtest), 1);
    BOOST_CHECK_EQUAL(point.get_amount(_bob), 0);
    BOOST_CHECK_EQUAL(point.get_supply(), 450+500-300); // same
    BOOST_CHECK_EQUAL(success(), point.transfer(_prefixtest, _bob, asset(1, point._symbol)));
    BOOST_CHECK_EQUAL(point.get_amount(_prefixtest), 0);
    BOOST_CHECK_EQUAL(point.get_amount(_bob), 1);
    BOOST_CHECK_EQUAL(point.get_supply(), 450+500-300); // same
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(transfer_buy_tokens_no_supply, commun_point_tester) try {
    BOOST_TEST_MESSAGE("Buy points if no supply");

    BOOST_CHECK_EQUAL(success(), point.create(_golos, asset(1000000, point._symbol), cfg::_100percent, 0));
    BOOST_CHECK_EQUAL(success(), token.create(_commun, asset(1+1000, token._symbol)));

    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _carol, asset(1+1000, token._symbol), ""));
    BOOST_CHECK_EQUAL(success(), token.transfer(_carol, _code, asset(1, token._symbol), cfg::restock_prefix + point_code_str));
    BOOST_CHECK_EQUAL(success(), point.open(_carol, point_code, _carol));
    BOOST_CHECK_EQUAL(err.tokens_cost_zero_points, token.transfer(_carol, _code, asset(1000, token._symbol), point_code_str));

    BOOST_CHECK_EQUAL(success(), point.issue(_golos, _golos, asset(1, point._symbol), std::string(point_code_str) + " issue"));
    BOOST_CHECK_EQUAL(success(), token.transfer(_carol, _code, asset(1000, token._symbol), point_code_str));
    BOOST_CHECK_EQUAL(point.get_amount(_carol), 1000);
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(withdraw_tests, commun_point_tester) try {
    BOOST_TEST_MESSAGE("withdraw tests");
    init();

    BOOST_CHECK_EQUAL(success(), point.open(_alice, point_code, _alice));
    BOOST_CHECK_EQUAL(err.not_reserve_symbol, point.withdraw(_alice, asset(100, _point)));
    BOOST_CHECK_EQUAL(err.no_balance, point.withdraw(_alice, asset(100, cfg::reserve_token)));
    BOOST_CHECK_EQUAL(success(), point.open(_alice, symbol_code(), _alice));
    BOOST_CHECK_EQUAL(err.overdrawn_balance, point.withdraw(_alice, asset(100, cfg::reserve_token)));

    BOOST_CHECK_EQUAL(success(), token.issue(_commun, _alice, asset(1000, token._symbol), ""));
    BOOST_CHECK_EQUAL(success(), token.transfer(_alice, _code, asset(300, token._symbol), ""));
    BOOST_CHECK_EQUAL(success(), point.withdraw(_alice, asset(100, cfg::reserve_token)));
    BOOST_CHECK_EQUAL(token.get_account(_alice)["balance"], asset(700+100, token._symbol).to_string());
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
