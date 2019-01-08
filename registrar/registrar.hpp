#pragma once
#include <eosiolib/asset.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/privileged.hpp>
#include <eosiolib/singleton.hpp>
#include <string>
#include "parameters.hpp"

namespace commun {

using eosio::name;
using eosio::asset;
using eosio::symbol_code;
using eosio::block_timestamp;
using eosio::time_point;
using eosio::time_point_sec;
using eosio::datastream;

struct [[eosio::table, eosio::contract("registrar")]] name_bid {
    symbol_code     sym_code;
    name            high_bidder;
    int64_t         high_bid = 0;   ///< negative high_bid == closed auction waiting to be claimed
    time_point_sec  last_bid_time;

    uint64_t primary_key()  const { return sym_code.raw(); }
    int64_t by_high_bid()   const { return high_bid; }      // ordered desc, check abi
};

struct [[eosio::table, eosio::contract("registrar")]] name_ask {
    symbol_code     sym_code;
    int64_t         price;
    uint64_t primary_key()  const { return sym_code.raw(); }
};

struct [[eosio::table, eosio::contract("registrar")]] name_bid_refund {
    name  bidder;
    asset amount;

    uint64_t primary_key() const { return bidder.value; }
};

using name_bid_tbl = eosio::multi_index<"namebid"_n, name_bid,
    eosio::indexed_by<"highbid"_n, eosio::const_mem_fun<name_bid, int64_t, &name_bid::by_high_bid> >
>;

using name_ask_tbl = eosio::multi_index<"nameask"_n, name_ask>;

using name_bid_refund_tbl = eosio::multi_index< "namebidrefnd"_n, name_bid_refund>;

struct [[eosio::table("state"), eosio::contract("registrar")]] name_bid_state {
    time_point_sec last_win;
    time_point_sec last_checkwin;

    // explicit serialization macro is not necessary, used here only to improve compilation time
    EOSLIB_SERIALIZE(name_bid_state, (last_win)(last_checkwin))
};

using state_singleton = eosio::singleton<"namebidstate"_n, name_bid_state>;

class [[eosio::contract("registrar")]] registrar: public eosio::contract {
    void add_refund(name payer, name bidder, int64_t amount, symbol_code sym_code);
    static int64_t net_amount(int64_t amount, int16_t fee) {
        return (static_cast<int128_t>(amount) * (config::_100percent - fee)) / config::_100percent;
    };
    void retire_fee(int64_t amount);
    registrar_param_singleton _cfg;
    registrar_param_state props() { return _cfg.get(); }
public:
    registrar(name self, name code, datastream<const char*> ds)
        : contract(self, code, ds)
        , _cfg(_self, _self.value) {}
    [[eosio::action]] void validateprms(std::vector<registrar_param>);
    [[eosio::action]] void setparams(std::vector<registrar_param>);

    [[eosio::action]] void checkwin();
    void on_transfer(name from, name to, asset quantity, std::string memo);
    [[eosio::action]] void claimrefund(name bidder, symbol_code sym_code);
    [[eosio::action]] void create(asset maximum_supply, int16_t cw, int16_t fee);
    [[eosio::action]] void setprice(symbol_code sym_code, asset price);
};

} /// commun
