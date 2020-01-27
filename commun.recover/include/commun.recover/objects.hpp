#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/crypto.hpp>
#include "config.hpp"

namespace commun::structures {

using namespace eosio;
using std::optional;


/**
    \brief DB record containing information about user requests for change owner authority.
    \ingroup recover_tables
*/
// DOCS_TABLE: owner_request
struct [[eosio::table]] owner_request {
    time_point change_time;        //!< time when it will be permissible to change the owner key
    public_key owner_key;          //!< new owner key
};

/**
    \brief DB record containing global parameters for contract.
    \ingroup recover_tables
*/
// DOCS_TABLE: params
struct [[eosio::table]] params {
    uint64_t recover_delay = config::def_recover_delay;  //!< delay (in seconds) for setting the owner key in specified value
};

}

namespace commun::tables {
    using namespace eosio;

    using owner_request_table = eosio::singleton<"ownerrequest"_n, structures::owner_request>;

    using params_table = eosio::singleton<"params"_n, structures::params>;
}
