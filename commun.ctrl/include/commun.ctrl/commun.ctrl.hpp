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
  \brief a struct represents leader record in a db table

  \ingroup control_tables
*/
struct [[eosio::table]] leader_info {
    name name;       /**< a leader name*/
    bool active;     /**< true if the leader is active*/            // can check key instead or even remove record

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
  \brief struct represents voter record in a db table
  \ingroup control_tables
 */
struct [[eosio::table]] leader_voter {
    uint64_t id;
    name voter; /**< a voter name*/
    name leader; /**< a leader name*/
    uint16_t pct; /**< percentage of voter power cast for the leader*/

    uint64_t primary_key() const { return id; }
    using key_t = std::tuple<name, name>;
    key_t by_voter()const { return std::make_tuple(voter, leader); }
    key_t by_leader()const { return std::make_tuple(leader, voter); }
};

using leadervote_id_idx = eosio::indexed_by<"leadervoteid"_n, eosio::const_mem_fun<leader_voter, uint64_t, &leader_voter::primary_key> >;
using leadervote_byvoter_idx = eosio::indexed_by<"byvoter"_n, eosio::const_mem_fun<leader_voter, leader_voter::key_t, &leader_voter::by_voter> >;
using leadervote_byleader_idx = eosio::indexed_by<"byleader"_n, eosio::const_mem_fun<leader_voter, leader_voter::key_t, &leader_voter::by_leader> >;

using leader_vote_tbl = eosio::multi_index<"leadervote"_n, leader_voter, leadervote_id_idx, leadervote_byvoter_idx, leadervote_byleader_idx>;

/**
  \brief struct represents DB record for transaction proposed and should be approved by leaders
  \ingroup control_tables
 */
struct [[eosio::table]] proposal {
    name proposal_name; //!< name of proposed transaction. A primary key
    symbol_code commun_code; //!< symbol of community (of leaders which should sign transaction)
    name permission; //!< name of leader permission which is need for transaction
    std::vector<char> packed_transaction; //!< transaction

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
    [[eosio::action]] void init(symbol_code commun_code);

    /**
        \brief The regleader action is used to register candidates for leaders of the specified community

        \param commun_code a point symbol of the community
        \param leader a name of a candidate for leaders
        \param url a website address from where information about the candidate can be obtained, including the reasons
        for her/his desire to become a leader. The address string must not exceed 256 characters

        Performing the action requires a signature of the leader candidate.
    */
    [[eosio::action]] void regleader(symbol_code commun_code, name leader, std::string url);

    /**
        \brief The action clearvotes is used to remove votes cast for a given leader

        \param commun_code a point symbol used by the community
        \param leader a name of the leader candidate
        \param count maximum number of votes to remove

        Sends leaderstate_event.

        Doing the clearvotes action requires signing the leader account.
    */
    [[eosio::action]] void clearvotes(symbol_code commun_code, name leader, std::optional<uint16_t> count);

    /**
        \brief The unregleader action is used to withdraw a user's candidacy from among the registered candidates to the leaders of the specified community

        \param commun_code a point symbol of the community
        \param leader the user name to be removed from the list of leaders registered as candidates

        The action can be called either by the candidate (in case of a withdrawal) or by a leader who found a
        discrepancy of the leader capabilities to desirable leader requirements, and mismatched data published on the web
        site with her/his relevant data.
        Conditions for performing the action:
            - no votes for this leader candidate. Votes of all users who voted for this leader candidate should be removed;
            - the transaction must be signed by the leader candidate himself
     */
    [[eosio::action]] void unregleader(symbol_code commun_code, name leader);

    /**
        \brief The stopleader action is used to temporarily suspend active actions of a leader (or a leader candidate) of the specified community

        \param commun_code a point symbol of the community
        \param leader account name of a leader (or a leader candidate) whose activity is temporarily suspended

        Sends leaderstate_event.

        Conditions for performing the action:
            - the leader account should be active;
            - a transaction must be signed by the leader account.
        The leader account activity can be continued in case it has performed the startleader action.
    */
    [[eosio::action]] void stopleader(symbol_code commun_code, name leader);

    /**
        \brief The startleader action is used to resume suspended leader activity (or a leader candidate activity)

        \param commun_code a point symbol of the community
        \param leader account name of a leader (or a leader candidate), whose activity is resumed

        Sends leaderstate_event.

        Conditions for performing the action:
            - the leader account activity should be suspended, that is, the operation stopleader should be performed previously;
            - a transaction must be signed by the leader account
     */
    [[eosio::action]] void startleader(symbol_code commun_code, name leader);

    /**
        \brief The voteleader action is used to vote for a leader candidate

        \param commun_code a point symbol of the community
        \param voter an account name that is voting for the leader candidate
        \param leader a name of the leader candidate for whom the vote is cast
        \param pct percentage of voter power cast for the leader; all unused power if pct is empty

        Sends leaderstate_event.

        Doing the action requires signing by the voter account.

        <b>Restrictions:</b>
            - the leader candidate name must first be registered through a call to regleader;
            - total number of votes cast by the voter account for all candidates should not exceed the max_votes community parameter value;
            - it is not allowed to vote for a leader candidate whose activity is suspended (after the candidate has completed the
            stopleader action).
    */
    [[eosio::action]] void voteleader(symbol_code commun_code, name voter, name leader, std::optional<uint16_t> pct);

    /**
        \brief The action unvotelead is used to withdraw a previously cast vote for a leader candidate

        \param commun_code a point symbol of the community
        \param voter an account name that intends to withdraw her/his vote which was previously cast for the leader candidate
        \param leader the leader candidate name from whom the vote is withdrawn

        Sends leaderstate_event.

        It is allowed to withdraw a vote cast for a leader candidate whose activity is suspended (after the candidate has completed
        the stopleader action).
        Doing the action requires signing by the voter account.
    */
    [[eosio::action]] void unvotelead(symbol_code commun_code, name voter, name leader);

    /**
        \brief The claim action is used to withdraw leader reward by the leader

        \param commun_code a point symbol of the community
        \param leader account name of a leader

        Performing the action requires a signature of the leader.
    */
    [[eosio::action]] void claim(symbol_code commun_code, name leader);

    /**
        \brief The changepoints is an internal and unavailable to the user action. It is used by commun.point smart contract to notify
        the commun.ctrl smart contract about a change of the points amount on the user's balance.

        \param who an account name whose points amount has been changed.
        \param diff a relative change of points amount.

        Sends leaderstate_event.

        Calling this action for a leader voter causes updating weights of leaders, voted by this voter.

        The action is called automatically each time in case the points amount is changed on a user's balance.
    */
    [[eosio::action]] void changepoints(name who, asset diff);
    void on_points_transfer(name from, name to, asset quantity, std::string memo);

    /**
        \brief The propose action is used to create a multi-signature transaction offer (proposed transaction), which requires permission from leader accounts.

        \param commun_code a point symbol of the community
        \param proposer author of transaction
        \param proposal_name the unique name assigned to the multi-signature transaction when it is created
        \param permission name of permission to sign transaction
        \param trx proposed transaction

        The pair of proposer and proposal_name is used in another actions to identify created proposed transaction.

        Performing the action requires a signature of the proposer.
    */
    [[eosio::action]] void propose(ignore<symbol_code> commun_code, ignore<name> proposer, ignore<name> proposal_name,
                ignore<name> permission, ignore<eosio::transaction> trx);

    /**
        \brief The approve action used by each approver of multi-sig transaction to mark that he/she approves it.

        \param proposer author of transaction
        \param proposal_name the unique name assigned to the multi-signature transaction when it is created
        \param approver name of account who approves transaction
        \param proposal_hash hash to check if transaction is not substituted (optional)

        Performing the action checks a signature of the approver, and adds approver to list of approvers of the transaction.
    */
    [[eosio::action]] void approve(name proposer, name proposal_name, name approver, 
                const eosio::binary_extension<eosio::checksum256>& proposal_hash);

    /**
        \brief The unapprove action used by approver of multi-sig transaction to undo his/her approve.

        \param proposer author of transaction
        \param proposal_name the unique name assigned to the multi-signature transaction when it is created
        \param approver name of account who undoes approve from transaction

        Performing the action checks a signature of the approver, and removes approver from list of approvers of the transaction.
    */
    [[eosio::action]] void unapprove(name proposer, name proposal_name, name approver);

    /**
        \brief The cancel action used to free memory by clearing proposed multi-sig transaction, which was not yet executed.

        \param proposer author of transaction
        \param proposal_name the unique name assigned to the multi-signature transaction when it is created
        \param canceler name of account who cancels transaction

        If canceler is not proposer, only expired transaction can be canceled.

        Performing the action checks a signature of the canceler.
    */
    [[eosio::action]] void cancel(name proposer, name proposal_name, name canceler);

    /**
        \brief The exec action used to execute proposed multi-sig transaction.

        \param proposer author of transaction
        \param proposal_name the unique name assigned to the multi-signature transaction when it is created
        \param executer name of account who executes transaction

        Action checks that transaction is not expired, that approvers on transaction are enough for threshold (considering only top leaders, excluding invalidated), sends all actions and erases transaction in DB.

        Performing the action checks a signature of the executer.
    */
    [[eosio::action]] void exec(name proposer, name proposal_name, name executer);

    /**
        \brief The invalidate action is used to revoke all permissions previously issued by the account for performing multi-signature transactions

        \param account name of account who revokes permissions

        Performing the action checks a signature of the account.
    */
    [[eosio::action]] void invalidate(name account);

private:
    void check_started(symbol_code commun_code);
    uint8_t get_required(symbol_code commun_code, name permission);

    std::vector<name> top_leaders(symbol_code commun_code);
    std::vector<leader_info> top_leader_info(symbol_code commun_code);

    /**
      \brief A struct represents event about leader state update (sending from \ref changepoints, \ref clearvotes, \ref voteleader, \ref unvotelead, \ref startleader, \ref stopleader)
      \ingroup control_events
    */
    struct leaderstate_event {
        symbol_code commun_code; //!< point symbol of community to which leader belongs
        name name; //!< a leader name
        uint64_t total_weight; //!< total weight of leader
        bool active; //!< true if the leader is active
    };

    void send_leader_event(symbol_code commun_code, const leader_info& wi);
    void active_leader(symbol_code commun_code, name leader, bool flag);
    int64_t get_power(symbol_code commun_code, name voter, uint16_t pct);

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
