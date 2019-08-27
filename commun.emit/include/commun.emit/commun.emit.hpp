#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
namespace commun {

using namespace eosio;

class emit: public contract {
struct structures {
    struct [[eosio::table]] param {
        uint64_t id;
        symbol commun_symbol;
        uint16_t annual_emission_rate;
        uint16_t leaders_reward_prop;
        uint64_t primary_key()const { return id; }
    };
};
    using params = eosio::multi_index<"param"_n, structures::param>;
     
    static int64_t get_continuous_rate(int64_t annual_rate);
        
    void issue_reward(symbol commun_symbol, int64_t passed_seconds, name recipient_contract, bool for_leaders);
    
public:
    using contract::contract;
    [[eosio::action]] void create(symbol commun_symbol, uint16_t annual_emission_rate, uint16_t leaders_reward_prop);
    [[eosio::action]] void mscsreward(symbol commun_symbol, int64_t passed_seconds, name recipient_contract);
    [[eosio::action]] void ldrsreward(symbol commun_symbol, int64_t passed_seconds, name recipient_contract);
};

}
