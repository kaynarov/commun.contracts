#include <commun.list/commun.list.hpp>
#include <commun.emit.hpp>
#include <commun.point/commun.point.hpp>
#include <commun.point/config.hpp>
#include <commun/util.hpp>
#include <cmath>

namespace commun {

void emit::init(symbol_code commun_code) {
    require_auth(_self);

    stats stats_table(_self, commun_code.raw());
    eosio::check(stats_table.find(commun_code.raw()) == stats_table.end(), "already exists");

    auto& community = commun_list::get_community(commun_code);

    auto now = eosio::current_time_point();
    stats_table.emplace(_self, [&](auto& s) {
        s.id = commun_code.raw();
        s.reward_receivers.reserve(community.emission_receivers.size());
        for (auto& receiver: community.emission_receivers) {
            s.reward_receivers.push_back({receiver.contract, now});
        }
    });
}

int64_t emit::get_continuous_rate(int64_t annual_rate) {
    static auto constexpr real_100percent = static_cast<double>(config::_100percent);
    auto real_rate = static_cast<double>(annual_rate);
    return static_cast<int64_t>(std::log(1.0 + (real_rate / real_100percent)) * real_100percent); 
}

void emit::issuereward(symbol_code commun_code, name to_contract) {
    require_auth(_self);

    stats stats_table(_self, commun_code.raw());
    const auto& stat = stats_table.get(commun_code.raw(), "emitter does not exists, create it before issue");
    auto passed_seconds = stat.last_reward_passed_seconds(to_contract);

    auto& community = commun_list::get_community(commun_code);
    if (!is_it_time_to_reward(community, to_contract, passed_seconds)) {
        return; // action can be called twice before updating the date
    }

    eosio::check(is_account(to_contract), to_contract.to_string() + " contract does not exists");

    auto supply = point::get_supply(commun_code);
    auto cont_emission = safe_pct(supply.amount, get_continuous_rate(community.emission_rate));

    static constexpr int64_t seconds_per_year = int64_t(365)*24*60*60;
    auto period_emission = safe_prop(cont_emission, passed_seconds, seconds_per_year);
    auto amount = safe_pct(period_emission, community.get_emission_receiver(to_contract).percent);

    if (amount) {
        auto issuer = point::get_issuer(commun_code);
        asset quantity(amount, supply.symbol);

        action(
            permission_level{config::point_name, config::issue_permission},
            config::point_name,
            "issue"_n,
            std::make_tuple(issuer, quantity, string())
        ).send();

        action(
            permission_level{issuer, config::transfer_permission},
            config::point_name,
            "transfer"_n,
            std::make_tuple(issuer, to_contract, quantity, string())
        ).send();
    }

    stats_table.modify(stat, name(), [&](auto& s) {
        s.get_reward_receiver(to_contract).time = eosio::current_time_point();
    });
}

} // commun
