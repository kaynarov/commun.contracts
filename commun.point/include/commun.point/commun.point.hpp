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
#include <commun/util.hpp>

namespace commun {

using std::string;
using namespace eosio;
/**
* \brief This class implements the commun.point contract functionality
* \ingroup point_class
*/
class [[eosio::contract]] point : public contract {
public:
    using contract::contract;

    /**
        \brief The \ref create action is used to create a new type (a symbol) of point token and supply it into the community.

        \param issuer account issuing the token
        \param initial_supply number of point tokens supplied initially. These tokens are on the issuer's balance sheet. The initial_supply parameter must be at most maximum_supply and must have the same symbol
        \param maximum_supply maximum number of point tokens supplied
        \param cw connector weight showing the exchange rate between the point token and the system one, calculted by the specified formula. This parameter should take value from «0» to «10000» inclusive («0» and «10000» equal to «0.00 % » and «100.00 % » correspondingly)
        \param fee amount of commission that should be charged from a user when exchanging created tokens for system ones. This parameter is set in the same way as \a cw one

        The asset type is a structure value containing the fields:
           - maximum number of tokens supplied;
           - token symbol (data type that uniquely identifies the token):
               + token name consisting of a set of capital letters;
               + field that specifies a token cost accuracy in the form of decimal places number.

        When this action is called, the information about \a currency event is sent to the event engine.

        This action requires the signature of most validators.
    */
    [[eosio::action]]
    void create(name issuer, asset initial_supply, asset maximum_supply, int16_t cw, int16_t fee);

    /**
        \brief The \ref setparams action is used by a point issuer to set the parameters of the issued point token. These settings can be updated after issuing the token.

        \param commun_code the point symbol code
        \param transfer_fee commission (in percent) charged from the amount of token transfer. This parameter should be at least min_transfer_fee_points. The comission and the amount transferred are debited from balance of the «from» account 
        \param min_transfer_fee_points minimum amount (in percent) of fee charged for transfer of point tokens

        <b>Requirements:</b>
            - the point token should be created before calling this action;
            - the \a min_transfer_fee_points parameter should take the value «0» if \a transfer_fee is set to «0» (%);
            - no fee is charged on transfer if the «from» (or «to») account is either a point issuer or is one of the commun contracts.

        Performing the action requires the point issuer signature.
    */
    [[eosio::action]]
    void setparams(symbol_code commun_code, uint16_t transfer_fee, int64_t min_transfer_fee_points);

    /**
        \brief The \ref setfreezer action is used to set contract account, so, this account will have ability to «freeze» the point tokens or the system ones on its balance.

        \param freezer a freezer account

        This action requires the signature of most validators.
    */
    [[eosio::action]]
    void setfreezer(name freezer);


    /**
        \brief The \ref issue action is used to put the point tokens into circulation in the system.

        \param to recipient account, on whose balance the point tokens are credited
        \param quantity number of the supplied tokens
        \param memo memo text that clarifies a meaning (necessity) of the point tokens emission in the system. This text should not exceed 256 symbols including blanks

        The number of supplied point tokens should not exceed the maximum_supply value specified by the \ref create action.
        When this action is called, the information about \a currency and \a balance events is sent to the event engine.

        This action requires a signature either of the point tokens issuer or of most validators.
    */
    [[eosio::action]]
    void issue(name to, asset quantity, string memo);

    /**
        \brief The \ref retire action is used for taking a certain number of point tokens out of circulation («burning» the point tokens). These tokens can be withdrawn from balance of the \a issuer account as well as from balance of any other account.

        \param from account from the balance of which tokens are withdrawn
        \param quantity number of retiring tokens. This parameter should be positive
        \param memo a memo text clarifying a purpose of retiring tokens. Should not exceed 256 symbols including blanks

        When this action is called, the information about \a currency and \a balance events is sent to the event engine.

        Use of the bandwidth resources (RAM) is charged to the account \a from. The number of tokens withdrawn from circulation will also be deducted from account balance, so this account (\a from) can not withdraw tokens more than he/she has them on own balance.
        This action requires a signature of the account \a from..
    */
    [[eosio::action]]
    void retire(name from, asset quantity, string memo);

    /**
        \brief The \ref transfer action is used to transfer tokens from one account to another, as well as to exchange point tokens for system tokens at the current exchange rate.

        \param from sender account from balance of which the tokens are withdrawn
        \param to recipient account to balance of which the tokens are transferred. This parameter takes the point contract name, in case of selling point tokens
        \param quantity amount of tokens to be transferred. This value should be positive
        \param memo a memo text that clarifies a meaning of the tokens transfer. This text should not exceed 256 symbols including blanks.

        When this action is called, the information about \a balance, \a currency and \a exchange events is sent to the event engine.

        <b>Restriction:</b>
            - It is not allowed to transfer tokens to oneself.

        This action requires a signature either of the \a from account or of most validators.
        Use of the bandwidth (RAM) resources is charged either to sending account or to receiving account, depending on who signed the transaction. If the \ref open action was previously performed, none of them should pay bandwidth, since the record already created in database is used.
    */
    [[eosio::action]]
    void transfer(name from, name to, asset quantity, string memo);

    /**
        \brief The \ref open action is used to create a record in database. This record contains information about either point or system token symbol on the account balance.

        \param owner account name for which the memory is allocated
        \param commun_code a token symbol for which the record is being created
        \param ram_payer account name that pays for the used memory

        This action requires a signature of account specified in the \a ram_payer parameter.
     */
    [[eosio::action]]
    void open(name owner, symbol_code commun_code, std::optional<name> ram_payer);

    /**
        \brief The \ref close action is used to free memory in the database previously allocated for information about token symbol on the account balance. This action is an opposite of the \ref open one.

        \param owner account name to which the record in database was created
        \param commun_code a token symbol for which the record in database was cleared

        To perform this action, it is necessary to have two zeroed balances of the account \a owner:
            - zero token balance (determined by the \a commun_code);
            - zero payment balance.

        This action requires a signature of the \a owner account.
     */
    [[eosio::action]]
    void close(name owner, symbol_code commun_code);

    void on_reserve_transfer(name from, name to, asset quantity, std::string memo);

    /**
        \brief The \ref withdraw action is used to debit system tokens from the commun.point balance and credit them to the <i>cyber.token</i> balance. It should be noted that the \ref transfer action can be used for the reverse operation — to debit system tokens from the <i>cyber.token</i> balance and credit them to commun.point balance.

        \param owner account name withdrawing the tokens
        \param quantity number of tokens to be withdrawn

        When this action is called, the information about \a balance event is sent to the event engine.

        This action requires a signature of the \a owner account.
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

    static inline asset get_reserve_quantity(asset token_quantity, asset* fee_quantity) {
        auto commun_code = token_quantity.symbol.code();
        params params_table(config::point_name, config::point_name.value);
        const auto& param = params_table.get(commun_code.raw(), "point with symbol does not exist");
        stats stats_table(config::point_name, commun_code.raw());
        const auto& st = stats_table.get(commun_code.raw());
        return calc_reserve_quantity(param, st, token_quantity, fee_quantity);
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

      \brief DB record containing information about tokens of a certain symbol on the account balance
      \ingroup point_tables
    */
    // DOCS_TABLE: account_struct
    struct [[eosio::table]] account {
        asset    balance; /**< number of point tokens or of system ones on the account balance*/
        uint64_t primary_key()const { return balance.symbol.code().raw(); }
    };

    /**

      \brief DB record containing statistical information about tokens of a certain symbol in the system.
      \ingroup point_tables
    */
    // DOCS_TABLE: stat_struct
    struct [[eosio::table]] stat {
        asset    supply; /**< number of point tokens that are in circulation in the system */
        asset    reserve; /**< number of system tokens that are not in circulation in the system. These tokens can be used for exchange */
        uint64_t primary_key()const { return supply.symbol.code().raw(); }
    };


    /**
      \brief DB record containing data for a point token.
      \ingroup point_tables
    */
    // DOCS_TABLE: param_struct
    struct [[eosio::table]] param {
        asset max_supply; /**< maximum number of the point tokens supplied*/
        int16_t  cw; /**< connector weight showing the exchange rate between the point token and the system one*/
        int16_t fee; /**< amount of commission that is charged from the \a issuer when exchanging the issued tokens for the system ones */
        name     issuer; /**< account who issued the point tokens */
        uint16_t transfer_fee = config::def_transfer_fee; //!< fee charged for transfer of the point tokens
        int64_t min_transfer_fee_points = config::def_min_transfer_fee_points; //!< minimum amount of fee charged for transfer of point tokens

        uint64_t primary_key()const { return max_supply.symbol.code().raw(); }
        name by_issuer()const { return issuer; }
    };

    /**
      \brief The struct representing global params table in DB.
      \ingroup point_tables
    */
    // DOCS_TABLE: globalparam_struct
    struct [[eosio::table]] global_param {
        name point_freezer; /**< a name of contract that has the ability to freeze tokens of accounts*/
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

    static inline asset calc_reserve_quantity(const structures::param& param, const structures::stat& stat, asset token_quantity, asset* fee_quantity) {
        check(token_quantity.amount >= 0, "can't convert negative quantity");
        check(token_quantity.amount <= stat.supply.amount, "can't convert more than supply");
        if (token_quantity.amount == 0)
            return asset(0, stat.reserve.symbol);
        int64_t ret = 0;
        if (token_quantity.amount == stat.supply.amount)
            ret = stat.reserve.amount;
        else if (param.cw == commun::config::_100percent)
            ret = safe_prop(token_quantity.amount, stat.reserve.amount, stat.supply.amount);
        else {
            double sell_prop = static_cast<double>(token_quantity.amount) / static_cast<double>(stat.supply.amount);
            ret = static_cast<int64_t>(static_cast<double>(stat.reserve.amount) * (1.0 - std::pow(1.0 - sell_prop, 1.0 / get_cw(param))));
        }

        if (fee_quantity) {
            if (param.fee) {
                auto initial = ret;
                ret = safe_pct(ret, config::_100percent - param.fee);
                *fee_quantity = asset(initial - ret, stat.reserve.symbol);
            } else {
                *fee_quantity = asset(0, stat.reserve.symbol);
            }
        }

        return asset(ret, stat.reserve.symbol);
    }

    static inline asset calc_token_quantity(const structures::param& param, const structures::stat& stat, asset reserve_quantity) {
        return asset(calc_bancor_amount(stat.reserve.amount, stat.supply.amount, get_cw(param), reserve_quantity.amount), stat.supply.symbol);
    }

    static inline bool no_fee_transfer(name issuer, name from, name to) {
        return from == issuer || to == issuer || get_prefix(from) == config::dapp_name || get_prefix(to) == config::dapp_name;
    };

    void do_transfer(name from, name to, const asset& quantity, const string& memo);

    /**
      \brief The structure representing the event related to change of account balance. Such event occurs during execution of the actions \ref create, \ref issue and \ref retire as well as during the reserve tokens transfer from \a cyber.token to commun.point via performing \ref transfer.
      \ingroup point_events
    */
    struct currency_event {
        asset   supply; //!< number of point tokens (with a specific symbol) supplied
        asset   reserve; //!< number of reserve system tokens
        asset   max_supply; //!< maximum number of the point tokens supplied
        int16_t cw; //!< connector weight for the point token
        int16_t fee; //!< fee for the point token
        name    issuer; //!< account name issuing the point token
        uint16_t transfer_fee = config::def_transfer_fee; //!< fee charged for transfer of the point tokens
        int64_t min_transfer_fee_points = config::def_min_transfer_fee_points; //!< minimum amount of fee charged for transfer of point tokens
    };

    /**
      \brief The structure representing the event related to change of account balance. Such event occurs during execution of the actions \ref retire, \ref withdraw, \ref transfer and \ref issue, as well as during a transfer of system tokens to the commun.point contract.
      \ingroup point_events
    */
    struct balance_event {
        name    account; //!< account name whose balance is changing
        asset   balance; //!< balance opened by the account for a concrete token
    };

    /**
      \brief The structure representing the event related to the purchase of a certain amount of system tokens in exchange for point tokens, as well as the event related to the purchase of a certain amount of point tokens. The first event occurs during the \ref transfer execution while the second one occurs during transfering system tokens to <i>commun.point</i> contract.
      \ingroup point_events
    */
    struct exchange_event {
        asset   amount; //!< this parameter is either amount of system tokens when point tokens are selling or amount of point tokens when they are buying
    };

    /**
      \brief The structure representing the events related to:
       - amount of point tokens is to be debited as a fee for the transfer of point tokens
       - amount of reserve CMN tokens is to be debited as a fee for the purchase of CMN tokens when selling point tokens.
      \ingroup point_events
    */
    struct fee_event {
        asset   amount; //!< amount of tokens is to be debited as a fee
    };

    void send_currency_event(const structures::stat& st, const structures::param& par);

    void send_balance_event(name acc, const structures::account& accinfo);

    void send_exchange_event(const asset& amount);

    void send_fee_event(const asset& amount);
};
} /// namespace commun
