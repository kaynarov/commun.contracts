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

    /**
        \brief The pin action is used to add an account name to a pin-list.

        \param pinner account name which is the pin-list owner and adds the name specified in the pinning field to the pin-list
        \param pinning the account name that is being added to the pin-list

        Conditions for transaction execution:
            - \a pinning account exists;
            - \a pinner is not equal to \a pinning.

        Action do not stores any data in DB.

        Performing the action requires the \a pinner account signature and client signature.
    */
    [[eosio::action]] void pin(name pinner, name pinning);

    /**
        \brief The unpin action is used to remove an account name from a pin-list.

        \param pinner the account name that removes a name specified in the pinning field from the pin-list
        \param pinning the account name that is about to be removed from the pin-list

        Conditions for transaction execution:
            - \a pinning account exists;
            - \a pinner is not equal to \a pinning.

        Action do not stores any data in DB.

        Performing the action requires the \a pinner account signature and client signature.
    */
    [[eosio::action]] void unpin(name pinner, name pinning);

    /**
        \brief The block action is used to add an account name to "black" list.

        \param blocker the account name which is the "black" list owner and adds the name specified in the blocking field to the "black" list
        \param blocking the account name that is being added to the "black" list

        Conditions for transaction execution:
            - \a blocker account exists;
            - \a blocker is not equal to \a blocking.

        Action do not stores any data in DB.

        Performing the action requires the \a blocker account signature and client signature.
    */
    [[eosio::action]] void block(name blocker, name blocking);

    /**
        \brief The unblock action is used to remove an account name from "black" list.

        \param blocker the account name which is the "black" list owner and removes the name specified in the blocking field from the "black" list
        \param blocking the account name that is about to be removed from the "black" list

        Conditions for transaction execution:
            - \a blocker account exists;
            - \a blocker is not equal to \a blocking.

        Action do not stores any data in DB.

        Performing the action requires the \a blocker account signature and client signature.
    */
    [[eosio::action]] void unblock(name blocker, name blocking);

    /**
        \brief The updatemeta action is used to fill, update, or delete account profile field values.

        \param account name of the account whose profile is being edited
        \param meta a value of type of the \ref accountmeta structure

        Action do not stores any data in DB.

        Performing the action requires the \a account signature and client signature.
    */
    [[eosio::action]] void updatemeta(name account, accountmeta meta);

    /**
        \brief The deletemeta action is used for deleting an account profile.

        \param account is a name of the account whose profile is being deleted

        Action do not stores any data in DB.

        Performing the action requires the \a account signature and client signature.
    */
    [[eosio::action]] void deletemeta(name account);
};

}
