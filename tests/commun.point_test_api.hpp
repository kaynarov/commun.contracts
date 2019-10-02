#pragma once
#include "test_api_helper.hpp"
#include <commun.point/include/commun.point/config.hpp>

namespace eosio { namespace testing {


struct commun_point_api: base_contract_api {
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
    commun_point_api(golos_tester* tester, name code, symbol sym)
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
    action_result create(account_name issuer, asset maximum_supply, int16_t cw, int16_t fee) {
        if (!_creators_added)
            return push(N(create), commun::config::dapp_name, args()
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

    action_result setfreezer(account_name freezer) {
        return push(N(setfreezer), _code, args()("freezer", freezer));
    }

    action_result issue(account_name issuer, account_name to, asset quantity, string memo) {
        return push(N(issue), issuer, args()
            ("to", to)
            ("quantity", quantity)
            ("memo", memo)
        );
    }

    action_result retire(account_name issuer, asset quantity, string memo) {
        return push(N(retire), issuer, args()
            ("quantity", quantity)
            ("memo", memo)
        );
    }

    action_result open(account_name owner, symbol_code commun_code = symbol_code(), account_name payer = account_name()) {
        auto a = args()("owner", owner)("commun_code", commun_code);

        if (payer) {
            a("ram_payer", payer);
        }
        return push(N(open), payer ? payer : owner, a);
    }

    action_result close(account_name owner, symbol_code commun_code = symbol_code()) {
        return push(N(close), owner, args()
            ("owner", owner)
            ("commun_code", commun_code)
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
    variant get_params() {
        return get_params(_symbol);
    }
    variant get_params(symbol sym) {
        auto sname = sym.to_symbol_code().value;
        auto v = get_struct(_code, N(param), sname, "param");
        if (v.is_object()) {
            auto o = mvo(v);
            o["max_supply"] = o["max_supply"].as<asset>().to_string();
            v = o;
        }
        return v;
    }

    variant get_stats() {
        auto sname = _symbol.to_symbol_code().value;
        auto v = get_struct(sname, N(stat), sname, "currency_stats");
        if (v.is_object()) {
            auto o = mvo(v);
            o["supply"] = o["supply"].as<asset>().to_string();
            o["reserve"] = o["reserve"].as<asset>().to_string();
            v = o;
        }
        return v;
    }

    int64_t get_supply() {
        auto sname = _symbol.to_symbol_code().value;
        auto v = get_struct(sname, N(stat), sname, "currency_stats");
        if (v.is_object()) {
            auto o = mvo(v);
            return o["supply"].as<asset>().get_amount();
        }
        return 0;
    }

    int64_t get_amount(account_name acc) {
        auto v = get_struct(acc, N(accounts), _symbol.to_symbol_code().value, "account");
        if (v.is_object()) {
            auto o = mvo(v);
            return o["balance"].as<asset>().get_amount();
        }
        return 0;
    }

    std::vector<variant> get_accounts(account_name user) {
        return _tester->get_all_chaindb_rows(_code, user, N(accounts), false);
    }

    variant get_global_params() {
        return _tester->get_chaindb_singleton(_code, _code, N(globalparam), "");
    }
};


}} // eosio::testing
