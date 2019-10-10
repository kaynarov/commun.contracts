//based on eosio.token
/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include "config.hpp"

#include <string>
#include <cmath>
#include "commun.point/commun.point.hpp"
#include <commun/bancor.hpp>

namespace commun {

using std::string;
using namespace eosio;

class [[eosio::contract("comn.point")]] point : public contract {
public:
    using contract::contract;

    [[eosio::action]]
    void create(name issuer, asset maximum_supply, int16_t cw, int16_t fee);

    [[eosio::action]]
    void setfreezer(name freezer);

    [[eosio::action]]
    void issue(name to, asset quantity, string memo);

    [[eosio::action]]
    void retire(name from, asset quantity, string memo);

    [[eosio::action]]
    void transfer(name from, name to, asset quantity, string memo);

    [[eosio::action]]
    void open(name owner, symbol_code commun_code, std::optional<name> ram_payer);

    [[eosio::action]]
    void close(name owner, symbol_code commun_code);
    
    void on_reserve_transfer(name from, name to, asset quantity, std::string memo);
    
    [[eosio::action]]
    void withdraw(name owner, asset quantity);

    static inline bool exist(name token_contract_account, symbol_code commun_code) {
        stats stats_table(token_contract_account, commun_code.raw());
        return stats_table.find(commun_code.raw()) != stats_table.end();
    }

    static inline asset get_supply(name token_contract_account, symbol_code commun_code) {
        stats stats_table(token_contract_account, commun_code.raw());
        const auto& st = stats_table.get(commun_code.raw(), "point with symbol does not exist");
        return st.supply;
    }
    
    static inline asset get_reserve(name token_contract_account, symbol_code commun_code) {
        stats stats_table(token_contract_account, commun_code.raw());
        const auto& st = stats_table.get(commun_code.raw(), "point with symbol does not exist");
        return st.reserve;
    }

    static inline asset get_balance(name token_contract_account, name owner, symbol_code commun_code) {
        accounts accountstable(token_contract_account, owner.value);
        const auto& ac = accountstable.get(commun_code.raw(), "balance does not exist");
        return ac.balance;
    }

    static inline name get_issuer(name token_contract_account, symbol_code commun_code) {
        params params_table(token_contract_account, token_contract_account.value);
        const auto& param = params_table.get(commun_code.raw(), "point with symbol does not exist");
        return param.issuer;
    }
    
    static inline int64_t get_assigned_reserve_amount(name token_contract_account, name owner) {
        params params_table(token_contract_account, token_contract_account.value);
        auto params_idx = params_table.get_index<"byissuer"_n>();
        accounts accounts_table(token_contract_account, owner.value);
        auto param = params_idx.find(owner);
        
        auto ac = accounts_table.find(symbol_code().raw());
        eosio::check(param != params_idx.end() || ac != accounts_table.end(), "no assigned reserve");
        return (param != params_idx.end()  ? get_reserve(token_contract_account, symbol_code()).amount : 0) + 
               (ac != accounts_table.end() ? ac->balance.amount : 0);
    }

    static inline asset get_reserve_quantity(name token_contract_account, asset token_quantity, bool apply_fee = true) {
        auto commun_code = token_quantity.symbol.code();
        params params_table(token_contract_account, token_contract_account.value);
        const auto& param = params_table.get(commun_code.raw(), "point with symbol does not exist");
        stats stats_table(token_contract_account, commun_code.raw());
        const auto& st = stats_table.get(commun_code.raw());
        return calc_reserve_quantity(param, st, token_quantity, apply_fee);
    }

    static inline asset get_token_quantity(name token_contract_account, symbol_code commun_code, asset reserve_quantity) {
        params params_table(token_contract_account, token_contract_account.value);
        const auto& param = params_table.get(commun_code.raw(), "point with symbol does not exist");
        stats stats_table(token_contract_account, commun_code.raw());
        const auto& st = stats_table.get(commun_code.raw());
        return calc_token_quantity(param, st, reserve_quantity);
    }

    static bool balance_exists(name token_contract_account, name owner, symbol_code commun_code) {
        accounts accountstable(token_contract_account, owner.value);
        return accountstable.find(commun_code.raw()) != accountstable.end();
    }

private:
struct structures {

    struct [[eosio::table]] account {
        asset    balance;
        uint64_t primary_key()const { return balance.symbol.code().raw(); }
    };

    struct [[eosio::table]] stat {
        asset    supply;
        asset    reserve;
        uint64_t primary_key()const { return supply.symbol.code().raw(); }
    };

    struct [[eosio::table]] param {
        asset max_supply;
        int16_t  cw; //connector weight
        int16_t fee;
        name    issuer;
        uint64_t primary_key()const { return max_supply.symbol.code().raw(); }
        name by_issuer()const { return issuer; }
    };

    struct [[eosio::table]] global_param {
        name point_freezer;
    };
};

    using param_id_index = eosio::indexed_by<"paramid"_n, eosio::const_mem_fun<structures::param, uint64_t, &structures::param::primary_key> >;
    using param_issuer_index = eosio::indexed_by<"byissuer"_n, eosio::const_mem_fun<structures::param, name, &structures::param::by_issuer> >;
    using params = eosio::multi_index<"param"_n, structures::param, param_id_index, param_issuer_index>;
    
    using stats  = eosio::multi_index<"stat"_n,  structures::stat>;
    using accounts  = eosio::multi_index<"accounts"_n,  structures::account>;

    using global_params = eosio::singleton<"globalparam"_n, structures::global_param>;

    void notify_balance_change(name owner, asset diff);
    void sub_balance(name owner, asset value);
    void add_balance(name owner, asset value, name ram_payer);

    static inline double get_cw(const structures::param& param) {
        return static_cast<double>(param.cw) / static_cast<double>(commun::config::_100percent);
    }

    static inline asset calc_reserve_quantity(const structures::param& param, const structures::stat& stat, asset token_quantity, bool apply_fee = true) {
        check(token_quantity.amount >= 0, "can't convert negative quantity");
        check(token_quantity.amount <= stat.supply.amount, "can't convert more than supply");
        if (token_quantity.amount == 0)
            return asset(0, stat.reserve.symbol);
        int64_t ret = 0;
        if (token_quantity.amount == stat.supply.amount)
            ret = stat.reserve.amount;
        else if (param.cw == commun::config::_100percent)
            ret = static_cast<int64_t>((static_cast<int128_t>(token_quantity.amount) * stat.reserve.amount) / stat.supply.amount);
        else {
            double sell_prop = static_cast<double>(token_quantity.amount) / static_cast<double>(stat.supply.amount);
            ret = static_cast<int64_t>(static_cast<double>(stat.reserve.amount) * (1.0 - std::pow(1.0 - sell_prop, 1.0 / get_cw(param))));
        }

        if (apply_fee && param.fee)
            ret = (static_cast<int128_t>(ret) * (commun::config::_100percent - param.fee)) / commun::config::_100percent;
        return asset(ret, stat.reserve.symbol);
    }

    static inline asset calc_token_quantity(const structures::param& param, const structures::stat& stat, asset reserve_quantity) {
        return asset(calc_bancor_amount(stat.reserve.amount, stat.supply.amount, get_cw(param), reserve_quantity.amount), stat.supply.symbol);
    }

    void do_transfer(name from, name to, const asset& quantity, const string& memo); 

    struct currency_event {
        asset   supply;
        asset   reserve;
        asset   max_supply;
        int16_t cw; //connector weight
        int16_t fee;
        name    issuer;
    };

    struct balance_event {
        name    account;
        asset   balance;
    };

    void send_currency_event(const structures::stat& st, const structures::param& par);

    void send_balance_event(name acc, const structures::account& accinfo);
};
} /// namespace commun
