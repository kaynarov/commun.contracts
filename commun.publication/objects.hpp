#pragma once
#include <eosio/eosio.hpp>
#include "types.h"
#include "config.hpp"
#include <eosio/time.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/crypto.hpp>
#include <commun.gallery/commun.gallery.hpp>

namespace commun { namespace structures {

using namespace eosio;

struct mssgid {
    mssgid() = default;

    name author;
    std::string permlink;

    bool operator==(const mssgid& value) const {
        return author == value.author &&
               permlink == value.permlink;
    }
    
    uint64_t tracery() const {
        auto hash = sha256(permlink.c_str(), permlink.size());
        return *(reinterpret_cast<const uint64_t *>(&hash));
    }

    EOSLIB_SERIALIZE(mssgid, (author)(permlink))
};

struct vertex {
    uint64_t id;
    name     creator;
    uint64_t tracery;
    name     parent_creator;
    uint64_t parent_tracery;
    uint16_t level;
    uint32_t childcount;

    uint64_t primary_key() const { return id; }

    using key_t = gallery_base::mosaic_key_t;
    key_t by_key()const { return std::make_tuple(creator, tracery); }
};

} // structures


using namespace eosio;

using vertex_id_index  = indexed_by<N(primary), const_mem_fun<structures::vertex, uint64_t, &structures::vertex::primary_key> >;
using vertex_key_index = indexed_by<N(bykey), const_mem_fun<structures::vertex, structures::vertex::key_t, &structures::vertex::by_key> >;
using vertices = multi_index<N(vertex), structures::vertex, vertex_id_index, vertex_key_index>;

} // commun
