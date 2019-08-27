#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>


namespace commun::structures {

using namespace eosio;

struct pinblock_record {
    pinblock_record() = default;

    name account;
    bool pinning = false;
    bool blocking = false;

    uint64_t primary_key() const {
        return account.value;
    }

    EOSLIB_SERIALIZE(pinblock_record, (account)(pinning)(blocking))
};

}

namespace commun::tables {
    using pinblock_table = eosio::multi_index<"pinblock"_n, structures::pinblock_record>;
}
