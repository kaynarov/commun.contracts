#include "types.h"

namespace commun {

using namespace eosio;

class commun_social: public contract {
public:
    using contract::contract;

    [[eosio::action]] void pin(name pinner, name pinning);
    [[eosio::action]] void unpin(name pinner, name pinning);

    [[eosio::action]] void block(name blocker, name blocking);
    [[eosio::action]] void unblock(name blocker, name blocking);

    [[eosio::action]] void updatemeta(name account, accountmeta meta);
    [[eosio::action]] void deletemeta(name account);
};

}
