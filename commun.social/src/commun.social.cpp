#include <eosio/event.hpp>
#include <commun.social.hpp>

using namespace commun;

void commun_social::pin(name pinner, name pinning) {
    require_auth(pinner);

    eosio::check(is_account(pinning), "Pinning account doesn't exist.");
    eosio::check(pinner != pinning, "You cannot pin yourself");
}

void commun_social::unpin(name pinner, name pinning) {
    require_auth(pinner);

    eosio::check(is_account(pinning), "Pinning account doesn't exist.");
    eosio::check(pinner != pinning, "You cannot unpin yourself");
}

void commun_social::block(name blocker, name blocking) {
    require_auth(blocker);

    eosio::check(is_account(blocking), "Blocking account doesn't exist.");
    eosio::check(blocker != blocking, "You cannot block yourself");
}

void commun_social::unblock(name blocker, name blocking) {
    require_auth(blocker);

    eosio::check(is_account(blocking), "Blocking account doesn't exist.");
    eosio::check(blocker != blocking, "You cannot unblock yourself");
}

void commun_social::updatemeta(name account, accountmeta meta) {
    require_auth(account);
}

void commun_social::deletemeta(name account) {
    require_auth(account);
}

EOSIO_DISPATCH(commun::commun_social, (pin)(unpin)(block)(unblock)(updatemeta)(deletemeta))