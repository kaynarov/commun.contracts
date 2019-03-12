#include <eosiolib/eosio.hpp>


namespace commun::structures {

using namespace eosio;

struct community {
    symbol symbol;
    name ctrl;
    name emit;
    name token;
    name charge;
    name social;
    name vesting;
    name posting;
    name referral;
    uint64_t id;

    uint64_t primary_key() const {
        return id;
    }

    std::tuple<uint64_t, uint64_t> secondary_key() const {
        return {symbol.raw(), token.value};
    }
};

}

namespace commun::tables {
    using community_index = eosio::indexed_by<"bytoken"_n, eosio::const_mem_fun<structures::community, std::tuple<uint64_t, uint64_t>, &structures::community::secondary_key>>;
    using community = eosio::multi_index<"community"_n, structures::community, community_index>;
}
