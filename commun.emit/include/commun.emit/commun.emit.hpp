#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "config.hpp"
#include <commun/dispatchers.hpp>
namespace commun {

using namespace eosio;

/**
 * \brief This class implements the operation logic of the \a c.emit contract.
 * \ingroup emission_class
 */
class
/// @cond
[[eosio::contract("commun.emit")]]
/// @endcond
emit: public contract {
struct structures {
    /**
     * \brief The structure represents the record form in the DB statistical table and contains information about a reward recipient.
     * \ingroup emission_tables
     */
    struct reward_struct final {
        name contract;   //!< Name of the contract receiving the reward
        time_point time; //!< Time (in seconds) when the contract was rewarded
    };

    /**
     * \brief The structure represents the record form in the DB statistical table.
     * \ingroup emission_tables
     *
     * The structure contains information about the latest recipients of rewards in community.
     */
    struct stat_struct {
        uint64_t id;  //!< Community identifier, unique primary key
        std::vector<reward_struct> reward_receivers; //!< List of rewards receivers

        const reward_struct& get_reward_receiver(name to_contract) const {
            return get_reward_receiver<const reward_struct>(reward_receivers, to_contract);
        }

        reward_struct& get_reward_receiver(name to_contract) {
            return get_reward_receiver<reward_struct>(reward_receivers, to_contract);
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
    using stats [[using eosio: order("id","asc"), scope_type("symbol_code")]] = eosio::multi_index<"stat"_n, structures::stat_struct>;

    static int64_t get_continuous_rate(int64_t annual_rate);

public:
    using contract::contract;

    /**
     * \brief The \ref init action is used by \a c.list contract to initialize emission for a community specified by point symbol.
     *
     * \param commun_code point symbol of the community for which emission is initialized
     *
     * \details The action is internal and its call is unavailable to users. 
     *
     * Funds issued due to emission are divided between the top mosaics. Mosaics for reward are selected from among the candidates included in the top list. The number of candidates in this list is determined by the \a default_comm_grades parameter. It is set by leaders and defaults to 20. The number of most rated mosaics submitted for reward is also set by leaders and defaults to 10.

     * Distribution of rewards between mosaics is carried out by \a c.gallery and \a c.ctrl contracts. The \a c.emit contract only issues requested amount of points and transfers them to those contracts.
     
     * Presence and location of a mosaic in the top list is determined by the \a comm_rating parameter. This parameter means total amount of rating points (hereinafter — ratings) obtained by a mosaic during collection of user opinions taking into account the sign of vote (positive or negative). The more ratings, the higher position of a mosaic in the list.
     
     * Total amount of ratings obtained for all top mosaics is calculated by the formula:
     * <center> <i> points_sum = &sum; comm_rating<sub>j</sub> </i> </center>
     * Component of the formula:
     * - <i> comm_rating<sub>j</sub> </i> — amount of ratings of j-th mosaic in the top list. By default, j takes the value from 1 to 20.
     
     * Each mosaic is assigned amount of grade ratings taken from the \a default_comm_grades array in accordance with position in the top list. In addition, each top mosaic is also assigned a share of the whole grade ratings.

     Total number of ratings assigned to the j-th mosaic is calculated by the formula:
     * <center> <i> SUM<sub>j</sub> = default_comm_grades[ j ] + ( default_comm_points_grade_sum × comm_rating<sub>j</sub> ) / points_sum </i> </center>
     * Components of the formula:
     * - \a j — mosaic position, it takes the values from 1 to 20 (default is 20 positions);
     * - \a default_comm_grades — array of the ratings which are assigned to the mosaic according to its position in the top list. Default is {1000, 1000, 1000, 500, 500, 300, 300, 300, 300, 300, 100, 100, 100, 100, 100, 50, 50, 50, 50, 50}. For example, mosaic on the fourth position is assigned 500 ratings;
     * - \a default_comm_points_grade_sum — amount of whole grade ratings. Default value is 5000. Each top mosaic is additionally assigned a share of this amount, taking into account the opinions of users — the mosaic weight.
     
     * The first mosaics having the highest SUM will be awarded (default is 10 mosaics).
     
     * \signreq
     *      — the \a c.list contract account .
     */
    [[eosio::action]] void init(symbol_code commun_code);

    /**
     * \brief The \ref issuereward action is called by other contracts to issue next batch of points to be used by these contracts for rewarding. This action checks if it is time to reward. If so, this action transfers points to contract that called it.
     
     * \param commun_code point symbol to be issued
     * \param to_contract name of contract that is a recipient and for which points are issued.
     
     * \details The \ref issuereward action is internal and its call is unavailable to users. It can only be called by contracts that have received permission from \a c.emit. This action determines the time elapsed since last award.
     
     * The period of access to \ref issuereward is set up separately for each contract. Number of points issued at each call of \ref issuereward is determined taking into account this period and set percentage of annual emission. If a contract calls the action frequently, the number of points issued will be less.
     
     * If \a c.ctrl or \a c.gallery makes a request for points, then the points will be issued as a reward for community leaders or for mosaics respectively.
     
     * Number of points to be issued and transfered to contract is calculated by the formula:
     * <center> <i> amount = get_supply() × get_continuous_rate() × ( passed_seconds / seconds_per_year ) × prcnt </i> </center>
     * Components of the formula:
     * - get_supply() — method determining the number of points currently in circulation;
     * - get_continious_rate() — method converting the set percentage of annual emission into a numerical value. This value is used to determine the number of points that can be issued at the moment;
     * - passed_seconds — number of seconds elapsed by the time \ref issuereward is called;
     * - seconds_per_year — seconds per year;
     * - prcnt — share of points determined by the \a def_leaders_percent hardcoded value. It takes the value of \a def_leaders_percent when the request comes from \a c.ctrl and (100 - \a def_leaders_percent) when it comes from \a c.gallery.
     
     * \signreq  
            — the \a c.list contract account
            or
            - the \a c.gallery contract account .
     */
    [[eosio::action]] void issuereward(symbol_code commun_code, name to_contract);

    static inline bool is_it_time_to_reward(const commun::structures::community& community, name to_contract, int64_t passed_seconds) {
        eosio::check(passed_seconds >= 0, "SYSTEM: incorrect passed_seconds");
        return passed_seconds >= community.get_emission_receiver(to_contract).period;
    }

    static inline bool is_it_time_to_reward(symbol_code commun_code, name to_contract) {
        stats stats_table(config::emit_name, commun_code.raw());
        auto& stat = stats_table.get(commun_code.raw(), "emitter does not exists");
        auto& community = commun_list::get_community(commun_code);

        return is_it_time_to_reward(community, to_contract, stat.last_reward_passed_seconds(to_contract));
    }
    
    static inline void maybe_issue_reward(symbol_code commun_code, name to_contract) {
        if (is_it_time_to_reward(commun_code, to_contract)) {
           call_issue_reward_action(commun_code, to_contract);
        }
    }

    static inline void issue_reward(symbol_code commun_code, name to_contract) {
        eosio::check(is_it_time_to_reward(commun_code, to_contract), "it isn't time for reward");
        call_issue_reward_action(commun_code, to_contract);
    }

private:
    static inline void call_issue_reward_action(symbol_code commun_code, name to_contract) {
        action(
            permission_level{config::emit_name, config::reward_perm_name},
            config::emit_name,
            "issuereward"_n,
            std::make_tuple(commun_code, to_contract)
        ).send();
    }
};

} // commun
