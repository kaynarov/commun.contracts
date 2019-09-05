#pragma once
#include <common/parameter.hpp>
#include <common/config.hpp>
#include <eosio/singleton.hpp>

namespace golos { namespace param {


using namespace eosio;

struct token: immutable_parameter {
    symbol_code code;

    void validate() const override {
        eosio::check(code.is_valid(), "invalid token");
    }
};

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

struct update_auth_period: parameter {
    uint32_t period = 30*60;

    void validate() const override {
        eosio::check(period > 0, "update auth period can't be 0");
    }
};

} // param

using ctrl_token_param = param_wrapper<param::token,1>;
using multisig_acc_param = param_wrapper<param::multisig,1>;
using max_witnesses_param = param_wrapper<param::witnesses,1>;
using msig_perms_param = param_wrapper<param::msig_permissions,3>;
using witness_votes_param = param_wrapper<param::witness_votes,1>;
using update_auth_param = param_wrapper<param::update_auth_period,1>;

using ctrl_param = std::variant<ctrl_token_param, multisig_acc_param, max_witnesses_param,
                                msig_perms_param, witness_votes_param, update_auth_param>;

struct [[eosio::table]] ctrl_state {
    ctrl_token_param    token;
    multisig_acc_param  multisig;
    max_witnesses_param witnesses;
    msig_perms_param    msig_perms;
    witness_votes_param witness_votes;
    update_auth_param   update_auth_period;

    static constexpr int params_count = 6;
};
using ctrl_params_singleton = eosio::singleton<"ctrlparams"_n, ctrl_state>;


} // golos
