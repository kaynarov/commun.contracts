#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/crypto.hpp>
#include "config.hpp"

namespace commun::structures {

using namespace eosio;
using std::optional;


//// DOCS_TABLE: dapp
struct [[eosio::table]] owner_request {
    time_point change_time;
    public_key owner_key;
};

}

namespace commun::tables {
    using namespace eosio;

    using owner_request_table = eosio::singleton<"ownerrequest"_n, structures::owner_request>;
}
