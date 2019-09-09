/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "commun.point/commun.point.hpp"
#include <commun.gallery/commun.gallery.hpp>
#include <commun/dispatchers.hpp>
#include <cyber.token/cyber.token.hpp>
#include <eosio/system.hpp>
#include <commun/util.hpp>

namespace commun {

void point::create(name issuer, asset maximum_supply, int16_t cw, int16_t fee) {
    require_auth(_self);

    auto commun_symbol = maximum_supply.symbol;
    check(commun_symbol.is_valid(), "invalid symbol name");
    check(maximum_supply.is_valid(), "invalid supply");
    check(maximum_supply.amount > 0, "max-supply must be positive");
    check(0 <  cw  && cw  <= 10000, "connector weight must be between 0.01% and 100% (1-10000)");
    check(0 <= fee && fee <= 10000, "fee must be between 0% and 100% (0-10000)");
    symbol_code commun_code = commun_symbol.code();
    
    params params_table(_self, commun_code.raw());
    eosio::check(params_table.find(commun_code.raw()) == params_table.end(), "already exists");
    
    params_table.emplace(_self, [&](auto& p) { p = {
        .max_supply = maximum_supply,
        .cw = cw,
        .fee = fee,
        .issuer = issuer
    };});
        
    stats stats_table(_self, commun_code.raw());
    eosio::check(stats_table.find(commun_code.raw()) == stats_table.end(), "SYSTEM: already exists");
    stats_table.emplace(_self, [&](auto& s) { s = {
        .supply = asset(0, commun_symbol),
        .reserve = asset(0, config::reserve_token)
    };});
    
    accounts accounts_table(_self, issuer.value);
    accounts_table.emplace(_self, [&](auto& a) { a = {
        .balance = asset(0, commun_symbol)
    };});
}

void point::setfreezer(name freezer) {
    require_auth(_self);
    auto singparam = singparams(_self, _self.value);
    singparam.set(structures::singleton_param { .point_freezer = freezer }, _self);
}

void point::issue(name to, asset quantity, string memo) {
    auto commun_symbol = quantity.symbol;
    check(commun_symbol.is_valid(), "invalid symbol name");
    check(memo.size() <= 256, "memo has more than 256 bytes");
    symbol_code commun_code = commun_symbol.code();
    
    params params_table(_self, commun_code.raw());
    const auto& param = params_table.get(commun_code.raw(), "point with symbol does not exist, create it before issue");

    stats stats_table(_self, commun_code.raw());
    const auto& stat = stats_table.get(commun_code.raw(), "SYSTEM: point with symbol does not exist");

    check(has_auth(param.issuer) || has_auth(_self), "missing required signature");

    check(stat.reserve.amount > 0, "no reserve");
    check(quantity.is_valid(), "invalid quantity");
    check(quantity.amount > 0, "must issue positive quantity");

    check(quantity.symbol == stat.supply.symbol, "symbol precision mismatch");
    check(quantity.amount <= param.max_supply.amount - stat.supply.amount, "quantity exceeds available supply");

    stats_table.modify(stat, same_payer, [&]( auto& s) {
       s.supply += quantity;
    });

    add_balance(param.issuer, quantity, param.issuer);

    if (to != param.issuer) {
      SEND_INLINE_ACTION(*this, transfer, { {param.issuer, "active"_n} },
                          { param.issuer, to, quantity, memo }
      );
    }
}

void point::retire(asset quantity, string memo) {
    auto commun_symbol = quantity.symbol;
    check( commun_symbol.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );
    symbol_code commun_code = commun_symbol.code();
    
    params params_table(_self, commun_code.raw());
    const auto& param = params_table.get(commun_code.raw(), "point with symbol does not exist");

    stats stats_table(_self, commun_code.raw());
    const auto& stat = stats_table.get(commun_code.raw(), "SYSTEM: point with symbol does not exist");

    check(has_auth(param.issuer) || has_auth(_self), "missing required signature");
    check(quantity.is_valid(), "invalid quantity");
    check(quantity.amount > 0, "must retire positive quantity");

    check(quantity.symbol == stat.supply.symbol, "symbol precision mismatch");

    stats_table.modify(stat, same_payer, [&](auto& s) {
       s.supply -= quantity;
    });

    sub_balance(param.issuer, quantity);
}

void point::transfer(name from, name to, asset quantity, string memo) {
     do_transfer(from, to, quantity, memo);
}

void point::on_reserve_transfer(name from, name to, asset quantity, std::string memo) {
    if(_self != to)
        return;
    const size_t pref_size = config::restock_prefix.size();
    const size_t memo_size = memo.size();
    bool restock = (memo_size >= pref_size) && memo.substr(0, pref_size) == config::restock_prefix;
    auto commun_code = symbol_code(restock ? memo.substr(pref_size).c_str() : memo.c_str());
    
    params params_table(_self, commun_code.raw());
    const auto& param = params_table.get(commun_code.raw(), "point with symbol does not exist");
    
    stats stats_table(_self, commun_code.raw());
    auto& stat = stats_table.get(commun_code.raw(), "SYSTEM: point with symbol does not exist");
    check(quantity.symbol == stat.reserve.symbol, "invalid reserve token symbol");
    
    asset add_tokens(0, stat.supply.symbol);
    if (!restock) {
        add_tokens = calc_token_quantity(param, stat, quantity);
        check(add_tokens.amount <= param.max_supply.amount - stat.supply.amount, "quantity exceeds available supply");
        add_balance(from, add_tokens, from);
    }

    stats_table.modify(stat, same_payer, [&](auto& s) {
        s.reserve += quantity;
        s.supply += add_tokens;
    });
}

void point::sub_balance(name owner, asset value) {
   accounts accounts_table(_self, owner.value);

   const auto& from = accounts_table.get( value.symbol.code().raw(), "no balance object found");

   auto avail_balance = from.balance.amount;
   auto singparam = singparams(_self, _self.value);
   auto point_freezer = singparam.exists() ? singparam.get().point_freezer : name();
   if (point_freezer) {
       avail_balance -= gallery::get_frozen_amount(point_freezer, owner, value.symbol.code());
   }
   check(avail_balance >= value.amount, "overdrawn balance");

   accounts_table.modify(from, owner, [&](auto& a) {
         a.balance -= value;
      });
}

void point::add_balance(name owner, asset value, name ram_payer) {
    accounts accounts_table(_self, owner.value);
    auto to = accounts_table.find(value.symbol.code().raw());
    if (to == accounts_table.end()) {
        accounts_table.emplace( ram_payer, [&]( auto& a ) {
            a.balance = value;
        });
    } 
    else {
        accounts_table.modify(to, same_payer, [&](auto& a) {
            a.balance += value;
        });
    }
}

void point::open(name owner, const symbol& symbol, name ram_payer) {
   require_auth(ram_payer);
   eosio::check( is_account( owner ), "owner account does not exist");
   auto commun_code = symbol.code();

   stats stats_table(_self, commun_code.raw());
   const auto& st = stats_table.get(commun_code.raw(), "symbol does not exist");
   check(st.supply.symbol == symbol, "symbol precision mismatch");
   
   accounts accounts_table(_self, owner.value);
   auto it = accounts_table.find(commun_code.raw());
   if (it == accounts_table.end()) {
      accounts_table.emplace(ram_payer, [&](auto& a){
         a.balance = asset{0, symbol};
      });
   }
}

void point::close(name owner, const symbol& symbol) {
   require_auth(owner);
   
   params params_table(_self, symbol.code().raw());
   const auto& param = params_table.get(symbol.code().raw(), "point with symbol does not exist");
   check(owner != param.issuer, "issuer can't close");
   
   accounts accounts_table( _self, owner.value );
   auto it = accounts_table.find( symbol.code().raw() );
   check( it != accounts_table.end(), "Balance row already deleted or never existed. Action won't have any effect." );
   check( it->balance.amount == 0, "Cannot close because the balance is not zero." );
   accounts_table.erase( it );
}

void point::do_transfer(name from, name to, const asset &quantity, const string &memo)
{
    check( from != to, "cannot transfer to self" );
    check(has_auth(from) || has_auth(_self), "missing required signature");
    check( is_account( to ), "to account does not exist");
    auto commun_code = quantity.symbol.code();
    
    params params_table(_self, commun_code.raw());
    const auto& param = params_table.get(commun_code.raw(), "point with symbol does not exist");
    stats stats_table(_self, commun_code.raw());
    const auto& stat = stats_table.get(commun_code.raw(), "SYSTEM: point with symbol does not exist");

    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must transfer positive quantity" );
    check( quantity.symbol == stat.supply.symbol, "symbol precision mismatch" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    sub_balance(from, quantity);
    auto payer = has_auth(to) ? to : from;

    if (to != param.issuer) {
        require_recipient( from );
        require_recipient( to );

        add_balance( to, quantity, payer );
    }
    else {
        auto sub_reserve = calc_reserve_quantity(param, stat, quantity);
        stats_table.modify(stat, same_payer, [&](auto& s) {
            s.reserve -= sub_reserve;
            s.supply -= quantity;
        });

        INLINE_ACTION_SENDER(eosio::token, transfer)(config::token_name, {_self, config::active_name},
        {_self, from, sub_reserve, quantity.symbol.code().to_string() + " sold"});
    }
}

} /// namespace commun

DISPATCH_WITH_TRANSFER(commun::point, commun::config::token_name, on_reserve_transfer,
    (create)(setfreezer)(issue)(transfer)(open)(close)(retire)
    )
