#include <commun.emit.hpp>
#include <commun.list/commun.list.hpp>
#include <commun.point/commun.point.hpp>
#include <commun.point/config.hpp>
#include <commun/util.hpp>
#include <cmath>

namespace commun {

void emit::create(symbol_code commun_code) {
    require_auth(_self);
    check(point::exist(config::point_name, commun_code), "point with symbol does not exist");

    stats stats_table(_self, commun_code.raw());
    eosio::check(stats_table.find(commun_code.raw()) == stats_table.end(), "already exists");

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

    stats stats_table(_self, commun_code.raw());
    const auto& stat = stats_table.get(commun_code.raw(), "emitter does not exists, create it before issue");
    
    auto community = commun_list::get_community(config::list_name, commun_code);

    auto now = eosio::current_time_point();
    int64_t passed_seconds = (now - stat.latest_reward(for_leaders)).to_seconds();
    eosio::check(passed_seconds >= 0, "SYSTEM: incorrect passed_seconds");
    eosio::check(passed_seconds >= config::reward_period(for_leaders), "SYSTEM: untimely claim reward");

    auto to_contract = for_leaders ? config::control_name : config::gallery_name;
    eosio::check(is_account(to_contract), to_contract.to_string() + " contract does not exists");

    auto supply = point::get_supply(config::point_name, commun_code);
    auto cont_emission = safe_pct(supply.amount, get_continuous_rate(community.emission_rate));

    static constexpr int64_t seconds_per_year = int64_t(365)*24*60*60;
    auto period_emission = safe_prop(cont_emission, passed_seconds, seconds_per_year);
    auto amount = safe_pct(period_emission, for_leaders ? community.leaders_percent : config::_100percent - community.leaders_percent);

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

    stats_table.modify(stat, name(), [&](auto& s) { (for_leaders ? s.latest_leaders_reward : s.latest_mosaics_reward) = now; });
}

} // commun

EOSIO_DISPATCH(commun::emit, (create)(issuereward))
