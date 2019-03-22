#include "registrar.hpp"
#include "config.hpp"
#include <cyber.token/cyber.token.hpp>
#include <bancor.token/bancor.token.hpp>
#include <bancor.token/config.hpp>
#include <eosiolib/transaction.hpp>
#include <common/dispatchers.hpp>
#include <common/parameter_ops.hpp>

namespace commun {
using std::vector;
    
struct registrar_params_setter: set_params_visitor<registrar_param_state> {
    using set_params_visitor::set_params_visitor;
    bool operator()(const token_param& p) { return set_param(p, &registrar_param_state::token); }
    bool operator()(const market_param& p) { return set_param(p, &registrar_param_state::market); }
    bool operator()(const checkwin_param& p) { return set_param(p, &registrar_param_state::checkwin); }
};
    
void registrar::validateprms(vector<registrar_param> params) {
    param_helper::check_params(params, _cfg.exists());
}

void registrar::setparams(vector<registrar_param> params) {
    require_auth(_self);
    auto setter = param_helper::set_parameters<registrar_params_setter>(params, _cfg, _self);
}

//this action mostly copypasted from cyber.domain.cpp
void registrar::checkwin() {
    require_auth(_self);
    const int64_t now = ::now();  // int64 to prevent overflows (can only happen if something broken)
    auto tnow = time_point_sec(now);
 
    auto state = state_singleton(_self, _self.value);
    bool exists = state.exists();
    auto s = exists ? state.get() : name_bid_state{tnow, tnow};
    
    if (exists) {
        eosio_assert(now >= s.last_checkwin.utc_seconds, "SYSTEM: last_checkwin is in future");  // must be impossible

        if ((now - s.last_win.utc_seconds) >= props().checkwin.min_time_from_last_win) {
            name_bid_tbl bids(_self, _self.value);
            auto idx = bids.get_index<"highbid"_n>();
            
            auto min_time_from_last_bid = props().checkwin.min_time_from_last_bid_start;
            size_t bid_num = 0;
            //it's added to prevent freezing of assignment of slightly less popular names during active bidding for leading names
            for (auto highest = idx.begin(); bid_num < props().checkwin.max_bid_checks && highest != idx.end(); highest++) {
                if (highest->high_bid > 0 && (now - highest->last_bid_time.utc_seconds) >= min_time_from_last_bid) {
                    s.last_win = tnow;
                    idx.modify(highest, name(), [&](auto& b) {
                        b.high_bid = -b.high_bid;
                    });
                    break;
                }
                min_time_from_last_bid += props().checkwin.min_time_from_last_bid_step;
                bid_num++;
            }
        }
        s.last_checkwin = tnow;
    }
    state.set(s, _self);

    print("schedule next\n");
    auto sender_id = s.last_checkwin.utc_seconds;
    transaction tx;
    tx.actions.emplace_back(action{permission_level(_self, config::active_name), _self, "checkwin"_n, std::tuple<>()});
    tx.delay_sec = props().checkwin.interval;
    tx.send(sender_id, _self);
}

void registrar::retire_fee(int64_t amount) {
    if (amount > 0) {
        INLINE_ACTION_SENDER(eosio::token, transfer)(config::token_name, {_self, config::active_name}, {
            _self,
            token::get_issuer(config::token_name, props().token.sym.code()),
            asset(amount, props().token.sym),
            config::retire_memo
        });
    }
}

void registrar::on_transfer(name from, name to, asset quantity, std::string memo) {
    if (_self != to)
        return;
    const size_t pref_size = config::bid_prefix.size();
    const size_t memo_size = memo.size();
    eosio_assert((memo_size > pref_size) && memo.substr(0, pref_size) == config::bid_prefix, "incorrect bid format");
    auto sym_code = symbol_code(memo.substr(pref_size).c_str());
    eosio_assert(!bancor::exist(config::bancor_name, sym_code), "bancor token already exists");
    eosio_assert(quantity.symbol == props().token.sym, "asset must be reserve token");
    eosio_assert(quantity.amount > 0, "insufficient bid"); //? fee
    
    name_bid_tbl bids(_self, _self.value);
    print(name{from}, " bid ", quantity, " on ", sym_code.to_string(), "\n"); 

    const auto set_bid = [&](auto& b) {
        b.high_bidder = from;
        b.high_bid = quantity.amount;
        b.last_bid_time = time_point_sec{::now()};
    };
    
    auto current = bids.find(sym_code.raw());
    if (current == bids.end()) {
        bids.emplace(from, [&](auto& b) {
            b.sym_code = sym_code;
            set_bid(b);
        });
    } else {
        eosio_assert(current->high_bid != 0, "SYSTEM: incorrect high bid");
        eosio_assert(current->high_bidder != from, "account is already highest bidder");
        if (current->high_bid > 0) {
            eosio_assert(quantity.amount - current->high_bid > (current->high_bid / props().market.bid_increment_denom), "insufficient bid");
            add_refund(from, current->high_bidder, current->high_bid, sym_code);
            bids.modify(current, from, set_bid);
        }
        else {
            name_ask_tbl asks(_self, _self.value);
            auto ask = asks.find(sym_code.raw());
            eosio_assert(ask != asks.end(), "this auction has already closed");
            eosio_assert(ask->price <= quantity.amount, "insufficient bid");
            asks.erase(ask);
            auto fee_amount = get_fee_amount(quantity.amount, props().market.sale_fee);
            add_refund(from, current->high_bidder, quantity.amount - fee_amount, sym_code);
            retire_fee(fee_amount);
            bids.modify(current, name(), [&](auto& r) { r.high_bidder = from; });
        }
    }
}

void registrar::add_refund(name payer, name bidder, int64_t amount, symbol_code sym_code) {
    if(amount == 0)
        return;
    name_bid_refund_tbl refunds_table(_self, sym_code.raw());
    print("refund to ", name{bidder}, " on ", sym_code.to_string(), ", amount = ", amount, "\n"); 
    auto it = refunds_table.find(bidder.value);
    if (it != refunds_table.end()) {
        refunds_table.modify(it, name(), [&](auto& r) {
            r.amount += asset(amount, props().token.sym);
        });
    } else {
        refunds_table.emplace(payer, [&](auto& r) {
            r.bidder = bidder;
            r.amount = asset(amount, props().token.sym);
        });
    }
    
    //can we just use special transfer w/o notifications in a future?
    transaction t;
    t.actions.emplace_back(permission_level{_self, config::active_name}, _self, "claimrefund"_n,
        std::make_tuple(bidder, sym_code));
    t.delay_sec = 0;
    uint128_t deferred_id = (uint128_t(sym_code.raw()) << 64) | bidder.value;
    cancel_deferred(deferred_id);
    t.send(deferred_id, _self);
    //should we actually create this transaction and pay for it. isn't it better to just wait for bidder's claiming?
    
}

void registrar::claimrefund(name bidder, symbol_code sym_code) {
    //require_auth(anyone);
    name_bid_refund_tbl refunds_table(_self, sym_code.raw());
    auto it = refunds_table.find(bidder.value);
    eosio_assert( it != refunds_table.end(), "refund not found" );
    eosio::token::get_balance(config::token_name, bidder, props().token.sym.code()); //commun dapp does not have to pay for ram if bidder has closed a balance
    
    INLINE_ACTION_SENDER(eosio::token, transfer)(config::token_name, {_self, config::active_name},
        {_self, bidder, asset(it->amount.amount, props().token.sym), std::string("refund bid on ") + sym_code.to_string()});

    refunds_table.erase(it);
}

void registrar::create(asset maximum_supply, int16_t cw, int16_t fee) {
    name_bid_tbl bids(_self, _self.value);
    auto sym_code = maximum_supply.symbol.code();
    auto current = bids.find(sym_code.raw());
    eosio_assert(current != bids.end(), "no active bid for token symbol");
    require_auth(current->high_bidder);
    eosio_assert(current->high_bid < 0, "auction is not closed yet");
    auto bid_amount = -current->high_bid;
    if (bancor::exist(config::bancor_name, sym_code)) { //exotic situation but not impossible
        print("bancor token already exists\n");
        //we have to refund, so no assert here. otherwise claimrefund will have more sophisticated logic
        add_refund(_self, current->high_bidder, bid_amount, sym_code);
        return;
    }
    
    INLINE_ACTION_SENDER(bancor, create) (config::bancor_name, {config::bancor_name, config::create_permission},
        {current->high_bidder, maximum_supply, cw, fee});
    
    auto fee_amount = get_fee_amount(bid_amount, props().market.bancor_creation_fee);
    asset reserve(bid_amount - fee_amount, props().token.sym);
    if (reserve.amount > 0) {
        INLINE_ACTION_SENDER(eosio::token, transfer)(config::token_name, {_self, config::active_name}, {
            _self,
            config::bancor_name,
            reserve,
            config::restock_prefix + sym_code.to_string()
        });
    }

    retire_fee(fee_amount);
    bids.erase(current);
}

void registrar::setprice(symbol_code sym_code, asset price) {
    name_bid_tbl bids(_self, _self.value);
    eosio_assert(price.is_valid(), "invalid price");
    eosio_assert(price.symbol == props().token.sym, "asset must be reserve token");
    auto current = bids.find(sym_code.raw());
    eosio_assert(current != bids.end(), "no active bid for token symbol");
    require_auth(current->high_bidder);
    eosio_assert(current->high_bid < 0, "auction is not closed yet");
    
    name_ask_tbl asks(_self, _self.value);
    auto ask = asks.find(sym_code.raw());
    
    if (ask != asks.end() && price.amount != 0)
        asks.modify(ask, name(), [&](auto& a) { a.price = price.amount; });
    else if (ask != asks.end() && price.amount == 0)
        asks.erase(ask);
    else if (ask == asks.end() && price.amount != 0)
        asks.emplace(current->high_bidder, [&](auto& a) { a.sym_code = sym_code; a.price = price.amount; });
    else
        eosio_assert(false, "invalid amount");
}

} // commun

DISPATCH_WITH_TRANSFER(commun::registrar, commun::config::token_name, on_transfer,
    (validateprms)(setparams)
    (checkwin)(claimrefund)(create)(setprice)
)
