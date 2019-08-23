#pragma once
#include <eosio/eosio.hpp>
#include "types.h"
#include "config.hpp"
#include <common/calclib/atmsp_storable.h>
#include <eosio/time.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/crypto.hpp>

namespace golos { namespace structures {

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

struct message {
    message() = default;

    name author;
    uint64_t id;
    uint64_t date;

    uint64_t primary_key() const {
        return id;
    }
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

struct voteinfo {
    voteinfo() = default;

    uint64_t id;
    uint64_t message_id;
    name voter;
    int16_t weight;
    uint64_t time;
    uint8_t count;

    uint64_t primary_key() const {
        return id;
    }

    uint64_t by_message() const {
        return message_id;
    }
};

struct vote_event {
    name voter;
    name author;
    std::string permlink;
    int16_t weight;
};


} // structures

namespace tables {

using namespace eosio;

using id_index = indexed_by<N(primary), const_mem_fun<structures::message, uint64_t, &structures::message::primary_key>>;
using message_table = multi_index<N(message), structures::message, id_index>;

using permlink_id_index = indexed_by<N(primary), const_mem_fun<structures::permlink, uint64_t, &structures::permlink::primary_key>>;
using permlink_value_index = indexed_by<N(byvalue), const_mem_fun<structures::permlink, std::string, &structures::permlink::secondary_key>>;
using permlink_table = multi_index<N(permlink), structures::permlink, permlink_id_index, permlink_value_index>;

using vote_id_index = indexed_by<N(id), const_mem_fun<structures::voteinfo, uint64_t, &structures::voteinfo::primary_key>>;
using vote_messageid_index = indexed_by<N(messageid), eosio::composite_key<structures::voteinfo,
      eosio::member<structures::voteinfo, uint64_t, &structures::voteinfo::message_id>>>;
using vote_group_index = indexed_by<N(byvoter), eosio::composite_key<structures::voteinfo,
      eosio::member<structures::voteinfo, uint64_t, &structures::voteinfo::message_id>,
      eosio::member<structures::voteinfo, name, &structures::voteinfo::voter>>>;
using vote_table = multi_index<N(vote), structures::voteinfo, vote_id_index, vote_messageid_index, vote_group_index>;

}


} // golos
