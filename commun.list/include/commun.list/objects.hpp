#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

namespace commun::structures {

using namespace eosio;

struct community {
    symbol_code commun_code;
    std::string community_name;

    uint64_t primary_key() const {
        return commun_code.raw();
    }
};

}

namespace commun::tables {
    using namespace eosio;

    using comn_name_index = eosio::indexed_by<"byname"_n, eosio::member<structures::community, std::string, &structures::community::community_name>>;
    using community = eosio::multi_index<"community"_n, structures::community, comn_name_index>;
}
