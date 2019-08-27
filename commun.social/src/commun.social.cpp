#include <eosio/event.hpp>
#include <commun.social.hpp>

using namespace commun;

void commun_social::pin(name pinner, name pinning) {
    require_auth(pinner);

    check(is_account(pinning), "Pinning account doesn't exist.");
    check(pinner != pinning, "You cannot pin yourself");

    tables::pinblock_table table(_self, pinner.value);
    auto itr = table.find(pinning.value);
    bool item_exists = (itr != table.end());

    if (item_exists) {
        check(!itr->blocking, "You have blocked this account. Unblock it before pinning");
        check(!itr->pinning, "You already have pinned this account");

        table.modify(itr, name(), [&](auto& item){
            item.pinning = true;
        });

    } else {
        table.emplace(pinner, [&](auto& item){
            item.account = pinning;
            item.pinning = true;
        });
    }
}

void commun_social::unpin(name pinner, name pinning) {
    require_auth(pinner);

    check(pinner != pinning, "You cannot unpin yourself");

    tables::pinblock_table table(_self, pinner.value);
    auto itr = table.find(pinning.value);
    bool item_exists = (itr != table.end());

    check(item_exists && itr->pinning, "You have not pinned this account");

    table.modify(itr, name(), [&](auto& item){
        item.pinning = false;
    });

    if (record_is_empty(*itr))
        table.erase(itr);
}

bool commun_social::record_is_empty(structures::pinblock_record record) {
    return !record.pinning && !record.blocking;
}

void commun_social::block(name blocker, name blocking) {
    require_auth(blocker);

    check(is_account(blocking), "Blocking account doesn't exist.");
    check(blocker != blocking, "You cannot block yourself");

    tables::pinblock_table table(_self, blocker.value);
    auto itr = table.find(blocking.value);
    bool item_exists = (itr != table.end());

    if (item_exists) {
        check(!itr->blocking, "You already have blocked this account");

        table.modify(itr, name(), [&](auto& item){
            item.pinning = false;
            item.blocking = true;
        });

    } else {
        table.emplace(blocker, [&](auto& item){
            item.account = blocking;
            item.blocking = true;
        });
    }
}

void commun_social::unblock(name blocker, name blocking) {
    require_auth(blocker);

    check(blocker != blocking, "You cannot unblock yourself");

    tables::pinblock_table table(_self, blocker.value);
    auto itr = table.find(blocking.value);
    bool item_exists = (itr != table.end());

    check(item_exists && itr->blocking, "You have not blocked this account");

    table.modify(itr, name(), [&](auto& item){
        item.blocking = false;
    });

    if (record_is_empty(*itr))
        table.erase(itr);
}

void commun_social::updatemeta(std::string avatar_url, std::string cover_url, 
                               std::string biography, std::string facebook, 
                               std::string telegram, std::string whatsapp, 
                               std::string wechat) {
    require_auth(_self);
}

void commun_social::deletemeta(name account) {
    require_auth(account);
}

EOSIO_DISPATCH(commun::commun_social, (pin)(unpin)(block)(unblock)(updatemeta)(deletemeta))