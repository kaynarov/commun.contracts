#pragma once
#include <commun/upsert.hpp>
#include <commun/config.hpp>
#include <eosio/time.hpp>
//#include <eosiolib/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/transaction.hpp>
#include <eosio/binary_extension.hpp>
//#include <eosio/public_key.hpp>
//#include <commun.list/commun.list.hpp>
#include <vector>
#include <string>

namespace commun {
using namespace eosio;


// TODO: move to commun_list
namespace structures {

struct threshold {
    name permission;
    uint8_t required;
};

struct control_param {
    uint8_t leaders_num;
    std::vector<threshold> thresholds;
    uint8_t max_votes;
    void validate() {}; // TODO
};

struct param {
    symbol_code commun_code;
    control_param control;
    uint64_t primary_key() const {
        return commun_code.raw();
    }
};

struct sysparam {
    control_param control;
};

}

namespace config {
static constexpr uint8_t default_comm_leaders_num = 3;
static const std::array<structures::threshold, 2>  default_comm_thresholds = 
    {{structures::threshold{active_name, 2}, structures::threshold{ban_name, 1}}};
static constexpr uint8_t default_comm_max_votes = 5;

    
static constexpr uint8_t default_dapp_leaders_num = 21;
static const std::array<structures::threshold, 1>  default_dapp_thresholds = 
    {{structures::threshold{active_name, 11}}};
static constexpr uint8_t default_dapp_max_votes = 30;
}

namespace tables {
using params = eosio::multi_index<"param"_n, structures::param>;
using sysparams = eosio::singleton<"sysparam"_n, structures::sysparam>;
}
////////////////////////////////////////////////////////////

static const name control_param_contract = config::control_name; // TODO: config::list_name

using share_type = int64_t;

struct [[eosio::table]] witness_info {
    name name;
    std::string url;        // not sure it's should be in db (but can be useful to get witness info)
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
using witness_weight_idx = indexed_by<"byweight"_n, const_mem_fun<witness_info, uint64_t, &witness_info::weight_key>>;
using witness_tbl = eosio::multi_index<"witness"_n, witness_info, witness_weight_idx>;


struct [[eosio::table]] witness_voter {
    name voter;
    std::vector<name> witnesses;

    uint64_t primary_key() const {
        return voter.value;
    }
};
using witness_vote_tbl = eosio::multi_index<"witnessvote"_n, witness_voter>;

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

    [[eosio::action]] void regwitness(symbol_code commun_code, name witness, std::string url);
    [[eosio::action]] void unregwitness(symbol_code commun_code, name witness);
    [[eosio::action]] void stopwitness(symbol_code commun_code, name witness);
    [[eosio::action]] void startwitness(symbol_code commun_code, name witness);
    [[eosio::action]] void votewitness(symbol_code commun_code, name voter, name witness);
    [[eosio::action]] void unvotewitn(symbol_code commun_code, name voter, name witness);

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
    
    [[eosio::action]] void setparams(symbol_code commun_code, std::optional<structures::control_param> p); //TODO: remove it

private:
    void check_started(symbol_code commun_code);

    std::vector<name> top_witnesses(symbol_code commun_code);
    std::vector<witness_info> top_witness_info(symbol_code commun_code);

    void change_voter_points(symbol_code commun_code, name voter, share_type diff);
    void apply_vote_weight(symbol_code commun_code, name voter, name witness, bool add);
    void update_witnesses_weights(symbol_code commun_code, std::vector<name> witnesses, share_type diff);
    void send_witness_event(symbol_code commun_code, const witness_info& wi);
    void active_witness(symbol_code commun_code, name witness, bool flag);
    
    // TODO: move to commun_list
    static inline structures::control_param get_control_param(name contract_account, symbol_code commun_code) {
        if (commun_code) {
            tables::params params_table(contract_account, commun_code.raw());
            return params_table.get(commun_code.raw(), "not created").control;
        }
        else {
            tables::sysparams sysparams_table(contract_account, contract_account.value);
            eosio::check(sysparams_table.exists(), "not initialized");
            return sysparams_table.get().control;
        }
    }
    
    static inline bool initialized(name contract_account) {
        tables::sysparams sysparams_table(contract_account, contract_account.value);
        return sysparams_table.exists();
    }
    
    static inline bool created(name contract_account, symbol_code commun_code) {
        tables::params params_table(contract_account, commun_code.raw());
        return params_table.find(commun_code.raw()) != params_table.end();
    }
    ////////////////////////////

public:
    static inline bool in_the_top(name ctrl_contract_account, symbol_code commun_code, name account) {
        const auto l = get_control_param(control_param_contract, commun_code).leaders_num;
        witness_tbl witness(ctrl_contract_account, commun_code.raw());
        auto idx = witness.get_index<"byweight"_n>();    // this index ordered descending
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
