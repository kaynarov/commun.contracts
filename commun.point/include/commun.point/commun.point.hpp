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
/**
* \brief This class implements comn.point contract behaviour
* \ingroup point_class
*/
class [[eosio::contract]] point : public contract {
public:
    using contract::contract;

    /**
        \brief Create point token.

        \param issuer an account who issues the token
        \param maximum_supply a structure value containing the fields:
                - maximum number of tokens supplied;
                - token symbol (data type that uniquely identifies the token). This is a structure value containing fields:
                   + token name, consisting of a set of capital letters;
                   + field that specifies a token cost accuracy in the form of decimal places number.
        \param cw connector weight of creating token to the system token
        \param fee fee charging from account when it selling point tokens for system tokens

        Performing the action requires a validators signature.
    */
    [[eosio::action]]
    void create(name issuer, asset maximum_supply, int16_t cw, int16_t fee);


    /**
        \brief Set contract account which can freeze point or system tokens from account balance.

        \param freezer a freezer account

        Performing the action requires the validators signature.
    */
    [[eosio::action]]
    void setfreezer(name freezer);


    /**
        \brief Put the point tokens into circulation in the system.

        \param to recipient account to balance of which the tokens are transferred.
        \param quantity number of the supplied tokens. This is a structure value containing fields:
                - number of the supplied tokens in the system;
                - the token symbol. This is a structure value containing fields:
                    + the token name, consisting of a set of capital letters;
                    + the field that specifies a token cost accuracy in the form of decimal places number.
        \param memo memo text that clarifies a meaning (necessity) of the token emission in the system. Should not exceed 256 symbols including blanks.

        Performing the action requires a signature of token issuer, or of the validators.
    */
    [[eosio::action]]
    void issue(name to, asset quantity, string memo);

    /**
        \brief Retire a certain number of point tokens from the issuer account, or another account.

        \param from account from which tokens as retiring
        \param quantity number of retiring tokens
        \param memo a memo text clarifying a purpose of retiring tokens. Should not exceed 256 symbols including blanks.


        Use of the bandwidth resources (RAM) is charged to the from account. The number of tokens withdrawn from circulation is also removed from from account balance, so this account can not withdraw tokens more than he/she has them on own balance.
        To perform this action, the from account authorization is required.
    */
    [[eosio::action]]
    void retire(name from, asset quantity, string memo);

    /**
        \brief Transfer point tokens from one account to another, or sell point tokens for system tokens.

        \param from sender account from balance of which the tokens are withdrawn
        \param to recipient account to balance of which the tokens are transferred, or equals to point contract name if selling point tokens
        \param quantity amount of tokens to be transferred. This value should be greater than zero
        \param memo a memo text that clarifies a meaning of the tokens transfer. Should not exceed 256 symbols including blanks.

        Performing the action requires a signature of the from account, or of the validators.
    */
    [[eosio::action]]
    void transfer(name from, name to, asset quantity, string memo);

    /**
        \brief Create a record in database for account balance for point or system symbol.

        \param owner account name to which the memory is allocated
        \param commun_code symbol for which the entry is being created
        \param ram_payer account name that pays for the used memory

        Performing the action requires a signature of the ram_payer account.
     */
    [[eosio::action]]
    void open(name owner, symbol_code commun_code, std::optional<name> ram_payer);

    /**
        \brief Action is an opposite of open and is used to free allocated memory in database.

        \param owner  account name to which the memory is being created
        \param commun_code symbol for which the entry is being cleared

        Performing the action requires a signature of the owner account.
     */
    [[eosio::action]]
    void close(name owner, symbol_code commun_code);

    void on_reserve_transfer(name from, name to, asset quantity, std::string memo);

    /**
        \brief Withdraw a certain number of system tokens from the account to its balance in system token contract.

        \param owner account name to which the tokens withdrawing
        \param quantity amount of withdrawing tokens

        Performing the action requires a signature of the owner account.
     */
    [[eosio::action]]
    void withdraw(name owner, asset quantity);

    static inline bool exist(symbol_code commun_code) {
        stats stats_table(config::point_name, commun_code.raw());
        return stats_table.find(commun_code.raw()) != stats_table.end();
    }

    static inline asset get_supply(symbol_code commun_code) {
        stats stats_table(config::point_name, commun_code.raw());
        const auto& st = stats_table.get(commun_code.raw(), "point with symbol does not exist");
        return st.supply;
    }
    
    static inline asset get_reserve(symbol_code commun_code) {
        stats stats_table(config::point_name, commun_code.raw());
        const auto& st = stats_table.get(commun_code.raw(), "point with symbol does not exist");
        return st.reserve;
    }

    static inline asset get_balance(name owner, symbol_code commun_code) {
        accounts accountstable(config::point_name, owner.value);
        const auto& ac = accountstable.get(commun_code.raw(), "balance does not exist");
        return ac.balance;
    }

    static inline name get_issuer(symbol_code commun_code) {
        params params_table(config::point_name, config::point_name.value);
        const auto& param = params_table.get(commun_code.raw(), "point with symbol does not exist");
        return param.issuer;
    }
    
    static inline int64_t get_assigned_reserve_amount(name owner) {
        params params_table(config::point_name, config::point_name.value);
        auto params_idx = params_table.get_index<"byissuer"_n>();
        accounts accounts_table(config::point_name, owner.value);
        auto param = params_idx.find(owner);
        
        auto ac = accounts_table.find(symbol_code().raw());
        eosio::check(param != params_idx.end() || ac != accounts_table.end(), "no assigned reserve");
        return (param != params_idx.end()  ? get_reserve(param->max_supply.symbol.code()).amount : 0) +
               (ac != accounts_table.end() ? ac->balance.amount : 0);
    }

    static inline asset get_reserve_quantity(asset token_quantity, bool apply_fee = true) {
        auto commun_code = token_quantity.symbol.code();
        params params_table(config::point_name, config::point_name.value);
        const auto& param = params_table.get(commun_code.raw(), "point with symbol does not exist");
        stats stats_table(config::point_name, commun_code.raw());
        const auto& st = stats_table.get(commun_code.raw());
        return calc_reserve_quantity(param, st, token_quantity, apply_fee);
    }

    static inline asset get_token_quantity(symbol_code commun_code, asset reserve_quantity) {
        params params_table(config::point_name, config::point_name.value);
        const auto& param = params_table.get(commun_code.raw(), "point with symbol does not exist");
        stats stats_table(config::point_name, commun_code.raw());
        const auto& st = stats_table.get(commun_code.raw());
        return calc_token_quantity(param, st, reserve_quantity);
    }

    static bool balance_exists(name owner, symbol_code commun_code) {
        accounts accountstable(config::point_name, owner.value);
        return accountstable.find(commun_code.raw()) != accountstable.end();
    }

private:

struct structures {

    /**

      \brief struct represents an account table in a db
      \ingroup point_tables
    */
    struct [[eosio::table]] account {
        asset    balance; /**< amount of point or system tokens belongs to the account*/
        uint64_t primary_key()const { return balance.symbol.code().raw(); }
    };

    /**

      \brief struct represents token statistics table in a db
      \ingroup point_tables
    */
    struct [[eosio::table]] stat {
        asset    supply; /**< amount of tokens used in the system, point tokens */
        asset    reserve; /**< amount of tokens not used in the system, system tokens */
        uint64_t primary_key()const { return supply.symbol.code().raw(); }
    };


    /**
      \brief struct represents params table in a db
      \ingroup point_tables
    */
    struct [[eosio::table]] param {
        asset max_supply; /**< maximum amount of point tokens supplied*/
        int16_t  cw; /**< connector weight*/
        int16_t fee; /**< fee */
        name     issuer; /**< an token issuer */

        uint64_t primary_key()const { return max_supply.symbol.code().raw(); }
        name by_issuer()const { return issuer; }
    };

    /**
      \brief struct represents global params table in a db
      \ingroup point_tables
    */
    struct [[eosio::table]] global_param {
        name point_freezer; /**< a name of contract which can freeze tokens of accounts*/
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

    struct exchange_event {
        asset   amount;
    };

    void send_currency_event(const structures::stat& st, const structures::param& par);

    void send_balance_event(name acc, const structures::account& accinfo);

    void send_exchange_event(const asset& amount);
};
} /// namespace commun
