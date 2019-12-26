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
    ,   _symbol_code(sym.to_symbol_code())
    ,   _creators_added(false) {}

    void add_creators(std::vector<name> accs) {
        _creators_added = true;
        set_authority(_code, accs, commun::config::create_permission);
        _tester->link_authority(_code, _code, commun::config::create_permission, N(create));
    }

    symbol _symbol;
    symbol_code _symbol_code;
    bool _creators_added;

    //// point actions
    action_result create(account_name issuer, double initial_supply, double maximum_supply, int16_t cw, int16_t fee) {
        return create(issuer, make_asset(initial_supply), make_asset(maximum_supply), cw, fee);
    }
    action_result create(account_name issuer, asset initial_supply, asset maximum_supply, int16_t cw, int16_t fee) {
        if (!_creators_added)
            return push(N(create), _code, args()
                ("issuer", issuer)
                ("initial_supply", initial_supply)
                ("maximum_supply", maximum_supply)
                ("cw", cw)
                ("fee", fee)
            );

        return _tester->push_action_msig_tx(_code, N(create),
            {permission_level{_code, commun::config::create_permission}},
            {{_code}},
            args()
                ("issuer", issuer)
                ("initial_supply", initial_supply)
                ("maximum_supply", maximum_supply)
                ("cw", cw)
                ("fee", fee)
        );
    }

    action_result setparams(account_name issuer, mvo params) {
        return push(N(setparams), issuer, params("commun_code", _symbol.to_symbol_code()));
    }

    action_result setparams(account_name issuer, uint16_t transfer_fee, int64_t min_transfer_fee_points) {
        return setparams(issuer, args()
            ("transfer_fee", transfer_fee)
            ("min_transfer_fee_points", min_transfer_fee_points)
        );
    }

    action_result setfreezer(account_name freezer) {
        return push(N(setfreezer), _code, args()("freezer", freezer));
    }

    action_result issue(account_name to, double quantity, string memo="", account_name issuer={}) {
        return issue(to, make_asset(quantity), memo, issuer);
    }
    action_result issue(account_name to, asset quantity, string memo="", account_name issuer={}) {
        if (issuer.empty()) {
            issuer = get_issuer(quantity.get_symbol());
        }
        std::vector<action_data> tx_actions = {{_code, N(issue), {{_code, config::active_name}},
            args()("to", issuer)("quantity", quantity)("memo", memo)}};
        std::vector<account_name> signers = {_code};
        if (issuer != to) {
            tx_actions.push_back({_code, N(transfer), {{issuer, config::active_name}},
                args()("from", issuer)("to", to)("quantity", quantity)("memo", memo)});
            signers.push_back(issuer);
        }
        return _tester->push_tx(tx_actions, signers, false /*produce_and_check*/);
    }

    action_result retire(account_name from, double quantity, string memo="") {
        return retire(from, make_asset(quantity), memo);
    }
    action_result retire(account_name from, asset quantity, string memo="") {
        return push(N(retire), from, args()
            ("from", from)
            ("quantity", quantity)
            ("memo", memo)
        );
    }

    action_result open(account_name owner, account_name payer = {}) {
        return open(owner, _symbol_code, payer);
    }

    action_result close(account_name owner) {
        return close(owner, _symbol_code);
    }

    action_result open(account_name owner, symbol_code commun_code, account_name payer = {}) {
        auto a = args()("owner", owner)("commun_code", commun_code);

        if (payer.good()) {
            a("ram_payer", payer);
        }
        return push(N(open), payer ? payer : owner, a);
    }

    action_result close(account_name owner, symbol_code commun_code) {
        return push(N(close), owner, args()
            ("owner", owner)
            ("commun_code", commun_code)
        );
    }

    action_result transfer(account_name from, account_name to, double quantity, string memo = "") {
        return transfer(from, to, make_asset(quantity), memo);
    }
    action_result transfer(account_name from, account_name to, asset quantity, string memo = "") {
        return push(N(transfer), from, args()
            ("from", from)
            ("to", to)
            ("quantity", quantity)
            ("memo", memo)
        );
    }

    action_result withdraw(account_name owner, asset quantity) {
        return push(N(withdraw), owner, args()
            ("owner", owner)
            ("quantity", quantity)
        );
    }

    // safe
    action_result enable_safe(name owner, double unlock, uint32_t delay, name trusted = {}) {
        return enable_safe(owner, make_asset(unlock), delay, trusted);
    }
    action_result enable_safe(name owner, asset unlock, uint32_t delay, name trusted = {}) {
        return push(N(enablesafe), owner, args()
            ("owner", owner)
            ("unlock", unlock)
            ("delay", delay)
            ("trusted", trusted)
        );
    }

    action_result _disable_safe(name owner, name mod_id, signers_t signers) {
        return push_msig(N(disablesafe), signers, args()
            ("owner", owner)
            ("commun_code", _symbol_code)
            ("mod_id", mod_id)
        );
    }
    action_result disable_safe(name owner, name mod_id) {
        return _disable_safe(owner, mod_id, {owner});
    }
    action_result disable_safe2(name owner, name signer, name force_mod_id=0) {
        return _disable_safe(owner, force_mod_id, {owner, signer});
    }

    action_result _unlock_safe(name owner, asset unlock, name mod_id, signers_t signers) {
        return push_msig(N(unlocksafe), signers, args()
            ("owner", owner)
            ("unlock", unlock)
            ("mod_id", mod_id)
        );
    }
    action_result unlock_safe(name owner, name mod_id, double unlock) {
        return unlock_safe(owner, mod_id, make_asset(unlock));
    }
    action_result unlock_safe(name owner, name mod_id, asset unlock) {
        return _unlock_safe(owner, unlock, mod_id, {owner});
    }
    action_result unlock_safe2(name owner, double unlock, name signer, name force_mod_id=0) {
        return unlock_safe2(owner, make_asset(unlock), signer, force_mod_id);
    }
    action_result unlock_safe2(name owner, asset unlock, name signer, name force_mod_id=0) {
        return _unlock_safe(owner, unlock, force_mod_id, {owner, signer});
    }

    action_result lock_safe(name owner, double lock = 0) {
        return lock_safe(owner, make_asset(lock));
    }
    action_result lock_safe(name owner, asset lock) {
        return push(N(locksafe), owner, args()
            ("owner", owner)
            ("lock", lock)
        );
    }

    action_result _modify_safe(
        name owner, optional<uint32_t> delay, optional<name> trusted, name mod_id, signers_t signers
    ) {
        return push_msig(N(modifysafe), signers, args()
            ("owner", owner)
            ("commun_code", _symbol_code)
            ("mod_id", mod_id)
            ("delay", delay)
            ("trusted", trusted)
        );
    }
    action_result modify_safe(name owner, name mod_id, optional<uint32_t> delay = {}, optional<name> trusted = {}) {
        return _modify_safe(owner, delay, trusted, mod_id, {owner});
    }
    action_result modify_safe2(
        name owner, optional<uint32_t> delay, optional<name> trusted, name signer, name force_mod_id=0
    ) {
        return _modify_safe(owner, delay, trusted, force_mod_id, {owner, signer});
    }

    action_result _apply_safe_mod(name owner, name mod_id, signers_t signers) {
        return push_msig(N(applysafemod), signers, args()
            ("owner", owner)
            ("mod_id", mod_id)
        );
    }
    action_result apply_safe_mod(name owner, name mod_id) {
        return _apply_safe_mod(owner, mod_id, {owner});
    }
    action_result apply_safe_mod2(name owner, name mod_id, name signer) {
        return _apply_safe_mod(owner, mod_id, {owner, signer});
    }

    action_result cancel_safe_mod(name owner, name mod_id) {
        return push(N(cancelsafemod), owner, args()
            ("owner", owner)
            ("mod_id", mod_id)
        );
    }

    action_result global_lock(name owner, uint32_t period) {
        return push(N(globallock), owner, args()
            ("owner", owner)
            ("period", period)
        );
    }

    //// point tables
    variant get_params() {
        return get_params(_symbol);
    }
    variant get_params(symbol sym) {
        auto sname = sym.to_symbol_code().value;
        auto v = get_struct(_code, N(param), sname, "");
        if (v.is_object()) {
            auto o = mvo(v);
            o["max_supply"] = o["max_supply"].as<asset>().to_string();
            v = o;
        }
        return v;
    }

    account_name get_issuer(symbol sym) {
        auto sname = sym.to_symbol_code().value;
        auto v = get_struct(_code, N(param), sname, "");
        if (v.is_object()) {
            return mvo(v)["issuer"].as<account_name>();
        }
        return name();
    }

    variant get_stats() {
        auto sname = _symbol_code.value;
        auto v = get_struct(sname, N(stat), sname, "");
        if (v.is_object()) {
            auto o = mvo(v);
            o["supply"] = o["supply"].as<asset>().to_string();
            o["reserve"] = o["reserve"].as<asset>().to_string();
            v = o;
        }
        return v;
    }

    int64_t get_supply() {
        auto sname = _symbol_code.value;
        auto v = get_struct(sname, N(stat), sname, "");
        if (v.is_object()) {
            auto o = mvo(v);
            return o["supply"].as<asset>().get_amount();
        }
        return 0;
    }

    int64_t get_amount(account_name acc) {
        auto v = get_struct(acc, N(accounts), _symbol_code.value, "");
        if (v.is_object()) {
            auto o = mvo(v);
            return o["balance"].as<asset>().get_amount();
        }
        return 0;
    }

    variant get_account(account_name acc) {
        auto v = get_struct(acc, N(accounts), _symbol_code.value, "");
        if (v.is_object()) {
            auto o = mvo(v);
            o["balance"] = o["balance"].as<asset>().to_string();
            return o;
        }
        return v;
    }

    std::vector<variant> get_accounts(account_name user) {
        return _tester->get_all_chaindb_rows(_code, user, N(accounts), false);
    }

    variant get_global_params() {
        return get_singleton(_code, N(globalparam), "");
    }

    variant get_safe(name acc) {
        auto v = get_struct(acc, N(safe), _symbol_code.value, "");
        if (v.is_object() && v["unlocked"].is_object()) {
            auto o = mvo(v);
            return o("unlocked", o["unlocked"].as<asset>());
        }
        return v;
    }

    variant get_safe_mod(name acc, name mod_id) {
        return get_struct(acc, N(safemod), mod_id.value, "");
    }

    variant get_global_lock(name acc) {
        return get_singleton(acc, N(lock), "");
    }

    // generated objects
    variant make_safe(double unlocked, uint32_t delay, name trusted = {}) {
        return mvo()("unlocked", make_asset(unlocked).to_string())("delay", delay)("trusted", trusted);
    }
    variant make_safe_mod(name id, double unlock, optional<uint32_t> delay = {}, optional<name> trusted = {}) {
        auto mod = mvo()("id", id)("commun_code", _symbol_code)("unlock", to_shares(unlock));
        if (delay) mod = mod("delay", *delay);
        if (trusted) mod = mod("trusted", *trusted);
        // date
        return mod;
    }

    //// helpers
    string restock_memo() const {
        return commun::config::restock_prefix + name();
    }
    string name() const {
        return _symbol.name();
    }
    double satoshi() const {
        return 1. / _symbol.precision();
    }
    asset satoshi_asset() {
        return make_asset(satoshi());
    }
    int64_t to_shares(double x) const {
        return x * _symbol.precision() + .5 * (x < 0 ? 0 : 1);
    }
    asset make_asset(double x = 0) const {
        return asset(to_shares(x), _symbol);
    }
    string asset_str(double x = 0) {
        return make_asset(x).to_string();
    }
    symbol bad_sym(int diff = 1) const {
        return symbol{static_cast<uint8_t>(_symbol.decimals() + diff), name().c_str()};
    }

};


}} // eosio::testing
