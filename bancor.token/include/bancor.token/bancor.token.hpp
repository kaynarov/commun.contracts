//based on eosio.token
/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include "config.hpp"

#include <string>
#include <cmath>

namespace commun {

using std::string;
using namespace eosio;

class [[eosio::contract("bancor.token")]] bancor : public contract {
public:
    using contract::contract;

    [[eosio::action]]
    void create(name issuer, asset maximum_supply, int16_t cw, int16_t fee);

    [[eosio::action]]
    void issue(name to, asset quantity, string memo);

    [[eosio::action]]
    void retire(asset quantity, string memo);

    [[eosio::action]]
    void transfer(name from, name to, asset quantity, string memo);
    
    [[eosio::action]]
    void open(name owner, const symbol& symbol, name ram_payer);

    [[eosio::action]]
    void close(name owner, const symbol& symbol);
    
    void on_reserve_transfer(name from, name to, asset quantity, std::string memo);
    
    static inline bool exist(name token_contract_account, symbol_code sym_code) {
        stats statstable(token_contract_account, sym_code.raw());
        return statstable.find(sym_code.raw()) != statstable.end();
    }

    static inline asset get_supply(name token_contract_account, symbol_code sym_code) {
        stats statstable(token_contract_account, sym_code.raw());
        const auto& st = statstable.get(sym_code.raw());
        return st.supply;
    }

    static inline asset get_balance(name token_contract_account, name owner, symbol_code sym_code) {
        accounts accountstable(token_contract_account, owner.value);
        const auto& ac = accountstable.get(sym_code.raw());
        return ac.balance;
    }

    static inline name get_issuer(name token_contract_account, symbol_code sym_code) {
        stats statstable(token_contract_account, sym_code.raw());
        const auto& st = statstable.get(sym_code.raw());
        return st.issuer;
    }
    
    static inline asset get_reserve_quantity(name token_contract_account, asset token_quantity) {
        auto sym_code = token_quantity.symbol.code();
        stats statstable(token_contract_account, sym_code.raw());
        const auto& st = statstable.get(sym_code.raw());
        return calc_reserve_quantity(st, token_quantity);
    }
    
    static inline asset get_token_quantity(name token_contract_account, symbol_code sym_code, asset reserve_quantity) {
        stats statstable(token_contract_account, sym_code.raw());
        const auto& st = statstable.get(sym_code.raw());
        return calc_token_quantity(st, reserve_quantity);
    }

private:
    struct [[eosio::table]] account {
        asset    balance;

        uint64_t primary_key()const { return balance.symbol.code().raw(); }
    };

    struct [[eosio::table]] currency_stats {
        asset    supply;
        asset    max_supply;
        asset    reserve;
        int16_t  cw; //connector weight
        int16_t fee;
        name     issuer;
        uint64_t primary_key()const { return supply.symbol.code().raw(); }
    };

    typedef eosio::multi_index< "accounts"_n, account > accounts;
    typedef eosio::multi_index< "stat"_n, currency_stats > stats;

    void sub_balance(name owner, asset value);
    void add_balance(name owner, asset value, name ram_payer);
    
    using real_type = double; //should we use fixed point? or maybe we should use floating point everywere (?)
    
    static inline real_type get_cw(const currency_stats& st) {
        return static_cast<real_type>(st.cw) / static_cast<real_type>(config::_100percent);
    }
    
    static inline asset calc_reserve_quantity(const currency_stats& st, asset token_quantity) {
        eosio_assert(token_quantity.amount >= 0, "can't convert negative quantity");
        eosio_assert(token_quantity.amount <= st.supply.amount, "can't convert more than supply");
        if(token_quantity.amount == 0)
            return asset(0, st.reserve.symbol);
        int64_t ret = 0;
        if(token_quantity.amount == st.supply.amount)
            ret = st.reserve.amount;
        else if(st.cw == config::_100percent)
            ret = static_cast<int64_t>((static_cast<int128_t>(token_quantity.amount) * st.reserve.amount) / st.supply.amount);
        else {
            real_type sell_prop = static_cast<real_type>(token_quantity.amount) / static_cast<real_type>(st.supply.amount);
            ret = static_cast<int64_t>(static_cast<real_type>(st.reserve.amount) * (1.0 - std::pow(1.0 - sell_prop, 1.0 / get_cw(st))));
        }
        
        if(st.fee)
            ret = (static_cast<int128_t>(ret) * (config::_100percent - st.fee)) / config::_100percent;
        return asset(ret, st.reserve.symbol);
    }

    static inline asset calc_token_quantity(const currency_stats& st, asset reserve_quantity) {
        eosio_assert(st.reserve.amount > 0, "token has no reserve");
        real_type buy_prop = static_cast<real_type>(reserve_quantity.amount) / static_cast<real_type>(st.reserve.amount);
        real_type new_supply = static_cast<real_type>(st.supply.amount) * std::pow(1.0 + buy_prop, get_cw(st));
        eosio_assert(new_supply <= static_cast<real_type>(std::numeric_limits<int64_t>::max()), "invalid supply, int64_t overflow");
        return asset(static_cast<int64_t>(new_supply) - st.supply.amount, st.supply.symbol);
    }
};
} /// namespace commun
