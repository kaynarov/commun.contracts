#pragma once
#include <commun/upsert.hpp>
#include <commun/config.hpp>
#include <commun/dispatchers.hpp>

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
  \brief For each leader, a separate record is created in the database, containing the name and \a weight of the leader. Such record appears in DB after each calling send_leader_event().

  \ingroup control_tables
*/
// DOCS_TABLE: leader_info
struct leader_info {
    name name;       //!< a leader name
    bool active;     //!< \a true if the leader is active; default is \a true

    uint64_t total_weight;  //!< total \a weight of the leader taking into account the votes cast for him/her
    uint64_t counter_votes; //!< counter of votes cast for the leader
    
    int64_t unclaimed_points = 0; //!< the points accumulated from emission and not yet claimed by leader via \ref control::claim

    uint64_t primary_key() const {
        return name.value;
    }
    uint64_t weight_key() const {
        return total_weight;
    }
};

using leader_weight_idx [[using eosio: non_unique, order("total_weight","desc")]] = indexed_by<"byweight"_n, const_mem_fun<leader_info, uint64_t, &leader_info::weight_key>>;
using leader_tbl [[using eosio: scope_type("symbol_code"), order("name","asc"), contract("commun.ctrl")]] = eosio::multi_index<"leader"_n, leader_info, leader_weight_idx>;

/**
  \brief For each user who voted for a leader, a separate record is created in the database containing their names and strength of the vote. 
  
  \ingroup control_tables
 */
// DOCS_TABLE: leader_voter
struct leader_voter {
    uint64_t id;
    name voter; //!< a voter name
    name leader; //!< a leader name
    uint16_t pct; //!< share (in percent) of strength of the vote cast for the leader

    uint64_t primary_key() const { return id; }
    using key_t = std::tuple<name, name>;
    key_t by_voter()const { return std::make_tuple(voter, leader); }
    key_t by_leader()const { return std::make_tuple(leader, voter); }
};

using leadervote_byvoter_idx [[using eosio: order("voter","asc"), order("leader","asc")]] = eosio::indexed_by<"byvoter"_n, eosio::const_mem_fun<leader_voter, leader_voter::key_t, &leader_voter::by_voter> >;
using leadervote_byleader_idx [[using eosio: order("leader","asc"), order("voter","asc")]] = eosio::indexed_by<"byleader"_n, eosio::const_mem_fun<leader_voter, leader_voter::key_t, &leader_voter::by_leader> >;

using leader_vote_tbl [[using eosio: scope_type("symbol_code"), order("id","asc"), contract("commun.ctrl")]] = eosio::multi_index<"leadervote"_n, leader_voter, leadervote_byvoter_idx, leadervote_byleader_idx>;

/**
  \brief DB record containing information about a proposed transaction which needs to be signed by the accounts specified in this transaction.
  \ingroup control_tables
 */
// DOCS_TABLE: proposal
struct proposal {
    name proposal_name; //!< a name of proposed transaction. This is a primary key
    symbol_code commun_code; //!< symbol of the community whose leaders have to sign the transaction. It may be a company name whose representatives are entitled to sign the transaction
    name permission; //!< a level of permission required to sign the transaction. A person signing the transaction should have a permission level not lower than specified one
    std::vector<char> packed_transaction; //!< the proposed transaction

    uint64_t primary_key()const { return proposal_name.value; }
};

using proposals [[using eosio: order("proposal_name","asc"), contract("commun.ctrl")]] = eosio::multi_index< "proposal"_n, proposal>;

/**
  \brief The type of structure that is formed and stored in the approval_info when a leader sends an approval.
  \ingroup control_tables
 */
struct approval {
    name approver; //!< name of the leader approving a transaction
    time_point time; //!< time when the transaction was approved. This time should not go beyond the period set for approving the transaction
};

/**
  \brief DB record containing a list of approvals for proposed multi-signature transaction.
  \ingroup control_tables
 */
// DOCS_TABLE: proposal
struct approvals_info {
    name proposal_name; //!< a name of proposed multi-signature transaction
    std::vector<approval> provided_approvals; //!< the list of approvals received from certain leaders
    uint64_t primary_key()const { return proposal_name.value; }
};

using approvals [[using eosio: order("proposal_name","asc"), contract("commun.ctrl")]] = eosio::multi_index< "approvals"_n, approvals_info>;

/**
  \brief DB record containing the account name whose approval for performing a multi-signature transaction is to be revoked.
  \ingroup control_tables
 */
// DOCS_TABLE: invalidation
struct invalidation {
    name account; //!< the account whose signature in transaction is to be invalidated
    time_point last_invalidation_time; //!< the time when the previous signature of this account was invalidated

    uint64_t primary_key() const { return account.value; }
};

using invalidations [[using eosio: order("account","asc"), contract("commun.ctrl")]] = eosio::multi_index< "invals"_n, invalidation>;

/**
 * \brief This class implements the \a c.ctrl contract functionality.
 * \ingroup control_class
 */
class
/// @cond
[[eosio::contract("commun.ctrl")]]
/// @endcond
control: public contract {
    // DOCS_TABLE: stat_struct
    struct stat_struct {
        uint64_t id;
        int64_t retained = 0;
        uint64_t primary_key() const { return id; }
    };

    using stats [[using eosio: scope_type("symbol_code"), order("id","asc")]] = eosio::multi_index<"stat"_n, stat_struct>;
    
public:
    control(name self, name code, datastream<const char*> ds)
        : contract(self, code, ds)
    {
    }

    /**
     * \brief The \ref init action is used by \a c.list contract to initialize leaders for a community with specified point symbol.
     *
     * \param commun_code a point symbol of community
     *
     * This action is unavailable for user and can be called only internally.
     * \signreq
            — the \a c.list contract account .
     */
    [[eosio::action]] void init(symbol_code commun_code);

    /**
        \brief The \ref regleader action is used to register candidates for leaders of community.

        \param commun_code a point symbol of community
        \param leader a leader candidate name 
        \param url a website address from where information about the candidate can be obtained, including the reasons
        for her/his desire to become a leader. The address string must not exceed 256 characters

        \signreq
            — the <i>leader candidate</i> .
    */
    [[eosio::action]] void regleader(symbol_code commun_code, name leader, std::string url);

    /**
        \brief The \ref clearvotes action is used to remove votes cast for a leader.

        \param commun_code a point symbol used by the community
        \param leader the leader candidate name whose votes are removed
        \param count maximum number of votes that can be removed

        When this action is called, event information is stored in \a leaderstate and sent to the event engine. 

        \signreq
            — the <i>leader candidate</i> .
    */
    [[eosio::action]] void clearvotes(symbol_code commun_code, name leader, std::optional<uint16_t> count);

    /**
        \brief The \ref unregleader action is used to withdraw a user's candidacy from among the registered candidates to the leaders of a community.

        \param commun_code a point symbol of the community
        \param leader the user name to be removed from the list of leaders registered as candidates

        The action can be called either by the candidate (in case of a withdrawal) or by a leader who found a
        discrepancy of the leader capabilities to desirable leader requirements, and mismatched data published on the web
        site with her/his relevant data.

        Condition for performing the action:
            - no votes should be cast for this leader candidate. Votes of all users who voted for this leader candidate should be removed.

        \signreq
            — the <i>leader candidate</i> who is removed from the list .
     */
    [[eosio::action]] void unregleader(symbol_code commun_code, name leader);

    /**
        \brief The \ref stopleader action is used to temporarily suspend activity of a leader (or a leader candidate) of the specified community.

        \param commun_code a point symbol of the community
        \param leader account name of a leader (or a leader candidate) whose activity is temporarily suspended

        When this action is called, event information is stored in \p leaderstate and sent to the event engine.

        Condition for performing the action:
            - the leader account should be active.

        The leader account activity can be continued in case it has performed the \ref startleader action.

        \signreq
            — the \a leader whose activity is to be suspended .
    */
    [[eosio::action]] void stopleader(symbol_code commun_code, name leader);

    /**
        \brief The \ref startleader action is used to resume suspended leader activity (or a leader candidate activity).

        \param commun_code a point symbol of the community
        \param leader account name of a leader (or a leader candidate) whose activity is resumed

        When this action is called, event information is stored in \a leaderstate and sent to the event engine.

        Condition for performing the action:
            - the leader account activity should be suspended, that is, the operation \ref stopleader should be performed previously.

        \signreq
            — the \a leader whose activity is to be resumed .
     */
    [[eosio::action]] void startleader(symbol_code commun_code, name leader);

    /**
        \brief The \ref voteleader action is used to vote for a leader candidate.

        \param commun_code a point symbol of the community
        \param voter a user voting for the leader candidate
        \param leader a name of the leader candidate for whom the vote is cast
        \param pct a share (in percent) of voter power cast for the leader. If \a pct is empty, the leader is given all unused voter power

        When this action is called, event information is stored in \a leaderstate and sent to the event engine.

        Total amount of all shares that voter distributes among the leaders can not exceed 100 %. If \a pct is not set, then the percentage allocated to the leader is calculated as follows:

        <center> <i> pct = ( 100 — &sum; N<sub>j</sub> ) % </i> </center>

        <i> &sum; N<sub>j</sub> </i> — sum of shares allocated to another leaders.

        After voting, the leader’s total <i>weight</i> will be calculated as follows:

        <center> <i>weight = ( voter_balance × pct / 100 ) % </i> </center>

        This \a weight will be updated each time after changing the balance (see \ref changepoints).

        <b>Restrictions:</b>
            - the leader candidate name must first be registered through a call to regleader;
            - total number of votes cast by the voter account for all candidates should not exceed the max_votes community parameter value;
            - it is not allowed to vote for a leader candidate whose activity is suspended (after calling the stopleader action, for example).

        \signreq
            — the \a voter account .
    */
    [[eosio::action]] void voteleader(symbol_code commun_code, name voter, name leader, std::optional<uint16_t> pct);

    /**
        \brief The \ref unvotelead action is used to withdraw a previously cast vote for a leader candidate.

        \param commun_code a point symbol of the community
        \param voter a user who intends to withdraw her/his vote which was previously cast for the leader candidate
        \param leader the leader candidate name from whom the vote is withdrawn

        When this action is called, event information is stored in \a leaderstate and sent to the event engine.

        \signreq
            — the \a voter account .
    */
    [[eosio::action]] void unvotelead(symbol_code commun_code, name voter, name leader);

    /**
        \brief The \ref claim action is used to withdraw a reward by a leader

        \param commun_code a point symbol of the community
        \param leader account name of the leader

        \signreq
            — the \a leader who withdraws the reward .
    */
    [[eosio::action]] void claim(symbol_code commun_code, name leader);

    /**
        \brief The \ref emit action is used to emit rewards for community leaders

        \param commun_code a point symbol of the community

        \signreq
            — <i>trusted community client</i> .
    */
    [[eosio::action]] void emit(symbol_code commun_code);

    /**
        \brief The \ref changepoints action is an internal and unavailable to the user. It is used by \a c.point to notify the \a c.ctrl smart contract about a change of the points amount on the user's balance. The action is called automatically after changing the points amount on a user's balance. The \a weights of all the leaders voted for by this user are also updated.

        \param who an account name whose points amount has been changed.
        \param diff a relative change of points amount.

        When this action is called, event information is stored in \a leaderstate and sent to the event engine.
        \signreq
            - the \a c.point contract account .
    */
    [[eosio::action]] void changepoints(name who, asset diff);
    ON_TRANSFER(COMMUN_POINT) void on_points_transfer(name from, name to, asset quantity, std::string memo);

    /**
        \brief The \ref propose action is used to create a multi-signature transaction offer (proposed transaction) requiring permission from leaders.

        \param commun_code a point symbol of the community
        \param proposer an author of multi-signature transaction
        \param proposal_name the unique name assigned to the multi-signature transaction when it is created
        \param permission a name of the permission needing to sign the transaction
        \param trx the proposed transaction

        The pair of proposer and proposal_name can be used in another actions to identify created proposed transaction.

        \signreq
            — the \a proposer account .
    */
    [[eosio::action]] void propose(ignore<symbol_code> commun_code, ignore<name> proposer, ignore<name> proposal_name,
                ignore<name> permission, ignore<eosio::transaction> trx);

    /**
        \brief The \ref approve action is used to send permission to execute a multi-signature transaction. A name of person signing the transaction is saved in the list of approvals. The multi-signature transaction will be executed only after receiving necessary number of signatures with the corresponding permissions.

        \param proposer author of the multi-signature transaction
        \param proposal_name unique name assigned to the multi-signature transaction when it is created
        \param approver name of account approving multi-signature transaction
        \param proposal_hash a hash sum to check if the transaction is substituted or not (optional parameter)

        \signreq
            — a <i>top leader</i> .
    */
    [[eosio::action]] void approve(name proposer, name proposal_name, name approver,
                const eosio::binary_extension<eosio::checksum256>& proposal_hash);

    /**
        \brief The \ref unapprove actionis is used to revoke a previously sent permission (signature) for a multi-signature transaction in case of a change in decision.

        \param proposer author of the multi-signature transaction
        \param proposal_name unique name assigned to the multi-signature transaction when it is created
        \param approver name of account cancelling a previously sent permission

        <b>Restrictions:</b>
            - Only a person whose name is in approvals list can revoke a signature.

        \signreq
            — the \a approver account .
    */
    [[eosio::action]] void unapprove(name proposer, name proposal_name, name approver);

    /**
        \brief The \ref cancel action is used to free memory space via clearing proposed multi-signature transaction, that has not been executed.

        \param proposer author of the multi-signature transaction
        \param proposal_name unique name assigned to the multi-signature transaction when it is created
        \param canceler name of account cancelling the transaction

        <b>Restrictions:</b>
            - Only expired transaction can be canceled by a person who is not a proposer.

        \signreq
            — the \a canceler account .
    */
    [[eosio::action]] void cancel(name proposer, name proposal_name, name canceler);

    /**
        \brief The \ref exec action is used to execute proposed multi-signature transaction.

        \param proposer author of the multi-signature transaction
        \param proposal_name unique name assigned to the multi-signature transaction when it is created
        \param executer name of account executing transaction

        Conditions for transaction execution:
            - the transaction should not be expired;
            - there should be a required number of signatures (only the signatures of top leaders are taken into account, excluding invalid ones);
            - all operations contained in the transaction should be feasible;
            - the memory in the database allocated for the transaction can be erased.

        \signreq
            — the \a executer account .
    */
    [[eosio::action]] void exec(name proposer, name proposal_name, name executer);

    /**
        \brief The \ref invalidate action is used to revoke all permissions previously issued by the account for performing multi-signature transactions.

        \param account name of account revoking permissions

        \details This action revokes all permissions issued by this account for currently executing multi-signature transactions. This action does not apply to completed transactions, as well as to transactions that will be signed by this account after calling \ref invalidate. The proposed transaction can still be sent to the network if it contains the needed number of signatures, taking into account the withdrawn one.

        \signreq
            — the \a account revoking permissions .
    */
    [[eosio::action]] void invalidate(name account);

    /**
        \brief The \ref setrecover action is used to create dApp authority before danger operations which can cause dApp leaders lose access to \a c.ctrl multisignature transactions.

        \signreq
            — <i>majority of Commun dApp leaders</i> .
    */
    [[eosio::action]] void setrecover();

private:
    void check_started(symbol_code commun_code);
    uint8_t get_required(symbol_code commun_code, name permission);

    std::vector<name> top_leaders(symbol_code commun_code);
    std::vector<leader_info> top_leader_info(symbol_code commun_code);

    /**
      \brief The struct representing an event about updating a leader state. Such event is issued each time after calling \ref changepoints, \ref clearvotes, \ref voteleader, \ref unvotelead, \ref startleader and \ref stopleader.

      \ingroup control_events
    */
    struct [[eosio::event]] leaderstate_event {
        symbol_code commun_code; //!< a point symbol of the community to which the leader belongs
        name leader; //!< the leader name
        uint64_t weight; //!< total \a weight of the leader
        bool active; //!< \a true if the leader is active
    };

    void send_leader_event(symbol_code commun_code, const leader_info& wi);
    void active_leader(symbol_code commun_code, name leader, bool flag);
    int64_t get_power(symbol_code commun_code, name voter, uint16_t pct);

public:
    static inline bool in_the_top(symbol_code commun_code, name account) {
        const auto l = commun_list::get_control_param(commun_code).leaders_num;
        leader_tbl leader(config::control_name, commun_code.raw());
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

    static inline void require_leader_auth(symbol_code commun_code, name leader) {
        require_auth(leader);
        eosio::check(in_the_top(commun_code, leader), (leader.to_string() + " is not a leader").c_str());
    }
};

} // commun
