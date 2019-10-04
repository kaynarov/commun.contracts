#pragma once
#include <eosio/eosio.hpp>
#include <commun.publication/types.h>
#include <commun.publication/config.hpp>
#include <eosio/time.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/crypto.hpp>
#include <commun.gallery/commun.gallery.hpp>

namespace commun { 
using namespace eosio;

struct mssgid_t {
    mssgid_t() = default;

    name author;
    std::string permlink;

    bool operator==(const mssgid_t& value) const {
        return author == value.author &&
               permlink == value.permlink;
    }

    uint64_t tracery() const {
        auto hash = sha256(permlink.c_str(), permlink.size());
        return *(reinterpret_cast<const uint64_t *>(hash.extract_as_byte_array().data()));
    }
};

struct vertex_t {
    uint64_t id;
    name     creator;
    uint64_t tracery;
    name     parent_creator;
    uint64_t parent_tracery;
    uint16_t level;
    uint32_t childcount;

    uint64_t primary_key() const { return id; }

    using key_t = gallery_types::mosaic_key_t;
    key_t by_key()const { return std::make_tuple(creator, tracery); }
};

struct acc_param_t {
    name account;
    std::vector<name> providers;
    uint64_t primary_key() const { return account.value; }
};

using namespace eosio;

using vertex_id_index  = indexed_by<N(primary), const_mem_fun<vertex_t, uint64_t, &vertex_t::primary_key> >;
using vertex_key_index = indexed_by<N(bykey), const_mem_fun<vertex_t, vertex_t::key_t, &vertex_t::by_key> >;
using vertices = multi_index<N(vertex), vertex_t, vertex_id_index, vertex_key_index>;

using accparams = eosio::multi_index<"accparam"_n, acc_param_t>;

} // commun
