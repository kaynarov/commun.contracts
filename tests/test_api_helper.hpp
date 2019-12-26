#pragma once
#include "golos_tester.hpp"
#include <boost/algorithm/string/replace.hpp>

namespace eosio { namespace testing {

using mvo = fc::mutable_variant_object;
using action_result = base_tester::action_result;

// allows to use ' quotes instead of ", but will break if ' used not as quote
inline fc::variant json_str_to_obj(std::string s) {
    return fc::json::from_string(boost::replace_all_copy(s, "'", "\""));
}

inline account_name user_name(size_t n) {
    string ret_str("user");
    while (n) {
        constexpr auto q = 'z' - 'a' + 1;
        ret_str += std::string(1, 'a' + n % q);
        n /= q;
    }
    return string_to_name(ret_str.c_str());
}

struct base_contract_api {
    using signers_t = std::vector<account_name>;

    golos_tester* _tester;
    name _code;

    base_contract_api(golos_tester* tester, name code): _tester(tester), _code(code) {}

    action_result push(action_name name, account_name signer, const variant_object& data) {
        return _tester->push_action(_code, name, signer, data);
    }

    base_tester::action_result push_msig(action_name name, signers_t signers, const variant_object& data) {
        std::vector<permission_level> perms{signers.size()};
        std::transform(signers.begin(), signers.end(), perms.begin(), [](const auto& s){
            return permission_level{s, config::active_name};
        });
        return _tester->push_action_msig_tx(_code, name, perms, signers, data, false); // NOTE: doesn't produce block
    }

    base_tester::action_result push_msig(action_name name, std::vector<permission_level> perms, signers_t signers,
        const variant_object& data
    ) {
        return _tester->push_action_msig_tx(_code, name, perms, signers, data); // NOTE: produces block; TODO: add push_msig_produce?
    }

    variant get_struct(uint64_t scope, name tbl, uint64_t id, const string& name) const {
        return _tester->get_chaindb_struct(_code, scope, tbl, id, name);
    }

    variant get_singleton(uint64_t scope, name tbl, const string& name) const {
        return _tester->get_chaindb_singleton(_code, scope, tbl, name);
    }

    virtual mvo args() {
        return mvo();
    }

};


} }
