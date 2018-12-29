#pragma once
#include "test_api_helper.hpp"

namespace commun { namespace config {
    
const uint32_t time_denom = 400;
const uint32_t checkwin_interval = 60 * 60 / time_denom;
const uint32_t min_time_from_last_win = 60 * 60 * 24 / time_denom;
const uint32_t min_time_from_last_bid_start = 60 * 60 * 24 / time_denom;
const uint32_t min_time_from_last_bid_step = 60 * 60 * 3 / time_denom;
const size_t max_bid_checks = 64;
const uint32_t bid_increment_denom = 10;
const int16_t bancor_creation_fee = 3000;

}}

namespace eosio { namespace testing {

struct registrar_api: base_contract_api {
    registrar_api(golos_tester* tester, name code)
    :   base_contract_api(tester, code) {}

    action_result checkwin() {
        return push(N(checkwin), _code, args());
    }
    
    action_result claimrefund(name bidder, symbol_code sym_code) {
        return push(N(claimrefund), bidder, args()
            ("bidder", bidder)
            ("sym_code", sym_code)
        );
    }
    
    action_result create(account_name owner, asset maximum_supply, int16_t cw, int16_t fee) {
        return push(N(create), owner, args()
            ("maximum_supply", maximum_supply)
            ("cw", cw)
            ("fee", fee)
        );
    }
    
    action_result setprice(account_name owner, symbol_code sym_code, asset price) {
        return push(N(setprice), owner, args()
            ("sym_code", sym_code)
            ("price", price)
        );
    }
    
    action_result set_param(const std::string& json_param) {
        return push(N(setparams), _code, args()
            ("params", json_str_to_obj(std::string() + "[" + json_param + "]"))
        );
    }
    action_result validate_params(const std::string& json_params) {
        return push(N(validateprms), _code, args()
            ("params", json_str_to_obj(json_params))
        );
    }
    
    static std::string token_param(symbol token) {
        return std::string() + "['token_param',{'sym':'"+token.to_string()+"'}]";
    }
    static std::string market_param(
        uint32_t bid_increment_denom = commun::config::bid_increment_denom, 
        int16_t bancor_creation_fee = commun::config::bancor_creation_fee
    ) {
        return std::string() + "['market_param',{"
            "'bid_increment_denom':"+std::to_string(bid_increment_denom) +
            ",'bancor_creation_fee':"+std::to_string(bancor_creation_fee) + "}]";
    }
    
    static std::string checkwin_param(
        uint32_t interval = commun::config::checkwin_interval,
        uint32_t min_time_from_last_win = commun::config::min_time_from_last_win,
        uint32_t min_time_from_last_bid_start = commun::config::min_time_from_last_bid_start,
        uint32_t min_time_from_last_bid_step = commun::config::min_time_from_last_bid_step,
        uint32_t max_bid_checks = commun::config::max_bid_checks
    ) {
        return std::string() + "['checkwin_param',{"
            "'interval':"+std::to_string(interval) +
            "'min_time_from_last_win':"+std::to_string(min_time_from_last_win) +
            "'min_time_from_last_bid_start':"+std::to_string(min_time_from_last_bid_start) +
            "'min_time_from_last_bid_step':"+std::to_string(min_time_from_last_bid_step) +
            ",'max_bid_checks':"+std::to_string(max_bid_checks) + "}]";
    }
    
    static std::string default_params(symbol token) {
        return std::string() + 
            //"[" +
            token_param(token) + "," +
            market_param() + "," +
            checkwin_param() 
            //+ "]"
        ;
    }
};

}} // eosio::testing
