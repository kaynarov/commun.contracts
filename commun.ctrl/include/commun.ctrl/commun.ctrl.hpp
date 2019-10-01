#pragma once
#include "commun.ctrl/parameters.hpp"
#include <commun/upsert.hpp>
#include <commun/config.hpp>
#include <eosio/time.hpp>
//#include <eosiolib/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
//#include <eosio/public_key.hpp>
#include <vector>
#include <string>

namespace commun {

using namespace eosio;
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

//struct [[eosio::table]] msig_auths {
//    std::vector<name> witnesses;
//    time_point_sec last_update;
//};
//using msig_auth_singleton = eosio::singleton<"msigauths"_n, msig_auths>;


class control: public contract {
public:
    control(name self, name code, datastream<const char*> ds)
        : contract(self, code, ds)
    {
    }

    [[eosio::action]] void validateprms(symbol_code commun_code, std::vector<ctrl_param>);
    [[eosio::action]] void setparams(symbol_code commun_code, std::vector<ctrl_param>);

    [[eosio::action]] void regwitness(symbol_code commun_code, name witness, std::string url);
    [[eosio::action]] void unregwitness(symbol_code commun_code, name witness);
    [[eosio::action]] void stopwitness(symbol_code commun_code, name witness);
    [[eosio::action]] void startwitness(symbol_code commun_code, name witness);
    [[eosio::action]] void votewitness(symbol_code commun_code, name voter, name witness);
    [[eosio::action]] void unvotewitn(symbol_code commun_code, name voter, name witness);

    [[eosio::action]] void changepoints(name who, asset diff);
    void on_transfer(name from, name to, asset quantity, std::string memo);

private:
    ctrl_params_singleton& config(symbol_code commun_code) {
        static ctrl_params_singleton cfg(_self, commun_code.raw());
        return cfg;
    }
    const ctrl_state& props(symbol_code commun_code) {
        static const ctrl_state cfg = config(commun_code).get();
        return cfg;
    }
    void assert_started(symbol_code commun_code);

    std::vector<name> top_witnesses(symbol_code commun_code);
    std::vector<witness_info> top_witness_info(symbol_code commun_code);

    void change_voter_points(symbol_code commun_code, name voter, share_type diff);
    void apply_vote_weight(symbol_code commun_code, name voter, name witness, bool add);
    void update_witnesses_weights(symbol_code commun_code, std::vector<name> witnesses, share_type diff);
    //void update_auths();
    void send_witness_event(symbol_code commun_code, const witness_info& wi);
    void active_witness(symbol_code commun_code, name witness, bool flag);

public:
    static inline bool in_the_top(name ctrl_contract_account, symbol_code commun_code, name account) {
        ctrl_params_singleton cfg(ctrl_contract_account, commun_code.raw());
        eosio::check(cfg.exists(), "control is not initialized");
        const auto l = cfg.get().witnesses.max;
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
