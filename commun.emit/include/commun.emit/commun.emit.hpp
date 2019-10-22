#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "config.hpp"
namespace commun {

using namespace eosio;

/**
 * \brief This class implements comn.emit contract behaviour
 * \ingroup emission_class
 */
class emit: public contract {
struct structures {

    /**
     * \brief struct represents record in stats table in the db
     * \ingroup emission_tables
     *
     * Contains information about reward receiver
     */
    struct reward_receiver final {
        name contract;   //!< Contract name
        time_point time; //!< Last time point of reward
    };

    /**
     * \brief struct represents an stats table in a db
     * \ingroup emission_tables
     *
     * Contains information about last time points of rewards in community
     */
    struct [[eosio::table]] stat {
        uint64_t id;  //!< An unique primary key
        std::vector<reward_receiver> reward_recievers; //!< List of rewards receiver

        const reward_receiver& get_reward_receiver(name to_contract) const {
            return get_reward_receiver<const reward_receiver>(reward_recievers, to_contract);
        }

        reward_receiver& get_reward_receiver(name to_contract) {
            return get_reward_receiver<reward_receiver>(reward_recievers, to_contract);
        }

        int64_t last_reward_passed_seconds(name to_contract) const {
            return (eosio::current_time_point() - get_reward_receiver(to_contract).time).to_seconds();
        }

        uint64_t primary_key() const { return id; }

    private:
        template <typename Result, typename List>
        Result& get_reward_receiver(List& list, name to_contract) const {
            auto itr = std::find_if(list.begin(), list.end(), [&](auto& receiver) {
                return receiver.contract == to_contract;
            });
            eosio::check(itr != list.end(), to_contract.to_string() + " wasn't initialized for emission");
            return *itr;
        }
    };
};
    using stats = eosio::multi_index<"stat"_n, structures::stat>;

    static int64_t get_continuous_rate(int64_t annual_rate);

public:
    using contract::contract;

    /**
     * \brief Action is used by commun.list contract to initialize emission for community with specified point symbol.
     *
     * \param commun_code a point symbol of the community
     *
     * The action is unavailable for user, can be callen only internally.
     */
    [[eosio::action]] void init(symbol_code commun_code);

    /**
     * \brief Action is used by other contracts to emit POINTs to them. It checks if it is time to reward, and if yes, transfers the points to contract.
     *
     * \param commun_code symbol of POINT to emit.
     * \param to_contract name of contract, which should receive emitted POINTs
     *
     * Action can be called by contracts on frequent user actions to reward if it is time to it..
     *
     * The action is unavailable for user, can be callen only internally by contracts to which commun.emit contract permits it.
     */
    [[eosio::action]] void issuereward(symbol_code commun_code, name to_contract);

    static inline bool is_it_time_to_reward(const commun::structures::community& community, name to_contract, int64_t passed_seconds) {
        eosio::check(passed_seconds >= 0, "SYSTEM: incorrect passed_seconds");
        return passed_seconds >= community.get_emission_receiver(to_contract).period;
    }

    static inline bool is_it_time_to_reward(symbol_code commun_code, name to_contract) {
        stats stats_table(config::emit_name, commun_code.raw());
        auto& stat = stats_table.get(commun_code.raw(), "emitter does not exists");
        auto& community = commun_list::get_community(config::list_name, commun_code);

        return is_it_time_to_reward(community, to_contract, stat.last_reward_passed_seconds(to_contract));
    }
    
    static inline void maybe_issue_reward(symbol_code commun_code, name to_contract) {
        if (is_it_time_to_reward(commun_code, to_contract)) {
            action(
                permission_level{config::emit_name, config::reward_perm_name},
                config::emit_name,
                "issuereward"_n,
                std::make_tuple(commun_code, to_contract)
            ).send();
        }
    }
};

} // commun
