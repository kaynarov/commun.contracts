#pragma once
#include <eosio/eosio.hpp>
#include "types.h"
#include "config.hpp"
#include <eosio/time.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/crypto.hpp>

namespace commun { namespace structures {

using namespace eosio;

using counter_t = uint64_t;

struct mssgid {
    mssgid() = default;

    name author;
    std::string permlink;

    bool operator==(const mssgid& value) const {
        return author == value.author &&
               permlink == value.permlink;
    }

    EOSLIB_SERIALIZE(mssgid, (author)(permlink))
};

struct permlink {
    permlink() = default;

    uint64_t id;
    name parentacc;
    uint64_t parent_id;
    std::string value;
    uint16_t level;
    uint32_t childcount;

    uint64_t primary_key() const {
        return id;
    }

    std::string secondary_key() const {
        return value;
    }
};

} // structures

namespace tables {

using namespace eosio;

using permlink_id_index = indexed_by<N(primary), const_mem_fun<structures::permlink, uint64_t, &structures::permlink::primary_key>>;
using permlink_value_index = indexed_by<N(byvalue), const_mem_fun<structures::permlink, std::string, &structures::permlink::secondary_key>>;
using permlink_table = multi_index<N(permlink), structures::permlink, permlink_id_index, permlink_value_index>;

}


} // commun
