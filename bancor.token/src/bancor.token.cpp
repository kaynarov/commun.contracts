/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "bancor.token/bancor.token.hpp"
#include <commun/dispatchers.hpp>
#include <cyber.token/cyber.token.hpp>

namespace commun {

void bancor::create(name issuer, asset maximum_supply, int16_t cw, int16_t fee) {
    require_auth(_self);

    auto sym = maximum_supply.symbol;
    check(sym.is_valid(), "invalid symbol name");
    check(maximum_supply.is_valid(), "invalid supply");
    check(maximum_supply.amount > 0, "max-supply must be positive");
    check(0 <  cw  && cw  <= 10000, "connector weight must be between 0.01% and 100% (1-10000)");
    check(0 <= fee && fee <= 10000, "fee must be between 0% and 100% (0-10000)");
    stats statstable(_self, sym.code().raw());
    auto existing = statstable.find(sym.code().raw());
    check( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace(_self, [&]( auto& s) {
       s.supply = asset(0, maximum_supply.symbol);
       s.max_supply = maximum_supply;
       s.reserve = asset(0, config::reserve_token); // should we force it to be reserve_token?
       s.cw = cw;
       s.fee = fee;
       s.issuer = issuer;
    });
}

void bancor::issue(name to, asset quantity, string memo) {
    auto sym = quantity.symbol;
    check(sym.is_valid(), "invalid symbol name");
    check(memo.size() <= 256, "memo has more than 256 bytes");

    stats statstable(_self, sym.code().raw());
    const auto& st = statstable.get(sym.code().raw(), "token with symbol does not exist, create token before issue");

    require_auth(st.issuer);
    check(st.reserve.amount > 0, "token has no reserve");
    check(quantity.is_valid(), "invalid quantity");
    check(quantity.amount > 0, "must issue positive quantity");

    check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
    check(quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify(st, same_payer, [&]( auto& s) {
       s.supply += quantity;
    });

    add_balance(st.issuer, quantity, st.issuer);

    if (to != st.issuer) {
      SEND_INLINE_ACTION(*this, transfer, { {st.issuer, "active"_n} },
                          { st.issuer, to, quantity, memo }
      );
    }
}

void bancor::retire(asset quantity, string memo) {
    auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable(_self, sym.code().raw());
    const auto& st = statstable.get(sym.code().raw(), "token with symbol does not exist");

    require_auth(st.issuer);
    check(quantity.is_valid(), "invalid quantity");
    check(quantity.amount > 0, "must retire positive quantity");

    check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");

    statstable.modify(st, same_payer, [&](auto& s) {
       s.supply -= quantity;
    });

    sub_balance(st.issuer, quantity);
}

void bancor::transfer(name from, name to, asset quantity, string memo) {
     do_transfer(from, to, quantity, memo);
}

void bancor::payment(name from, name to, asset quantity, std::string memo) {
     do_transfer(from, to, quantity, memo, true);
}

void bancor::on_reserve_transfer(name from, name to, asset quantity, std::string memo) {
    if(_self != to)
        return;
    const size_t pref_size = config::restock_prefix.size();
    const size_t memo_size = memo.size();
    bool restock = (memo_size >= pref_size) && memo.substr(0, pref_size) == config::restock_prefix;
    auto sym_code = symbol_code(restock ? memo.substr(pref_size).c_str() : memo.c_str());
    
    stats statstable(_self, sym_code.raw());
    auto& st = statstable.get(sym_code.raw(), "token with symbol does not exist");
    check(quantity.symbol == st.reserve.symbol, "invalid reserve token symbol");
    
    asset add_tokens(0, st.supply.symbol);
    if (!restock) {
        add_tokens = calc_token_quantity(st, quantity);
        check(add_tokens.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");
        add_balance(from, add_tokens, from);
    }

    statstable.modify(st, same_payer, [&](auto& s) {
        s.reserve += quantity;
        s.supply += add_tokens;
    });
}

void bancor::sub_balance(name owner, asset value) {
   accounts from_acnts(_self, owner.value);

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found");
   check(from.balance.amount >= value.amount, "overdrawn balance");

   from_acnts.modify(from, owner, [&](auto& a) {
         a.balance -= value;
      });
}

void bancor::add_balance(name owner, asset value, name ram_payer) {
   accounts to_acnts(_self, owner.value);
   auto to = to_acnts.find(value.symbol.code().raw());
   if (to == to_acnts.end()) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
        a.payments.symbol = value.symbol;
      });
   } else {
      to_acnts.modify(to, same_payer, [&](auto& a) {
        a.balance += value;
      });
   }
}

void bancor::open(name owner, const symbol& symbol, name ram_payer) {
   require_auth(ram_payer);

   auto sym_code_raw = symbol.code().raw();

   stats statstable(_self, sym_code_raw);
   const auto& st = statstable.get(sym_code_raw, "symbol does not exist");
   check(st.supply.symbol == symbol, "symbol precision mismatch");

   accounts acnts(_self, owner.value);
   auto it = acnts.find(sym_code_raw);
   if (it == acnts.end()) {
      acnts.emplace(ram_payer, [&](auto& a){
        a.balance = asset{0, symbol};
        a.payments = asset{0, symbol};
      });
   }
}

void bancor::close(name owner, const symbol& symbol) {
   require_auth(owner);
   accounts acnts( _self, owner.value );
   auto it = acnts.find( symbol.code().raw() );
   check( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
   check( it->balance.amount == 0, "Cannot close because the balance is not zero." );
   acnts.erase( it );
}

void bancor::add_payment( name owner, asset value, name ram_payer ) {
   accounts to_acnts( _self, owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance.symbol = value.symbol;
        a.payments = value;
      });
   } else {
      to_acnts.modify( to, same_payer, [&]( auto& a ) {
        a.payments += value;
      });
   }
}

void bancor::claim( name owner, asset quantity )
{
   require_auth( owner );

   check( quantity.is_valid(), "invalid quantity" );
   check( quantity.amount > 0, "must transfer positive quantity" );

   accounts owner_acnts( _self, owner.value );
   auto account = owner_acnts.find( quantity.symbol.code().raw() );
   check( account != owner_acnts.end(), "not found object account" );
   check( quantity.symbol == account->payments.symbol, "symbol precision mismatch" );
   check( account->payments >= quantity, "insufficient funds" );
   owner_acnts.modify( account, owner, [&]( auto& a ) {
       a.balance += quantity;
       a.payments -= quantity;
   });
}

void bancor::do_transfer(name from, name to, const asset &quantity, const string &memo, bool payment)
{
        check( from != to, "cannot transfer to self" );
        require_auth( from );
        check( is_account( to ), "to account does not exist");
        auto sym = quantity.symbol.code();
        stats statstable( _self, sym.raw() );
        const auto& st = statstable.get( sym.raw() );

        check( quantity.is_valid(), "invalid quantity" );
        check( quantity.amount > 0, "must transfer positive quantity" );
        check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
        check( memo.size() <= 256, "memo has more than 256 bytes" );

        sub_balance(from, quantity);
        auto payer = has_auth(to) ? to : from;

        if (payment)
            add_payment( to, quantity, payer );
        else {
            if (to != st.issuer) {
                require_recipient( from );
                require_recipient( to );

                add_balance( to, quantity, payer );
            }
            else {
                auto sub_reserve = calc_reserve_quantity(st, quantity);
                statstable.modify(st, same_payer, [&](auto& s) {
                    s.reserve -= sub_reserve;
                    s.supply -= quantity;
                });

                INLINE_ACTION_SENDER(eosio::token, transfer)(config::token_name, {_self, config::active_name},
                {_self, from, sub_reserve, quantity.symbol.code().to_string() + " sold"});
            }
        }
}

} /// namespace commun

DISPATCH_WITH_TRANSFER(commun::bancor, commun::config::token_name, on_reserve_transfer,
    (create)(issue)(transfer)(payment)(open)(close)(claim)(retire))
