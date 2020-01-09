#include <eosio/permission.hpp>
#include <cyber.bios/cyber.bios.hpp>
#include <commun.recover/commun.recover.hpp>
#include <commun/config.hpp>

using namespace commun;
namespace cfg = commun::config;

void commun_recover::recover(name account, std::optional<public_key> active_key, std::optional<public_key> owner_key) {
    require_auth(_self);

    bool change_active = active_key.has_value() && active_key.value() != public_key();
    bool change_owner  = owner_key.has_value() &&  owner_key.value() != public_key();
    eosio::check(change_active || change_owner, "Specify at least one key for recovery");

    auto packed_requested = pack(std::vector<permission_level>{{_self, cfg::code_name}}); 
    auto res = eosio::check_permission_authorization(account, cfg::owner_name,
            (const char*)0, 0, packed_requested.data(), packed_requested.size(), eosio::microseconds());
    eosio::check(res > 0, "Key recovery for this account is not available");

    if (change_active) {
        cyber::authority auth;
        auth.threshold = 1;
        auth.keys.push_back({active_key.value(), 1});

        action(
            permission_level{account, cfg::owner_name},
            "cyber"_n, "updateauth"_n,
            std::make_tuple(account, cfg::active_name, cfg::owner_name, auth)
        ).send();
    }

    if (change_owner) {
        auto owner_request = tables::owner_request_table(_self, account.value);
        auto change_time = eosio::current_time_point() + eosio::seconds(config::def_recover_delay);
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

EOSIO_DISPATCH(commun::commun_recover, (recover)(applyowner)(cancelowner))
