#pragma once
#include <eosio/singleton.hpp>
#include <commun/parameter.hpp>
#include <commun/parameter_ops.hpp>
#include <commun/config.hpp>

namespace commun {

    using namespace eosio;

    struct st_max_comment_depth : parameter {
        uint16_t max_comment_depth;

        void validate() const override {
            eosio::check(max_comment_depth > 0, "Max comment depth must be greater than 0.");
        }
    };
    using max_comment_depth_prm = param_wrapper<st_max_comment_depth, 1>;

    using posting_params = std::variant<max_comment_depth_prm>;

    struct [[eosio::table]] posting_state {
        max_comment_depth_prm max_comment_depth_param;

        static constexpr int params_count = 1;
    };
    using posting_params_singleton = eosio::singleton<"pstngparams"_n, posting_state>;

} //commun
