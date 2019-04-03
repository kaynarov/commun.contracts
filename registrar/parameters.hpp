#pragma once
#include <commun/parameter.hpp>
#include <commun/config.hpp>
#include <eosiolib/singleton.hpp>
namespace commun { namespace param {
using eosio::symbol_code;
    
struct token : immutable_parameter {
    symbol sym;
    void validate() const override {
        eosio_assert(sym.is_valid(), "invalid token");
    }
};
    
struct market : parameter {
    uint32_t bid_increment_denom;
    int16_t bancor_creation_fee;
    int16_t sale_fee;

    void validate() const override {
        eosio_assert(bid_increment_denom > 0, "bid_increment_denom can't be 0");
        eosio_assert(0 <= bancor_creation_fee && bancor_creation_fee <= 10000,
            "bancor_creation_fee must be between 0% and 100% (0-10000)");
        eosio_assert(0 <= sale_fee && sale_fee <= 10000,
            "sale_fee must be between 0% and 100% (0-10000)");
    }
};

struct checkwin : parameter {
    uint32_t interval;
    uint32_t min_time_from_last_win;
    uint32_t min_time_from_last_bid_start;
    uint32_t min_time_from_last_bid_step;
    uint32_t max_bid_checks;

    void validate() const override {
        eosio_assert(max_bid_checks > 0, "max_bid_checks can't be 0");
    }
};
    
} //param

using token_param = param_wrapper<param::token, 1>;
using market_param = param_wrapper<param::market, 3>;
using checkwin_param = param_wrapper<param::checkwin, 5>;

using registrar_param = std::variant<token_param, market_param, checkwin_param>;

struct [[eosio::table]] registrar_param_state {
    token_param    token;
    market_param   market;
    checkwin_param checkwin;
    static constexpr int params_count = 3;
};

using registrar_param_singleton = eosio::singleton<"regparams"_n, registrar_param_state>;

} //commun
