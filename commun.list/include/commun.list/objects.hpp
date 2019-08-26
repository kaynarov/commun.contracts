#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

namespace commun::structures {

using namespace eosio;

struct community_contracts {
    name ctrl;
    name emit;
    name charge;
    name social;
    name publish;
};

struct community {
    symbol_code token_name;
    std::string community_name;
    community_contracts contracts;

    uint64_t primary_key() const {
        return token_name.raw();
    }
};

}

namespace commun::tables {
    using namespace eosio;

    using cmmn_name_index = eosio::indexed_by<"byname"_n, eosio::member<structures::community, std::string, &structures::community::community_name>>;
    using community = eosio::multi_index<"community"_n, structures::community, cmmn_name_index>;
}
