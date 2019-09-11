#pragma once
#include <commun/parameter.hpp>
#include <commun/config.hpp>
#include <eosio/singleton.hpp>

namespace commun { namespace param {


using namespace eosio;

struct multisig: parameter {
    name name;

    void validate() const override {
        eosio::check(is_account(name), "multisig account doesn't exists");
    }
};

// set to immutable for now coz it requires weights recalculation if decreasing `max` value
struct witness_votes: immutable_parameter {
    uint16_t max = 30;   // max witness votes

    void validate() const override {
        eosio::check(max > 0, "max witness votes can't be 0");
    }
};

struct witnesses: parameter {
    uint16_t max = 21;   // max witnesses

    void validate() const override {
        eosio::check(max > 0, "max witnesses can't be 0");
    }
};

struct msig_permissions: parameter {
    uint16_t super_majority = 0;    // 0 = auto (2/3+1)
    uint16_t majority = 0;          // 0 = auto (1/2+1)
    uint16_t minority = 0;          // 0 = auto (1/3+1)

    void validate() const override {
        // NOTE: can't compare with max_witnesses here (incl. auto-calculated values), do it in visitor
        eosio::check(!super_majority || !majority || super_majority >= majority, "super_majority must not be less than majority");
        eosio::check(!super_majority || !minority || super_majority >= minority, "super_majority must not be less than minority");
        eosio::check(!majority || !minority || majority >= minority, "majority must not be less than minority");
    }

    uint16_t super_majority_threshold(uint16_t top) const;
    uint16_t majority_threshold(uint16_t top) const;
    uint16_t minority_threshold(uint16_t top) const;
};

} // param

using multisig_acc_param = param_wrapper<param::multisig,1>;
using max_witnesses_param = param_wrapper<param::witnesses,1>;
using msig_perms_param = param_wrapper<param::msig_permissions,3>;
using witness_votes_param = param_wrapper<param::witness_votes,1>;

using ctrl_param = std::variant<multisig_acc_param, max_witnesses_param,
                                msig_perms_param, witness_votes_param>;

struct [[eosio::table]] ctrl_state {
    multisig_acc_param  multisig;
    max_witnesses_param witnesses;
    msig_perms_param    msig_perms;
    witness_votes_param witness_votes;

    static constexpr int params_count = 4;
};
using ctrl_params_singleton = eosio::singleton<"ctrlparams"_n, ctrl_state>;


} // commun
