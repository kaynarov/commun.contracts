#include <commun.emit.hpp>
#include <commun.point/commun.point.hpp>
#include <commun.point/config.hpp>
#include <commun/util.hpp>
#include <cmath>

namespace commun {
    
void emit::create(symbol_code commun_code, uint16_t annual_emission_rate, uint16_t leaders_reward_prop) {
    require_auth(_self);
    check(point::exist(config::point_name, commun_code), "point with symbol does not exist");
    check(0 <  annual_emission_rate && annual_emission_rate <= 10000, "annual_emission_rate must be between 0.01% and 100% (1-10000)");
    check(0 <= leaders_reward_prop && leaders_reward_prop <= 10000, "leaders_reward_prop must be between 0% and 100% (0-10000)");
     
    params params_table(_self, commun_code.raw());
    eosio::check(params_table.find(commun_code.raw()) == params_table.end(), "already exists");
    
    params_table.emplace(_self, [&](auto& p) { p = {
        .id = commun_code.raw(),
        .annual_emission_rate = annual_emission_rate,
        .leaders_reward_prop = leaders_reward_prop
    };});
    
    stats stats_table(_self, commun_code.raw());
    eosio::check(stats_table.find(commun_code.raw()) == stats_table.end(), "SYSTEM: stat already exists");
    
    auto now = eosio::current_time_point();
    stats_table.emplace(_self, [&](auto& s) { s = {
        .id = commun_code.raw(),
        .latest_mosaics_reward = now,
        .latest_leaders_reward = now
    };});
}

int64_t emit::get_continuous_rate(int64_t annual_rate) {
    static auto constexpr real_100percent = static_cast<double>(config::_100percent);
    auto real_rate = static_cast<double>(annual_rate);
    return static_cast<int64_t>(std::log(1.0 + (real_rate / real_100percent)) * real_100percent); 
}
    
void emit::issuereward(symbol_code commun_code, bool for_leaders) {
    require_auth(_self);
    
    params params_table(_self, commun_code.raw()); 
    const auto& param = params_table.get(commun_code.raw(), "emitter does not exists, create it before issue");
    
    stats stats_table(_self, commun_code.raw());
    const auto& stat = stats_table.get(commun_code.raw(), "SYSTEM: stat does not exists");
    
    auto now = eosio::current_time_point();
    int64_t passed_seconds = (now - stat.latest_reward(for_leaders)).to_seconds();
    eosio::check(passed_seconds >= 0, "SYSTEM: incorrect passed_seconds");
    eosio::check(passed_seconds >= config::reward_period(for_leaders), "SYSTEM: untimely claim reward");

    auto to_contract = for_leaders ? config::control_name : config::gallery_name;
    eosio::check(is_account(to_contract), to_contract.to_string() + " contract does not exists");

    auto supply = point::get_supply(config::point_name, commun_code);
    auto cont_emission = safe_pct(supply.amount, get_continuous_rate(param.annual_emission_rate));
    
    static constexpr int64_t seconds_per_year = int64_t(365)*24*60*60;
    auto period_emission = safe_prop(cont_emission, passed_seconds, seconds_per_year);
    auto amount = safe_pct(period_emission, for_leaders ? param.leaders_reward_prop : config::_100percent - param.leaders_reward_prop);
    
    if (amount) {
        auto issuer = point::get_issuer(config::point_name, commun_code);
        asset quantity(amount, supply.symbol);
        
        action(
            permission_level{config::point_name, config::issue_permission},
            config::point_name,
            "issue"_n,
            std::make_tuple(issuer, quantity, string())
        ).send();
        
        action(
            permission_level{config::point_name, config::transfer_permission},
            config::point_name,
            "transfer"_n,
            std::make_tuple(issuer, to_contract, quantity, string())
        ).send();
    }
    
    stats_table.modify(stat, name(), [&]( auto& s) { (for_leaders ? s.latest_leaders_reward : s.latest_mosaics_reward) = now; });
}
}

EOSIO_DISPATCH(commun::emit, (create)(issuereward))
