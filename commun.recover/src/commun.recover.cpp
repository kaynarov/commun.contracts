#include <eosio/permission.hpp>
#include <cyber.bios/cyber.bios.hpp>
#include <commun.recover/commun.recover.hpp>
#include <commun.point/commun.point.hpp>
#include <commun/config.hpp>

using namespace commun;
namespace cfg = commun::config;

structures::params_struct commun_recover::getParams() const {
    return tables::params_table(_self, _self.value).get_or_default();
}

void commun_recover::setParams(const structures::params_struct& params) {
    tables::params_table(_self, _self.value).set(params, _self);
}


#define SET_PARAM(STRUCTURE, PARAM) if (PARAM && (STRUCTURE.PARAM != *PARAM)) { STRUCTURE.PARAM = *PARAM; _empty = false; }
void commun_recover::setparams(std::optional<uint64_t> recover_delay) {
    require_auth(_self);

    auto params = getParams();
    
    bool _empty = true;
    SET_PARAM(params, recover_delay);

    eosio::check(!_empty, "No params changed");
    setParams(params);
}
#undef SET_PARAM

void commun_recover::recover(name account, std::optional<public_key> active_key, std::optional<public_key> owner_key) {
    require_auth(_self);

    bool change_active = active_key.has_value() && active_key.value() != public_key();
    bool change_owner  = owner_key.has_value() &&  owner_key.value() != public_key();
    eosio::check(change_active || change_owner, "Specify at least one key for recovery");

    auto packed_requested = pack(std::vector<permission_level>{{_self, cfg::code_name}}); 
    auto res = eosio::check_permission_authorization(account, cfg::owner_name,
            (const char*)0, 0, packed_requested.data(), packed_requested.size(), eosio::microseconds());
    eosio::check(res > 0, "Key recovery for this account is not available");

    const auto &params = getParams();

    if (change_active) {
        cyber::authority auth;
        auth.threshold = 1;
        auth.keys.push_back({active_key.value(), 1});

        action(
            permission_level{account, cfg::owner_name},
            "cyber"_n, "updateauth"_n,
            std::make_tuple(account, cfg::active_name, cfg::owner_name, auth)
        ).send();

        if (false == commun::point::get_global_lock_state(account, params.recover_delay)) {
            action(
                permission_level{account, cfg::owner_name},
                "c.point"_n, "globallock"_n,
                std::make_tuple(account, params.recover_delay)
            ).send();
        }
    }

    if (change_owner) {
        auto owner_request = tables::owner_request_table(_self, account.value);
        auto change_time = eosio::current_time_point() + eosio::seconds(params.recover_delay);
        owner_request.set({change_time, owner_key.value()}, _self);
    }
}

void commun_recover::applyowner(name account) {
    require_auth(account);

    auto owner_request = tables::owner_request_table(_self, account.value);
    eosio::check(owner_request.exists(), "Request for change owner key doesn't exists");

    auto request = owner_request.get();
    eosio::check(eosio::current_time_point() >= request.change_time, "Change owner too early");

    cyber::authority auth;
    auth.threshold = 1;
    auth.keys.push_back({request.owner_key, 1});
    auth.accounts.push_back({{_self, cfg::code_name}, 1});

    action(
        permission_level{account, cfg::owner_name},
        "cyber"_n, "updateauth"_n,
        std::make_tuple(account, cfg::owner_name, ""_n, auth)
    ).send();

    owner_request.remove();
}

void commun_recover::cancelowner(name account) {
    require_auth(account);

    auto owner_request = tables::owner_request_table(_self, account.value);
    eosio::check(owner_request.exists(), "Request for change owner key doesn't exists");
    owner_request.remove();
}
