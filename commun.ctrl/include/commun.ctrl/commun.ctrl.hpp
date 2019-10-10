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

struct [[eosio::table]] leader_info {
    name name;
    std::string url;        // not sure it's should be in db (but can be useful to get leader info)
    bool active;            // can check key instead or even remove record

    uint64_t total_weight;
    uint64_t counter_votes;

    uint64_t primary_key() const {
        return name.value;
    }
    uint64_t weight_key() const {
        return total_weight;
    }
};
using leader_weight_idx = indexed_by<"byweight"_n, const_mem_fun<leader_info, uint64_t, &leader_info::weight_key>>;
using leader_tbl = eosio::multi_index<"leader"_n, leader_info, leader_weight_idx>;


struct [[eosio::table]] leader_voter {
    name voter;
    std::vector<name> leaders;

    uint64_t primary_key() const {
        return voter.value;
    }
};
using leader_vote_tbl = eosio::multi_index<"leadervote"_n, leader_voter>;

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

struct [[eosio::table]] approvals_info {
    name proposal_name;
    std::vector<approval> provided_approvals; 
    uint64_t primary_key()const { return proposal_name.value; }
};

using approvals = eosio::multi_index< "approvals"_n, approvals_info>;

struct [[eosio::table]] invalidation {
    name account;
    time_point last_invalidation_time;

    uint64_t primary_key() const { return account.value; }
};

using invalidations = eosio::multi_index< "invals"_n, invalidation>;

class control: public contract {
    
public:
    control(name self, name code, datastream<const char*> ds)
        : contract(self, code, ds)
    {
    }

    [[eosio::action]] void regleader(symbol_code commun_code, name leader, std::string url);
    [[eosio::action]] void unregleader(symbol_code commun_code, name leader);
    [[eosio::action]] void stopleader(symbol_code commun_code, name leader);
    [[eosio::action]] void startleader(symbol_code commun_code, name leader);
    [[eosio::action]] void voteleader(symbol_code commun_code, name voter, name leader);
    [[eosio::action]] void unvotelead(symbol_code commun_code, name voter, name leader);

    [[eosio::action]] void changepoints(name who, asset diff);
    void on_transfer(name from, name to, asset quantity, std::string memo);

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
