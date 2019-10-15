#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "config.hpp"
namespace commun {

using namespace eosio;

class emit: public contract {
struct structures {

    struct [[eosio::table]] stat {
        uint64_t id;

        time_point latest_mosaics_reward;
        time_point latest_leaders_reward;

        time_point latest_reward(bool for_leaders) const { return (for_leaders ? latest_leaders_reward : latest_mosaics_reward); };

        uint64_t primary_key() const { return id; }
    };
};
    using stats = eosio::multi_index<"stat"_n, structures::stat>;

    static int64_t get_continuous_rate(int64_t annual_rate);

public:
    using contract::contract;
    [[eosio::action]] void create(symbol_code commun_code);
    [[eosio::action]] void issuereward(symbol_code commun_code, bool for_leaders);

    static inline bool it_is_time_to_reward(name emit_contract_account, symbol_code commun_code, bool for_leaders) {
        stats stats_table(emit_contract_account, commun_code.raw());
        const auto& stat = stats_table.get(commun_code.raw(), "emitter does not exists");
        return (eosio::current_time_point() - stat.latest_reward(for_leaders)).to_seconds() >= config::reward_period(for_leaders);
    }
    
    static inline void maybe_issue_reward(name emit_contract_account, symbol_code commun_code, bool for_leaders) {
        if (it_is_time_to_reward(emit_contract_account, commun_code, for_leaders)) {
            action(
                permission_level{emit_contract_account, config::reward_perm_name},
                emit_contract_account,
                "issuereward"_n,
                std::make_tuple(commun_code, for_leaders)
            ).send();
        }
    }
};

} // commun
