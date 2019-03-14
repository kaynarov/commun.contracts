#include <eosiolib/eosio.hpp>


namespace commun::structures {

using namespace eosio;

struct community {
    uint64_t id;
    symbol symbol;
    name ctrl;
    name emit;
    name token;
    name charge;
    name social;
    name vesting;
    name posting;
    name referral;

    uint64_t primary_key() const {
        return id;
    }

    std::tuple<eosio::symbol, eosio::name> secondary_key() const {
        return {symbol, token};
    }
};

}

namespace commun::tables {
    using community_index = eosio::indexed_by<"secondary"_n, eosio::const_mem_fun<structures::community, std::tuple<eosio::symbol, eosio::name>, &structures::community::secondary_key>>;
    using community = eosio::multi_index<"community"_n, structures::community, community_index>;
}
