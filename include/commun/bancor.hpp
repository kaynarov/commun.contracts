#pragma once

namespace commun {
    
int64_t calc_bancor_amount(int64_t current_reserve, int64_t current_supply, double cw, int64_t reserve_amount, bool strict = true) {
    if (!strict && !current_reserve) {
        return reserve_amount; //max(100%, reserve_amount)?
    }
    eosio::check(current_reserve > 0, "no reserve");
    double buy_prop = static_cast<double>(reserve_amount) / static_cast<double>(current_reserve);
    double new_supply = static_cast<double>(current_supply) * std::pow(1.0 + buy_prop, cw);
    eosio::check(new_supply <= static_cast<double>(std::numeric_limits<int64_t>::max()), "invalid supply, int64_t overflow");
    return static_cast<int64_t>(new_supply) - current_supply;
}

} // commun
