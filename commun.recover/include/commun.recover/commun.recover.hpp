#pragma once
#include "objects.hpp"

#include <eosio/crypto.hpp>
#include <commun/dispatchers.hpp>

namespace commun {

using namespace eosio;
using std::optional;

/**
 * \brief This class implements the \a c.recover contract functionality.
 * \ingroup recover_class
 */
class
/// @cond
[[eosio::contract("commun.recover")]]
/// @endcond
commun_recover: public contract {
public:
    using contract::contract;

    /**
    \brief The \ref setparams action is used for setting global contract parameters.

    \param recover_delay delay (in seconds) after which a made request can be executed to change the owner key

    \note
    At least one of the arguments must be non-empty and change the parameter value.

    \signreq
        — <i>majority of Commun dApp leaders</i> .
    */
    [[eosio::action]] void setparams(std::optional<uint64_t> recover_delay);

    /**
    \brief The \ref recover action is used to change the active- and owner-authorities for specified account.

    \param account account for which new keys should be applied
    \param active_key specifies new value for the key in active-authority (optional parameter)
    \param owner_key specifies new value for the key in owner-authority (optional parameter)

    This action recovers both the active- and owner-authorities. Active key recovery is performed immediately.

    To change the owner key, a user should make a request using the \ref recover action with non-empty \a owner_key parameter. This request will be saved in the \ref commun::structures::owner_request table and can be executed using \ref applyowner action with a delay specified by the \a recover_delay parameter. This request can be canceled by the \ref cancelowner action at any time.

    When a user makes two requests to recover the owner key, the last request cancels the previous one.

    \note
    At least one of the two parameters either \a active_key or \a owner_key must not be empty.

    This function can be applied only to those accounts that allow the contract to recovery keys. To do this, the account should allow the contract to satisfy the owner’s authority by adding <i>c.recover@cyber.code</i> actor.

    The contract sets active-authority as follows:
    
    \code{.json}
    {
        "threshold": 1,
        "keys": [{"key": active_key, "weight": 1}],
        "accounts": [],
        "waits": []
    }
    \endcode

    and owner-authority as follows:
    
    \code{.json}
    {
        "threshold": 1,
        "keys": [{"key": owner_key, "weight": 1}],
        "accounts": [{"permission": {"actor": "c.recover", "permission": "cyber@code"}, "weight": 1}],
        "waits": []
    }
    \endcode

     \signreq
        — <i>trusted community client</i>.
    */
    [[eosio::action]] void recover(name account, std::optional<public_key> active_key, std::optional<public_key> owner_key);

    /**
    \brief The \ref applyowner action applies new owner key specified through the \ref recover action.

    \param account account for which the owner key change request is applied

    An owner's key change request is created by calling the \ref recover action with non-empty \a owner_key argument. This action is performed with a delay specified by the \a recover_delay parameter (this parameter value is fixed at the time the request is created).

    \signreq
        — <i>account</i>
    */
    [[eosio::action]] void applyowner(name account);

    /**
    \brief The \ref cancelowner action cancels previously created request to change owner-authority.

    \param account account for which the owner key change request is removed

    An owner's key change request is created by calling the \ref recover action with a non-empty
    \a owner_key argument. This action can be performed at any time after creating the request.

    \signreq
        — <i>account</i>
    */
    [[eosio::action]] void cancelowner(name account);

private:
    structures::params_struct getParams() const;
    void setParams(const structures::params_struct& params);
};

}
