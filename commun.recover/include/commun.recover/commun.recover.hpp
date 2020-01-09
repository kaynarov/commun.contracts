#pragma once
#include "objects.hpp"

#include <eosio/crypto.hpp>

namespace commun {

using namespace eosio;
using std::optional;

/**
 * \brief This class implements the \a commun.recover contract functionality
 * \ingroup recover_class
 */
class commun_recover: public contract {
public:
    using contract::contract;

    [[eosio::action]] void recover(name account, std::optional<public_key> active_key, std::optional<public_key> owner_key);
    [[eosio::action]] void applyowner(name account);
    [[eosio::action]] void cancelowner(name account);
};

}
