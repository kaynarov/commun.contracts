#include <commun.emit.hpp>
#include <commun/config.hpp>
#include <commun.point/commun.point.hpp>
#include <commun.point/config.hpp>
#include <commun/util.hpp>
#include <cmath>

namespace commun {
    
void emit::create(symbol commun_symbol, uint16_t annual_emission_rate, uint16_t leaders_reward_prop) {
    require_auth(_self);
    auto commun_code = commun_symbol.code();
    check(commun_symbol == point::get_supply(config::commun_point_name, commun_code).symbol, "symbol precision mismatch");
     
    params params_table(_self, commun_code.raw());
    eosio::check(params_table.find(commun_code.raw()) == params_table.end(), "already exists");
    
    params_table.emplace(_self, [&](auto& p) { p = {
        .id = commun_code.raw(),
        .commun_symbol = commun_symbol,
        .annual_emission_rate = annual_emission_rate,
        .leaders_reward_prop = leaders_reward_prop
    };});
}

int64_t emit::get_continuous_rate(int64_t annual_rate) {
    static auto constexpr real_100percent = static_cast<double>(config::_100percent);
    auto real_rate = static_cast<double>(annual_rate);
    return static_cast<int64_t>(std::log(1.0 + (real_rate / real_100percent)) * real_100percent); 
}
    
void emit::issue_reward(symbol commun_symbol, int64_t passed_seconds, name recipient_contract, bool for_leaders) {
    require_auth(_self);
    auto commun_code = commun_symbol.code();
    
    params params_table(_self, commun_code.raw()); 
    const auto& param = params_table.get(commun_code.raw(), "emitter does not exists, create it before issue");

    auto cont_emission = safe_pct(point::get_supply(config::commun_point_name, commun_code).amount, get_continuous_rate(param.annual_emission_rate));
    
    static constexpr int64_t seconds_per_year = int64_t(365)*24*60*60;
    auto period_emission = safe_prop(cont_emission, passed_seconds, seconds_per_year);
    auto amount = safe_pct(period_emission, for_leaders ? param.leaders_reward_prop : config::_100percent - param.leaders_reward_prop);
    
    if (amount) {
        auto issuer = point::get_issuer(config::commun_point_name, commun_code);
        asset quantity(amount, commun_symbol);
        
        action(
            permission_level{config::commun_point_name, config::issue_permission},
            config::commun_point_name,
            "issue"_n,
            std::make_tuple(issuer, quantity, string())
        ).send();
        
        action(
            permission_level{config::commun_point_name, config::transfer_permission},
            config::commun_point_name,
            "transfer"_n,
            std::make_tuple(issuer, recipient_contract, quantity, string())
        ).send();
    }
}
    
void emit::mscsreward(symbol commun_symbol, int64_t passed_seconds, name recipient_contract) {
    issue_reward(commun_symbol, passed_seconds, recipient_contract, false);
}

void emit::ldrsreward(symbol commun_symbol, int64_t passed_seconds, name recipient_contract) {
    issue_reward(commun_symbol, passed_seconds, recipient_contract, true);
}
}

EOSIO_DISPATCH(commun::emit, (create)(mscsreward)(ldrsreward))
