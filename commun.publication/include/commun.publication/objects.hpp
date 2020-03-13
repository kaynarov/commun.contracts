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

struct mssgid {
    mssgid() = default;

    name author;
    std::string permlink;

    bool operator==(const mssgid& value) const {
        return author == value.author &&
               permlink == value.permlink;
    }

    // for set
    friend bool operator<(const mssgid& a, const mssgid& b) {
        return std::tie(a.author, a.permlink) < std::tie(b.author, b.permlink);
    }
    friend bool operator>(const mssgid& a, const mssgid& b) {
        return std::tie(a.author, a.permlink) > std::tie(b.author, b.permlink);
    }

    uint64_t tracery() const {
        std::string key = author.to_string() + "/" + permlink;
        auto hash = sha256(key.c_str(), key.size());
        return *(reinterpret_cast<const uint64_t *>(hash.extract_as_byte_array().data()));
    }
};

/**
 * \brief The structure represents a vertex table in DB.
 * \ingroup gallery_tables
 *
 * The table contains information about hierarchy of messages (post and comments).
 */
struct vertex_struct {
    uint64_t tracery;  //!< Message mosaic's tracery using as the primary key
    uint64_t parent_tracery;  //!< Mosaic's tracery of the parent message
    uint16_t level; //!< Nesting level of the message(a post is assigned the zero level, comments are assigned levels from one onwards)
    uint32_t childcount; //!< Count of comments of the same level under the message

    uint64_t primary_key() const { return tracery; }
};

struct acc_param {
    name account;
    std::vector<name> providers;
    uint64_t primary_key() const { return account.value; }
};

using namespace eosio;

using vertices [[using eosio: scope_type("symbol_code"), order("tracery","asc"), contract("commun.publication")]] = eosio::multi_index<"vertex"_n, vertex_struct>;
using accparams [[using eosio: scope_type("symbol_code"), order("account","asc"), contract("commun.publication")]] = eosio::multi_index<"accparam"_n, acc_param>;

} // commun
