#include "types.h"
#include <commun/dispatchers.hpp>

namespace commun {

using namespace eosio;

/**
 * \brief This class implements the \a c.social contract behaviour.
 * \ingroup social_class
 */
class
/// @cond
[[eosio::contract("commun.social")]]
/// @endcond
commun_social: public contract {
public:
    using contract::contract;

    /**
        \brief The \ref pin action is used to add an account name of interest to a pin-list.

        \param pinner account name who owns the pin-list and adds the \a pinning name to it
        \param pinning account name to be added to the pin-list

        The action does not allow:
            - to add nonexistent \a pinning account to pin-list;
            - to add own name to pin-list. This means the \a pinner name must not match the \a pinning one.

        The action does not store any data in DB.

        \signreq
            — the \a pinner account ;  
            — <i>trusted community client</i> .
    */
    [[eosio::action]] void pin(name pinner, name pinning);

    /**
        \brief The \ref unpin action is used to remove an account name from a pin-list.

        \param pinner the account name who owns the pin-list and removes the \a pinning name from it
        \param pinning the account name to be removed from the pin-list

        The action does not allow:
            - to remove nonexistent \a pinning account from pin-list;
            - to remove own name from pin-list. This means the \a pinner name must not match the \a pinning one.

        The action does not store any data in DB.

        \signreq
            — the \a pinner account ;  
            — <i>trusted community client</i> .
    */
    [[eosio::action]] void unpin(name pinner, name pinning);

    /**
        \brief The \ref block action is used to add a name of unwanted account to «black» list.

        \param blocker account name who owns the «black» list and adds the \a blocking name to it
        \param blocking account name to be added to the «black» list

        The action does not allow:
            - to add nonexistent \a blocking account to «black» list;
            - to add own name to «black» list. This means the \a blocker name must not match the \a blocking one.

        The action does not store any data in DB.

        \signreq
            — the \a blocker account ;  
            — <i>trusted community client</i> .
    */
    [[eosio::action]] void block(name blocker, name blocking);

    /**
        \brief The \ref unblock action is used to remove an account name from «black» list.

        \param blocker account name who owns the «black» list and removes the \a blocking name from it
        \param blocking account name to be removed from the «black» list

        The action does not allow:
            - to remove nonexistent \a blocking account from «black» list;
            - to remove own name from «black» list. This means the \a blocker name must not match the \a blocking one.

        The action does not store any data in DB.

        \signreq
            — the \a blocker account ;  
            — <i>trusted community client</i> .
    */
    [[eosio::action]] void unblock(name blocker, name blocking);

    /**
        \brief The \ref updatemeta action is used to fill, update, or delete account profile field values.

        \param account the account whose profile is being edited
        \param meta account profile (the accountmeta) to be edited

        The action does not store any data in DB.

        \signreq
            — <i>the account</i> ;  
            — <i>trusted community client</i> .
    */
    [[eosio::action]] void updatemeta(name account, accountmeta meta);

    /**
        \brief The \ref deletemeta action is used for deleting an account profile.

        \param account the account whose profile is being deleted

        The action does not store any data in DB.

        \signreq
            — <i>the account</i> ;  
            — <i>trusted community client</i> .
    */
    [[eosio::action]] void deletemeta(name account);
};

}
