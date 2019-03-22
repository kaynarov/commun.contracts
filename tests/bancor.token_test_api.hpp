#pragma once
#include "test_api_helper.hpp"
#include "../common/config.hpp"
#include "../bancor.token/include/bancor.token/config.hpp"

namespace eosio { namespace testing {


struct bancor_token_api: base_contract_api {
private:
    void set_authority(name from, std::vector<name> to, name perm) {
        authority auth(1, {});
        for (auto acc : to) {
            auth.accounts.emplace_back(permission_level_weight{.permission = {acc, N(cyber.code)}, .weight = 1});
        }
        if (std::find(to.begin(), to.end(), from) == to.end()) {
            auth.accounts.emplace_back(permission_level_weight{.permission = {from, N(cyber.code)}, .weight = 1});
        }
        auth.accounts.emplace_back(permission_level_weight{.permission = {from, config::active_name}, .weight = 1});
        std::sort(auth.accounts.begin(), auth.accounts.end(),
            [](const permission_level_weight& l, const permission_level_weight& r) {
                return std::tie(l.permission.actor, l.permission.permission) <
                    std::tie(r.permission.actor, r.permission.permission);
            });
        _tester->set_authority(from, perm, auth, "owner");
    }
public:
    bancor_token_api(golos_tester* tester, name code, symbol sym)
    :   base_contract_api(tester, code)
    ,   _symbol(sym)
    ,   _creators_added(false){}
    
    void add_creators(std::vector<name> accs) {
        _creators_added = true;
        set_authority(_code, accs, commun::config::create_permission);
        _tester->link_authority(_code, _code, commun::config::create_permission, N(create));
    }
    
    symbol _symbol;
    bool _creators_added;
    
    //// token actions
    action_result create(account_name issuer, asset maximum_supply, int16_t cw, int16_t fee, std::vector<name> invoicers = {}) {
        if (!invoicers.empty())
            set_authority(issuer, invoicers, commun::config::invoice_name);
        
        if(!_creators_added)
            return push(N(create), _code, args()
                ("issuer", issuer)
                ("maximum_supply", maximum_supply)
                ("cw", cw)
                ("fee", fee)
            );
            
        return _tester->push_action_msig_tx(_code, N(create), 
            {permission_level{_code, commun::config::create_permission}}, 
            {{_code}}, 
            args()
                ("issuer", issuer)
                ("maximum_supply", maximum_supply)
                ("cw", cw)
                ("fee", fee)
        );
    }

    action_result issue(account_name issuer, account_name to, asset quantity, string memo) {
        return push(N(issue), issuer, args()
            ("to", to)
            ("quantity", quantity)
            ("memo", memo)
        );
    }

    action_result open(account_name owner, symbol symbol, account_name payer) {
        return push(N(open), owner, args()
            ("owner", owner)
            ("symbol", symbol)
            ("ram_payer", payer)
        );
    }

    action_result transfer(account_name from, account_name to, asset quantity, string memo = "") {
        return push(N(transfer), from, args()
            ("from", from)
            ("to", to)
            ("quantity", quantity)
            ("memo", memo)
        );
    }

    //// token tables
    variant get_stats() {
        auto sname = _symbol.to_symbol_code().value;
        auto v = get_struct(sname, N(stat), sname, "currency_stats");
        if (v.is_object()) {
            auto o = mvo(v);
            o["supply"] = o["supply"].as<asset>().to_string();
            o["max_supply"] = o["max_supply"].as<asset>().to_string();
            o["reserve"] = o["reserve"].as<asset>().to_string();
            v = o;
        }
        return v;
    }

    variant get_account(account_name acc) {
        auto v = get_struct(acc, N(accounts), _symbol.to_symbol_code().value, "account");
        if (v.is_object()) {
            auto o = mvo(v);
            o["balance"] = o["balance"].as<asset>().to_string();
            v = o;
        }
        return v;
    }

    std::vector<variant> get_accounts(account_name user) {
        return _tester->get_all_chaindb_rows(_code, user, N(accounts), false);
    }
};


}} // eosio::testing
