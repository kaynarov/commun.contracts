#pragma once

#include <common/config.hpp>

#ifdef UNIT_TEST_ENV
#   define ABI_TABLE
#   include <eosio/chain/types.hpp>
#   include "../golos.charge/types.h"
namespace eosio { namespace testing {
    using namespace eosio::chain;
#else
#   include <golos.charge/types.h>
#   define ABI_TABLE [[eosio::table]]
#   include <eosio/eosio.hpp>
    using namespace eosio;
#endif

struct forumprops {
    forumprops() = default;

    name social_contract = name();
};

#ifdef UNIT_TEST_ENV
}} // eosio::testing
FC_REFLECT(eosio::testing::forumprops, (social_contract))
#endif

