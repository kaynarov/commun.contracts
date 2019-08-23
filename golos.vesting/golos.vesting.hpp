#pragma once
#include "parameters.hpp"
#include <eosio/time.hpp>
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/transaction.hpp>
#include "config.hpp"

namespace golos {


using namespace eosio;

class vesting : public contract {
public:
    using contract::contract;

    [[eosio::action]] void validateprms(symbol symbol, std::vector<vesting_param>);
    [[eosio::action]] void setparams(symbol symbol, std::vector<vesting_param>);

    [[eosio::action]] void retire(asset quantity, name user);
    [[eosio::action]] void unlocklimit(name owner, asset quantity);

    [[eosio::action]] void withdraw(name from, name to, asset quantity);
    [[eosio::action]] void stopwithdraw(name owner, symbol type);
    [[eosio::action]] void delegate(name delegator, name delegatee, asset quantity, uint16_t interest_rate);
    [[eosio::action]] void undelegate(name delegator, name delegatee, asset quantity);

    [[eosio::action]] void create(symbol symbol, name notify_acc);

    [[eosio::action]] void open(name owner, symbol symbol, name ram_payer);
    [[eosio::action]] void close(name owner, symbol symbol);

    [[eosio::action]] void procwaiting(symbol symbol, name payer);

    void on_transfer(name from, name to, asset quantity, std::string memo);
    void on_bulk_transfer(name from, std::vector<token::recipient> recipients);

    // tables
    struct [[eosio::table]] vesting_stats {
        asset supply;
        name notify_acc;

        uint64_t primary_key() const {
            return supply.symbol.code().raw();
        }
    };

    struct [[eosio::table]] account {
        asset vesting;
        asset delegated;        // TODO: this (incl. 2 fields below) can be share_type to reduce memory usage #554
        asset received;
        asset unlocked_limit;
        uint32_t delegators = 0;

        uint64_t primary_key() const {
            return vesting.symbol.code().raw();
        }

        inline asset available_vesting() const {
            return vesting - delegated;
        }

        inline asset effective_vesting() const {
            return available_vesting() + received;
        }

        inline asset unlocked_vesting() const {
            return std::min(available_vesting(), unlocked_limit);
        }
    };

    struct [[eosio::table]] delegation {
        uint64_t id;
        name delegator;
        name delegatee;
        asset quantity;
        uint16_t interest_rate;
        time_point_sec min_delegation_time;

        auto primary_key() const {
            return id;
        }
    };

    struct [[eosio::table]] return_delegation {
        uint64_t id;
        name delegator;
        asset quantity;
        time_point_sec date;

        uint64_t primary_key() const {
            return id;
        }
    };

    struct [[eosio::table]] withdraw_record {
        name from;
        name to;
        uint32_t interval_seconds;
        uint8_t remaining_payments; // decreasing counter
        time_point_sec next_payout;
        asset withdraw_rate;        // amount of vesting to convert per one withdraw step
        asset to_withdraw;          // remaining amount of vesting to withdraw

        uint64_t primary_key() const {
            return from.value;
        }
    };

    using vesting_table = multi_index<"stat"_n, vesting_stats>;
    using account_table = multi_index<"accounts"_n, account>;

    using delegation_table = multi_index<"delegation"_n, delegation,
        indexed_by<"delegator"_n, composite_key<delegation,
            member<delegation, name, &delegation::delegator>,
            member<delegation, name, &delegation::delegatee>>>,
        indexed_by<"delegatee"_n, member<delegation, name, &delegation::delegatee>>>;

    using return_delegation_table = multi_index<"rdelegation"_n, return_delegation,
        indexed_by<"date"_n, member<return_delegation, time_point_sec, &return_delegation::date>>>;

    using withdraw_table = multi_index<"withdrawal"_n, withdraw_record,
        indexed_by<"nextpayout"_n, member<withdraw_record, time_point_sec, &withdraw_record::next_payout>>>;


    // interface for external contracts
    static inline bool exists(name code, symbol_code sym) {
        vesting_table table(code, code.value);
        return table.find(sym.raw()) != table.end();
    }
    static inline bool balance_exist(name code, name owner, symbol_code sym) {
        account_table accounts(code, owner.value);          // TODO: maybe combine with `get_account`
        return accounts.find(sym.raw()) != accounts.end();
    }
    static inline const account& get_account(name code, name account, symbol_code sym) {
        account_table accounts(code, account.value);
        return accounts.get(sym.raw());    // TODO: this can be cached in static if several calls occur in one action #554
    }

    static inline asset get_account_vesting(name code, name account, symbol_code sym) {
        return get_account(code, account, sym).vesting;
    }
    static inline asset get_account_effective_vesting(name code, name account, symbol_code sym) {
        return get_account(code, account, sym).effective_vesting();
    }
    static inline asset get_account_available_vesting(name code, name account, symbol_code sym) {
        return get_account(code, account, sym).available_vesting();
    }
    static inline asset get_account_unlocked_vesting(name code, name account, symbol_code sym) {
        return get_account(code, account, sym).unlocked_vesting();
    }
    static inline asset get_withdrawing_vesting(name code, name account, symbol sym) {
        withdraw_table tbl(code, sym.code().raw());
        auto obj = tbl.find(account.value);
        return obj != tbl.end() ? obj->to_withdraw : asset(0, sym);
    }
    static inline bool can_retire_vesting(name code, name account, asset value) {
        auto sym = value.symbol;
        auto acc = get_account(code, account, sym.code());
        bool can = acc.unlocked_vesting() >= value;
        if (can) {
            auto withdrawing = get_withdrawing_vesting(code, account, sym);
            can = std::min(acc.available_vesting() - withdrawing, acc.unlocked_limit) >= value;
        }
        return can;
    }

    static inline std::vector<delegation> get_account_delegators(name code, name owner, symbol_code sym) {
        std::vector<delegation> result;
        delegation_table tbl(code, sym.raw());
        auto idx = tbl.get_index<"delegatee"_n>();
        auto itr = idx.find(owner);
        while (itr != idx.end() && itr->delegatee == owner) {
            result.push_back(*itr);
            ++itr;
        }
        return result;
    }

private:
    void providebw_for_trx(eosio::transaction& trx, const permission_level& provider);
    bool process_withdraws(eosio::time_point now, symbol symbol, name payer);
    bool return_delegations(eosio::time_point till, symbol symbol, name payer);
    void do_transfer_vesting(name from, name to, asset quantity, std::string memo);
    void notify_balance_change(name owner, asset diff);
    void sub_balance(name owner, asset value, bool retire_mode = false);
    void add_balance(name owner, asset value, name ram_payer);
    void send_account_event(name account, const struct account& balance);
    void send_stat_event(const vesting_stats& info);

    const asset vesting_to_token(const asset& vesting, const vesting_stats& vinfo, int64_t correction) const;
    const asset token_to_vesting(const asset& token, const vesting_stats& vinfo, int64_t correction) const;
};

} // golos
