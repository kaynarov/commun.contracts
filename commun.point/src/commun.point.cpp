/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "commun.point/commun.point.hpp"
#include <commun.gallery/commun.gallery.hpp>
#include <commun/dispatchers.hpp>
#include <cyber.token/cyber.token.hpp>
#include <eosio/event.hpp>
#include <eosio/system.hpp>

namespace commun {

void point::send_currency_event(const structures::stat& st, const structures::param& par) {
    currency_event data{st.supply, st.reserve, par.max_supply, par.cw, par.fee, par.issuer, par.transfer_fee, par.min_transfer_fee_points};
    eosio::event(_self, "currency"_n, data).send();
}

void point::send_balance_event(name acc, const structures::account& accinfo) {
    balance_event data{acc, accinfo.balance};
    eosio::event(_self, "balance"_n, data).send();
}

void point::send_exchange_event(const asset& amount) {
    exchange_event data{amount};
    eosio::event(_self, "exchange"_n, data).send();
}

static eosio::asset vague_asset(int64_t amount) { //bypass non-zero symbol restriction
    eosio::asset ret;
    ret.amount = amount;
    return ret;
}

void point::create(name issuer, asset initial_supply, asset maximum_supply, int16_t cw, int16_t fee) {
    require_auth(_self);
    check(is_account(issuer), "issuer account does not exist");

    check(maximum_supply.is_valid(), "invalid maximum_supply");
    check(maximum_supply.amount > 0, "maximum_supply must be positive");

    check(initial_supply.symbol == maximum_supply.symbol, "initial_supply and maximum_supply must have same symbol");
    check(initial_supply.is_valid(), "invalid initial_supply");
    check(initial_supply.amount >= 0, "initial_supply must be positive or zero");
    check(initial_supply <= maximum_supply, "initial_supply must be less or equal maximum_supply");

    check(0 <  cw  && cw  <= 10000, "connector weight must be between 0.01% and 100% (1-10000)");
    check(0 <= fee && fee <= 10000, "fee must be between 0% and 100% (0-10000)");
    symbol_code commun_code = maximum_supply.symbol.code();

    params params_table(_self, _self.value);
    eosio::check(params_table.find(commun_code.raw()) == params_table.end(), "point already exists");

    auto issuer_idx = params_table.get_index<"byissuer"_n>();
    eosio::check(issuer_idx.find(issuer) == issuer_idx.end(), "issuer already has a point");
    
    auto param = params_table.emplace(_self, [&](auto& p) { p = {
        .max_supply = maximum_supply,
        .cw = cw,
        .fee = fee,
        .issuer = issuer
    };});

    stats stats_table(_self, commun_code.raw());
    eosio::check(stats_table.find(commun_code.raw()) == stats_table.end(), "SYSTEM: already exists");
    auto stat = stats_table.emplace(_self, [&](auto& s) { s = {
        .supply = initial_supply,
        .reserve = asset(0, config::reserve_token)
    };});

    send_currency_event(*stat, *param);

    accounts accounts_table(_self, issuer.value);
    accounts_table.emplace(_self, [&](auto& a) { a = {
        .balance = initial_supply
    };});
}

void point::setparams(symbol_code commun_code, uint16_t transfer_fee, int64_t min_transfer_fee_points) {
    params params_table(_self, _self.value);
    const auto& param = params_table.get(commun_code.raw(), "symbol does not exist");
    require_auth(param.issuer);

    check(transfer_fee <= config::_100percent, "transfer_fee can't be greater than 100%");

    check(min_transfer_fee_points >= 0, "min_transfer_fee_points cannot be negative");
    if (transfer_fee) {
        check(min_transfer_fee_points > 0, "min_transfer_fee_points cannot be 0 if transfer_fee set");
    }

    params_table.modify(param, eosio::same_payer, [&](auto& p) {
        p.transfer_fee = transfer_fee;
        p.min_transfer_fee_points = min_transfer_fee_points;
    });
}

void point::setfreezer(name freezer) {
    require_auth(_self);
    eosio::check(is_account(freezer), "freezer account does not exist");
    auto global_param = global_params(_self, _self.value);
    global_param.set({ .point_freezer = freezer }, _self);
}

void point::issue(name to, asset quantity, string memo) {
    auto commun_symbol = quantity.symbol;
    check(commun_symbol.is_valid(), "invalid symbol name");
    check(memo.size() <= 256, "memo has more than 256 bytes");
    symbol_code commun_code = commun_symbol.code();

    params params_table(_self, _self.value);
    const auto& param = params_table.get(commun_code.raw(), "point with symbol does not exist, create it before issue");

    stats stats_table(_self, commun_code.raw());
    const auto& stat = stats_table.get(commun_code.raw(), "SYSTEM: point with symbol does not exist");

    require_auth(_self);

    check(stat.reserve.amount > 0, "no reserve");
    check(quantity.is_valid(), "invalid quantity");
    check(quantity.amount > 0, "must issue positive quantity");

    check(quantity.symbol == stat.supply.symbol, "symbol precision mismatch");
    check(quantity.amount <= param.max_supply.amount - stat.supply.amount, "quantity exceeds available supply");

    stats_table.modify(stat, same_payer, [&](auto& s) {
        s.supply += quantity;
        send_currency_event(s, param);
    });

    add_balance(param.issuer, quantity, param.issuer);

    if (to != param.issuer) {
        SEND_INLINE_ACTION(*this, transfer, { {param.issuer, "active"_n} },
            { param.issuer, to, quantity, memo }
        );
    }
}

void point::retire(name from, asset quantity, string memo) {
    auto commun_symbol = quantity.symbol;
    check(commun_symbol.is_valid(), "invalid symbol name");
    check(memo.size() <= 256, "memo has more than 256 bytes");
    symbol_code commun_code = commun_symbol.code();

    params params_table(_self, _self.value);
    const auto& param = params_table.get(commun_code.raw(), "point with symbol does not exist");

    stats stats_table(_self, commun_code.raw());
    const auto& stat = stats_table.get(commun_code.raw(), "SYSTEM: point with symbol does not exist");

    require_auth(from);
    check(quantity.is_valid(), "invalid quantity");
    check(quantity.amount > 0, "must retire positive quantity");

    check(quantity.symbol == stat.supply.symbol, "symbol precision mismatch");

    stats_table.modify(stat, same_payer, [&](auto& s) {
        s.supply -= quantity;
        send_currency_event(s, param);
    });

    sub_balance(from, quantity);
}

void point::transfer(name from, name to, asset quantity, string memo) {
    do_transfer(from, to, quantity, memo);
}

void point::withdraw(name owner, asset quantity) {
    require_auth(owner);
    check(quantity.symbol == config::reserve_token, "invalid reserve token symbol");
    sub_balance(owner, vague_asset(quantity.amount));
    INLINE_ACTION_SENDER(eosio::token, transfer)(config::token_name, {_self, config::active_name}, {_self, owner, quantity, ""});
}

void point::on_reserve_transfer(name from, name to, asset quantity, std::string memo) {
    if(_self != to)
        return;
    check(quantity.symbol == config::reserve_token, "invalid reserve token symbol");
    const size_t memo_size = memo.size();
    if (memo_size) {
        const size_t pref_size = config::restock_prefix.size();
        bool restock = (memo_size >= pref_size) && memo.substr(0, pref_size) == config::restock_prefix;
        auto commun_code = symbol_code(restock ? memo.substr(pref_size).c_str() : memo.c_str());

        params params_table(_self, _self.value);
        const auto& param = params_table.get(commun_code.raw(), "point with symbol does not exist");

        stats stats_table(_self, commun_code.raw());
        auto& stat = stats_table.get(commun_code.raw(), "SYSTEM: point with symbol does not exist");

        asset add_tokens(0, stat.supply.symbol);
        if (!restock) {
            add_tokens = calc_token_quantity(param, stat, quantity);
            check(add_tokens.amount > 0, "these tokens cost zero points");
            check(add_tokens.amount <= param.max_supply.amount - stat.supply.amount, "quantity exceeds available supply");
            check(balance_exists(from, commun_code), "balance of from not opened");
            add_balance(from, add_tokens, from);
            send_exchange_event(add_tokens);
        }

        stats_table.modify(stat, same_payer, [&](auto& s) {
            s.reserve += quantity;
            s.supply += add_tokens;
            send_currency_event(s, param);
        });
        
        notify_balance_change(param.issuer, vague_asset(quantity.amount));
    }
    else {
        check(balance_exists(from, symbol_code()), "balance of from not opened");
        add_balance(from, vague_asset(quantity.amount), from);
    }
}

void point::notify_balance_change(name owner, asset diff) {
    if (is_account(config::control_name)) {
        action(
            permission_level{config::control_name, "changepoints"_n},
            config::control_name, "changepoints"_n,
            std::make_tuple(owner, diff)
        ).send();
    }
}

void point::sub_balance(name owner, asset value) {
    accounts accounts_table(_self, owner.value);

    const auto& from = accounts_table.get(value.symbol.code().raw(), "no balance object found");

    auto avail_balance = from.balance.amount;
    auto global_param = global_params(_self, _self.value);
    auto point_freezer = global_param.get_or_default().point_freezer;
    if (point_freezer) {
        avail_balance -= gallery::get_frozen_amount(point_freezer, owner, value.symbol.code());
    }
    check(avail_balance >= value.amount, "overdrawn balance");

    accounts_table.modify(from, owner, [&](auto& a) {
        a.balance -= value;
        send_balance_event(owner, a);
    });

    notify_balance_change(owner, -value);
}

void point::add_balance(name owner, asset value, name ram_payer) {
    accounts accounts_table(_self, owner.value);
    auto to = accounts_table.find(value.symbol.code().raw());
    if (to == accounts_table.end()) {
        accounts_table.emplace(ram_payer, [&](auto& a) {
            a.balance = value;
            send_balance_event(owner, a);
        });
    } else {
        accounts_table.modify(to, same_payer, [&](auto& a) {
            a.balance += value;
            send_balance_event(owner, a);
        });
    }
    notify_balance_change(owner, value);
}

void point::open(name owner, symbol_code commun_code, std::optional<name> ram_payer) {

    auto actual_ram_payer = ram_payer.value_or(owner);
    
    require_auth(actual_ram_payer);
    eosio::check(is_account(owner), "owner account does not exist");
    symbol sym;
    
    if (commun_code) {
        stats stats_table(_self, commun_code.raw());
        const auto& st = stats_table.get(commun_code.raw(), "symbol does not exist");
        sym = st.supply.symbol;
    }

    accounts accounts_table(_self, owner.value);
    auto it = accounts_table.find(commun_code.raw());
    if (it == accounts_table.end()) {
        accounts_table.emplace(actual_ram_payer, [&](auto& a){
            a.balance = commun_code ? asset{0, sym} : vague_asset(0);
        });
    }
}

void point::close(name owner, symbol_code commun_code) {
    require_auth(owner);

    params params_table(_self, _self.value);

    if (commun_code) {
        const auto& param = params_table.get(commun_code.raw(), "point with symbol does not exist");
        check(owner != param.issuer, "issuer can't close");
    }

    accounts accounts_table(_self, owner.value);
    auto it = accounts_table.find(commun_code.raw());
    check(it != accounts_table.end(), "Balance row already deleted or never existed. Action won't have any effect.");
    check(it->balance.amount == 0, "Cannot close because the balance is not zero.");
    accounts_table.erase(it);
}

void point::do_transfer(name from, name to, const asset &quantity, const string &memo) {
    require_auth(from);
    check(from != to, "cannot transfer to self");
    check(is_account(to), "to account does not exist");
    auto commun_code = quantity.symbol.code();

    params params_table(_self, _self.value);
    const auto& param = params_table.get(commun_code.raw(), "point with symbol does not exist");
    stats stats_table(_self, commun_code.raw());
    const auto& stat = stats_table.get(commun_code.raw(), "SYSTEM: point with symbol does not exist");

    check(quantity.is_valid(), "invalid quantity");
    check(quantity.amount > 0, "must transfer positive quantity");
    check(quantity.symbol == stat.supply.symbol, "symbol precision mismatch");
    check(memo.size() <= 256, "memo has more than 256 bytes");

    auto payer = has_auth(to) ? to : from;

    if (to != _self) {
        require_recipient(from);
        require_recipient(to);

        auto sub_quantity = quantity;
        if (param.transfer_fee && !no_fee_transfer(param.issuer, from, to)) {
            auto fee_points = asset(
                std::max(safe_pct(quantity.amount, param.transfer_fee), param.min_transfer_fee_points),
                quantity.symbol);
            sub_quantity += fee_points;

            stats_table.modify(stat, same_payer, [&](auto& s) {
                s.supply -= fee_points;
                send_currency_event(s, param);
            });
        }
        sub_balance(from, sub_quantity);

        add_balance(to, quantity, payer);
    }
    else {
        sub_balance(from, quantity);

        auto sub_reserve = calc_reserve_quantity(param, stat, quantity);
        check(sub_reserve.amount > 0, "these points cost zero tokens");
        stats_table.modify(stat, same_payer, [&](auto& s) {
            s.reserve -= sub_reserve;
            s.supply -= quantity;
            send_currency_event(s, param);
        });

        INLINE_ACTION_SENDER(eosio::token, transfer)(config::token_name, {_self, config::active_name},
            {_self, from, sub_reserve, quantity.symbol.code().to_string() + " sold"});
        notify_balance_change(param.issuer, vague_asset(-quantity.amount));
        send_exchange_event(sub_reserve);
    }
}

} /// namespace commun

DISPATCH_WITH_TRANSFER(commun::point, commun::config::token_name, on_reserve_transfer,
    (create)(setparams)(setfreezer)(issue)(transfer)(withdraw)(open)(close)(retire)
)
