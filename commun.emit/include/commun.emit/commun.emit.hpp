#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "config.hpp"
namespace commun {

using namespace eosio;

class emit: public contract {
struct structures {
    struct [[eosio::table]] param {
        uint64_t id;
        uint16_t annual_emission_rate;
        uint16_t leaders_reward_prop;
        uint64_t primary_key()const { return id; }
    };
    
    struct [[eosio::table]] stat {
        uint64_t id;
        
        time_point latest_mosaics_reward;
        time_point latest_leaders_reward;
        
        time_point latest_reward(bool for_leaders) const { return (for_leaders ? latest_leaders_reward : latest_mosaics_reward); };

        uint64_t primary_key()const { return id; }
    };
    
};
    using params = eosio::multi_index<"param"_n, structures::param>;
    using stats = eosio::multi_index<"stat"_n, structures::stat>;
    
    static int64_t get_continuous_rate(int64_t annual_rate);
    
public:
    using contract::contract;
    [[eosio::action]] void create(symbol_code commun_code, uint16_t annual_emission_rate, uint16_t leaders_reward_prop);
    [[eosio::action]] void issuereward(symbol_code commun_code, bool for_leaders);
    
    static inline bool it_is_time_to_reward(name emit_contract_account, symbol_code commun_code, bool for_leaders) {
        stats stats_table(emit_contract_account, commun_code.raw());
        const auto& stat = stats_table.get(commun_code.raw(), "emitter does not exists");
        return (eosio::current_time_point() - stat.latest_reward(for_leaders)).to_seconds() >= config::reward_period(for_leaders);
    }
};

}
