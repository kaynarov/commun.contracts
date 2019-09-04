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

class [[eosio::contract("cmmn.point")]] point : public contract {
public:
    using contract::contract;

    [[eosio::action]]
    void create(name issuer, asset maximum_supply, int16_t cw, int16_t fee);

    [[eosio::action]]
    void setfreezer(name freezer);

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

    static inline bool exist(name token_contract_account, symbol_code commun_code) {
        stats stats_table(token_contract_account, commun_code.raw());
        return stats_table.find(commun_code.raw()) != stats_table.end();
    }

    static inline asset get_supply(name token_contract_account, symbol_code commun_code) {
        stats stats_table(token_contract_account, commun_code.raw());
        const auto& st = stats_table.get(commun_code.raw(), "point with symbol does not exist");
        return st.supply;
    }

    static inline asset get_balance(name token_contract_account, name owner, symbol_code commun_code) {
        accounts accountstable(token_contract_account, owner.value);
        const auto& ac = accountstable.get(commun_code.raw(), "balance does not exist");
        return ac.balance;
    }

    static inline name get_issuer(name token_contract_account, symbol_code commun_code) {
        params params_table(token_contract_account, commun_code.raw());
        const auto& param = params_table.get(commun_code.raw(), "point with symbol does not exist");
        return param.issuer;
    }

    static inline asset get_reserve_quantity(name token_contract_account, asset token_quantity, bool apply_fee = true) {
        auto commun_code = token_quantity.symbol.code();
        params params_table(token_contract_account, commun_code.raw());
        const auto& param = params_table.get(commun_code.raw(), "point with symbol does not exist");
        stats stats_table(token_contract_account, commun_code.raw());
        const auto& st = stats_table.get(commun_code.raw());
        return calc_reserve_quantity(param, st, token_quantity, apply_fee);
    }

    static inline asset get_token_quantity(name token_contract_account, symbol_code commun_code, asset reserve_quantity) {
        params params_table(token_contract_account, commun_code.raw());
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
        name     issuer;
        uint64_t primary_key()const { return max_supply.symbol.code().raw(); }
    };

    struct [[eosio::table]] singleton_param {
        name point_freezer;
    };
};

    using params = eosio::multi_index<"param"_n, structures::param>;
    using stats  = eosio::multi_index<"stat"_n,  structures::stat>;
    using accounts  = eosio::multi_index<"account"_n,  structures::account>;

    using singparams = eosio::singleton<"singlparam"_n, structures::singleton_param>;

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

    void do_transfer( name  from, name  to, const asset& quantity, const string& memo);
};
} /// namespace commun