#include <eosio.token/eosio.token.hpp>
#include <common/dispatchers.hpp>
#include <common/config.hpp>

namespace commun {
    
using eosio::name;
using eosio::asset;/*
using eosio::symbol_code;
using eosio::block_timestamp; 
using eosio::time_point;
using eosio::time_point_sec;
using eosio::datastream;*/

class [[eosio::contract("reserve.emit")]] reserve_emit: public eosio::contract {
public:
    using contract::contract;
    void on_transfer(eosio::name from, eosio::name to, eosio::asset quantity, std::string memo) {
        if(_self == to && memo == config::retire_memo && _self == eosio::token::get_issuer(config::token_name, quantity.symbol.code())) {
            eosio::print("retire ", quantity, "\n");
            INLINE_ACTION_SENDER(eosio::token, retire)(config::token_name, {_self, config::active_name}, {quantity, ""});
        }
    }
};

extern "C" {
void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    if (N(transfer).value == action && config::token_name.value == code)
        eosio::execute_action(eosio::name(receiver), eosio::name(code), &reserve_emit::on_transfer);
}
}

} // commun
