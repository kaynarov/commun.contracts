#include "objects.hpp"

namespace commun {

using namespace eosio;

class commun_list: public contract {
public:
    using contract::contract;

    [[eosio::action]] void create(symbol symbol, name token, name ctrl, name vesting, name emit,
                                  name charge, name posting, name social, name referral);

    [[eosio::action]] void update(symbol symbol, name token, name ctrl, name vesting, name emit,
                                  name charge, name posting, name social, name referral);
};

}
