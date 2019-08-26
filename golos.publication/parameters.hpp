#pragma once
#include <eosio/singleton.hpp>
#include <commun/parameter.hpp>
#include <commun/parameter_ops.hpp>
#include <commun/config.hpp>

namespace golos {

    using namespace eosio;
    using namespace commun;

    struct st_max_comment_depth : parameter {
        uint16_t max_comment_depth;

        void validate() const override {
            eosio::check(max_comment_depth > 0, "Max comment depth must be greater than 0.");
        }
    };
    using max_comment_depth_prm = param_wrapper<st_max_comment_depth, 1>;

    struct st_social_acc : parameter {
        name account;

        void validate() const override {
            if (account != name()) {
                eosio::check(is_account(account), "Social account doesn't exist.");
            }
        }
    };
    using social_acc_prm = param_wrapper<st_social_acc, 1>;

    using posting_params = std::variant<max_comment_depth_prm, social_acc_prm>;

    struct [[eosio::table]] posting_state {
        max_comment_depth_prm max_comment_depth_param;
        social_acc_prm social_acc_param;

        static constexpr int params_count = 2;
    };
    using posting_params_singleton = eosio::singleton<"pstngparams"_n, posting_state>;

} //golos
