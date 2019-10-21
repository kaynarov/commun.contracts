#include "types.h"

namespace commun {

using namespace eosio;

/**
 * \brief This class implements comn.social contract behaviour
 * \ingroup social_class
 */
class commun_social: public contract {
public:
    using contract::contract;

    [[eosio::action]] void pin(name pinner, name pinning);
    [[eosio::action]] void unpin(name pinner, name pinning);

    /**
     * \brief action is used by user to block other users
     *
     * \param blocker account of blocker.
     * \param blocking blocking account.
     */
    [[eosio::action]] void block(name blocker, name blocking);
    [[eosio::action]] void unblock(name blocker, name blocking);

    [[eosio::action]] void updatemeta(name account, accountmeta meta);
    [[eosio::action]] void deletemeta(name account);
};

}
