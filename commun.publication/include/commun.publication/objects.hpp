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

    // for set
    friend bool operator<(const mssgid_t& a, const mssgid_t& b) {
        return std::tie(a.author, a.permlink) < std::tie(b.author, b.permlink);
    }
    friend bool operator>(const mssgid_t& a, const mssgid_t& b) {
        return std::tie(a.author, a.permlink) > std::tie(b.author, b.permlink);
    }

    uint64_t tracery() const {
        std::string key = author.to_string() + "/" + permlink;
        auto hash = sha256(key.c_str(), key.size());
        return *(reinterpret_cast<const uint64_t *>(hash.extract_as_byte_array().data()));
    }
};

struct vertex_t {
    uint64_t tracery;
    uint64_t parent_tracery;
    uint16_t level;
    uint32_t childcount;

    uint64_t primary_key() const { return tracery; }
};

struct acc_param_t {
    name account;
    std::vector<name> providers;
    uint64_t primary_key() const { return account.value; }
};

using namespace eosio;

using vertices = eosio::multi_index<"vertex"_n, vertex_t>;
using accparams = eosio::multi_index<"accparam"_n, acc_param_t>;

} // commun
