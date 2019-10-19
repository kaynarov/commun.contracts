#pragma once
#include <commun/upsert.hpp>
#include <commun/config.hpp>

#include <eosio/time.hpp>
//#include <eosiolib/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/transaction.hpp>
#include <eosio/binary_extension.hpp>
//#include <eosio/public_key.hpp>
#include <commun.list/commun.list.hpp>
#include <vector>
#include <string>

namespace commun {
using namespace eosio;

using share_type = int64_t;

/**
  \brief a struct represents leader note in a db table

  \ingroup control_tables
*/
struct [[eosio::table]] leader_info {
    name name;       /**< a leader name*/
    bool active;     /**< true if the witness is active*/            // can check key instead or even remove record

    uint64_t total_weight;
    uint64_t counter_votes;
    
    int64_t unclaimed_points = 0;

    uint64_t primary_key() const {
        return name.value;
    }
    uint64_t weight_key() const {
        return total_weight;
    }
};

using leader_weight_idx = indexed_by<"byweight"_n, const_mem_fun<leader_info, uint64_t, &leader_info::weight_key>>;
using leader_tbl = eosio::multi_index<"leader"_n, leader_info, leader_weight_idx>;

/**
  \brief struct represents voter note in a db table
  \ingroup control_tables
 */
struct [[eosio::table]] leader_voter {
    name voter; /**< a voter name*/
    std::vector<name> leaders; /** witnesses the voter has voted for*/

    uint64_t primary_key() const {
        return voter.value;
    }
};

using leader_vote_tbl = eosio::multi_index<"leadervote"_n, leader_voter>;

/**
  \brief struct represents proposal note in a db table
  \ingroup control_tables
 */
struct [[eosio::table]] proposal {
    name proposal_name;
    symbol_code commun_code;
    name permission;
    std::vector<char> packed_transaction;

    uint64_t primary_key()const { return proposal_name.value; }
};

using proposals = eosio::multi_index< "proposal"_n, proposal>;

struct approval {
    name approver;
    time_point time;
};

/**
  \ingroup control_tables
*/
struct [[eosio::table]] approvals_info {
    name proposal_name;
    std::vector<approval> provided_approvals; 
    uint64_t primary_key()const { return proposal_name.value; }
};

using approvals = eosio::multi_index< "approvals"_n, approvals_info>;

/**
  \ingroup control_tables
*/
struct [[eosio::table]] invalidation {
    name account;
    time_point last_invalidation_time;

    uint64_t primary_key() const { return account.value; }
};

using invalidations = eosio::multi_index< "invals"_n, invalidation>;

/**
 * \brief This class implements comn.ctrl contract behaviour
 * \ingroup control_class
 */
class control: public contract {
    struct [[eosio::table]] stat {
        uint64_t id;
        int64_t retained = 0;
        uint64_t primary_key() const { return id; }
    };

    using stats = eosio::multi_index<"stat"_n, stat>;
    
public:
    control(name self, name code, datastream<const char*> ds)
        : contract(self, code, ds)
    {
    }
    [[eosio::action]] void create(symbol_code commun_code);

    /**
        \brief The regwitness action is used to register candidates for witnesses

        \param commun_code a point symbol used by the community
        \param witness a name of a candidate for witnesses
        \param url a website address from where information about the candidate can be obtained, including the reasons
        for her/his desire to become a witness. The address string must not exceed 256 characters

        Performing the regwitness action requires a signature of the witness candidate
    */
    [[eosio::action]] void regleader(symbol_code commun_code, name leader, std::string url);

    /**
        \brief The unregwitness action is used to withdraw a user's candidacy from among the registered candidates to the witnesses

        \param commun_code a point symbol used by the community
        \param witness the user name to be removed from the list of witnesses registered as candidates

        The unregwitness action can be called either by the candidate (in case of a withdrawal) or by a witness who found a
        discrepancy of the witness capabilities to desirable witness requirements, and mismatched data published on the web
        site with her/his relevant data.
        Conditions for performing the unregwitness action:
            - no votes for this witness candidate. Votes of all users who voted for this witness candidate should be removed;
            - the transaction must be signed by the witness candidate himself
     */
    [[eosio::action]] void unregleader(symbol_code commun_code, name leader);

    /**
        \brief The stopwitness action is used to temporarily suspend active actions of a witness (or a witness candidate)

        \param commun_code a point symbol used by the community
        \param witness account name of a witness (or a witness candidate) whose activity is temporarily suspended

        Conditions for performing the stopwitness action:
            - the witness account should be active;
            - a transaction must be signed by the witness account.
        The witness account activity can be continued in case it has performed the startwitness action
    */
    [[eosio::action]] void stopleader(symbol_code commun_code, name leader);

    /**
        \param The startwitness action is used to resume suspended witness activity (or a witness candidate activity)

        \param commun_code a point symbol used by the community
        \param witness account name of a witness (or a witness candidate), whose activity is resumed

        Conditions for performing the startfitness action:
            - the witness account activity should be suspended, that is, the operation stopwitness should be performed previously;
            - a transaction must be signed by the witness account
     */
    [[eosio::action]] void startleader(symbol_code commun_code, name leader);

    /**
        \brief The votewitness action is used to vote for a witness candidate

        \param commun_code a point symbol used by the community
        \param voter an account name that is voting for the witness candidate
        \param witness a name of the witness candidate for whom the vote is cast

        Doing the votewitness action requires signing the voter account

        *Restrictions:*
            - the witness candidate name must first be registered through a call to regwitness;
            - total number of votes cast by the voter account for all candidates should not exceed the max_witness_votes parameter value;
            - it is not allowed to vote for a witness candidate whose activity is suspended (after the candidate has completed the
            stopwitness action).

    */
    [[eosio::action]] void voteleader(symbol_code commun_code, name voter, name leader);

    /**
        \brief The action unvotewitn is used to withdraw a previously cast vote for a witness candidate

        \param commun_code a point symbol used by the community
        \param voter an account name that intends to withdraw her/his vote which was previously cast for the witness candidate
        \param witness the witness candidate name from whom the vote is withdrawn

        It is allowed to withdraw a vote cast for a witness candidate whose activity is suspended (after the candidate has completed
        the stopwitness action).
        Doing the unvotewitn action requires signing the voter account.
    */
    [[eosio::action]] void unvotelead(symbol_code commun_code, name voter, name leader);
    
    [[eosio::action]] void claim(symbol_code commun_code, name leader);

    /**
        \param The changepoints is an internal and unavailable to the user action. It is used by golos.vesting smart contract to notify
        the golos.ctrl smart contract about a change of the vesting amount on the user's balance

        \param who an account name whose vesting amount has been changed.
        \param diff a relative change of vesting amount.

        The changepoints action is called automatically each time in case the points amount is changed on a user's balance.
    */
    [[eosio::action]] void changepoints(name who, asset diff);
    void on_points_transfer(name from, name to, asset quantity, std::string memo);

    [[eosio::action]] void propose(ignore<symbol_code> commun_code, ignore<name> proposer, ignore<name> proposal_name,
                ignore<name> permission, ignore<eosio::transaction> trx);
    [[eosio::action]] void approve(name proposer, name proposal_name, name approver, 
                const eosio::binary_extension<eosio::checksum256>& proposal_hash);
    [[eosio::action]] void unapprove(name proposer, name proposal_name, name approver);
    [[eosio::action]] void cancel(name proposer, name proposal_name, name canceler);
    [[eosio::action]] void exec(name proposer, name proposal_name, name executer);
    [[eosio::action]] void invalidate(name account);

private:
    void check_started(symbol_code commun_code);
    uint8_t get_required(symbol_code commun_code, name permission);

    std::vector<name> top_leaders(symbol_code commun_code);
    std::vector<leader_info> top_leader_info(symbol_code commun_code);

    void change_voter_points(symbol_code commun_code, name voter, share_type diff);
    void apply_vote_weight(symbol_code commun_code, name voter, name leader, bool add);
    void update_leaders_weights(symbol_code commun_code, std::vector<name> leaders, share_type diff);
    void send_leader_event(symbol_code commun_code, const leader_info& wi);
    void active_leader(symbol_code commun_code, name leader, bool flag);

public:
    static inline bool in_the_top(name ctrl_contract_account, symbol_code commun_code, name account) {
        const auto l = commun_list::get_control_param(config::list_name, commun_code).leaders_num;
        leader_tbl leader(ctrl_contract_account, commun_code.raw());
        auto idx = leader.get_index<"byweight"_n>();    // this index ordered descending
        size_t i = 0;
        for (auto itr = idx.begin(); itr != idx.end() && i < l; ++itr) {
            if (itr->active && itr->total_weight > 0) {
                if (itr->name == account) {
                    return true;
                }
                ++i;
            }
        }
        return false;
    }
};

} // commun
