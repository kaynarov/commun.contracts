#include "golos_tester.hpp"
#include <eosio/chain/permission_object.hpp>

namespace eosio { namespace testing {

using std::vector;
using std::string;

uint64_t hash64(const std::string& s) {
    return fc::sha256::hash(s.c_str(), s.size())._hash[0];
}

authority create_code_authority(const std::vector<name> contracts) {
    authority auth(1, {});
    for (const auto contract: contracts) {
        auth.accounts.push_back({.permission = {contract, config::eosio_code_name}, .weight = 1});
    }
    return auth;
}

void golos_tester::install_contract(
    account_name acc, const vector<uint8_t>& wasm, const vector<char>& abi, bool produce, const private_key_type* signer
) {
    set_code(acc, wasm, signer);
    set_abi (acc, abi.data(), signer);
    if (produce)
        produce_block();
    const auto& accnt = _chaindb.get<account_object>(acc);
    abi_def abi_d;
    BOOST_CHECK_EQUAL(abi_serializer::to_abi(accnt.abi, abi_d), true);
    _abis[acc].set_abi(abi_d, abi_serializer_max_time);
};

vector<permission> golos_tester::get_account_permissions(account_name a) {
    vector<permission> r;
    auto table = _chaindb.get_table<permission_object>();
    auto perms = table.get_index<by_owner>();
    auto perm = perms.lower_bound(boost::make_tuple(a));
    while (perm != perms.end() && perm->owner == a) {
        name parent;
        if (perm->parent._id) {
            const auto* p = _chaindb.find<permission_object,by_id>(perm->parent);
            if (p) {
                parent = p->name;
            }
        }
        r.push_back(permission{perm->name, parent, perm->auth.to_authority()});
        ++perm;
    }
    return r;
}

// this method do produce_block() internaly and checks that chain_has_transaction()
base_tester::action_result golos_tester::push_and_check_action(
    account_name code,
    action_name name,
    account_name signer,
    const variant_object& data
) {
    auto& abi = _abis[code];
    action act;
    act.account = code;
    act.name    = name;
    act.data    = abi.variant_to_binary(abi.get_action_type(name), data, abi_serializer_max_time);
    return base_tester::push_action(std::move(act), uint64_t(signer));
}

base_tester::action_result golos_tester::push_action(
    account_name code,
    action_name name,
    account_name signer,
    const variant_object& data
) {
    try {
        base_tester::push_action(code, name, signer, data);
    } catch (const fc::exception& ex) {
        edump((ex.to_detail_string()));
        return error(ex.top_message());
    }
    return success();
}

base_tester::action_result golos_tester::push_action_msig_tx(
    account_name code,
    action_name name,
    vector<permission_level> perms,
    vector<account_name> signers,
    const variant_object& data,
    bool produce/*=true*/
) {
    auto& abi = _abis[code];
    action act;
    act.account = code;
    act.name    = name;
    act.data    = abi.variant_to_binary(abi.get_action_type(name), data, abi_serializer_max_time);
    for (const auto& perm : perms) {
        act.authorization.emplace_back(perm);
    }

    signed_transaction tx;
    tx.actions.emplace_back(std::move(act));
    set_transaction_headers(tx);
    for (const auto& a : signers) {
        tx.sign(get_private_key(a, "active"), control->get_chain_id());
    }
    return push_tx(std::move(tx), produce);
}

base_tester::action_result golos_tester::push_tx(signed_transaction&& tx, bool produce_and_check) {
    try {
        push_transaction(tx);
    } catch (const fc::exception& ex) {
        edump((ex.to_detail_string()));
        return error(ex.top_message()); // top_message() is assumed by many tests; otherwise they fail
        //return error(ex.to_detail_string());
    }
    if (produce_and_check) {
        produce_block();
        BOOST_REQUIRE_EQUAL(true, chain_has_transaction(tx.id()));
    }
    return success();
}

base_tester::action_result golos_tester::push_tx(const std::vector<action_data> actions, const std::vector<account_name> signers, bool produce_and_check) {
    signed_transaction tx;
    for(const auto& act_data : actions) {
        auto& abi = _abis[act_data.code];
        action act;
        act.account = act_data.code;
        act.name    = act_data.action;
        act.data    = abi.variant_to_binary(abi.get_action_type(act_data.action), act_data.data, abi_serializer_max_time);
        for (const auto& perm : act_data.perms) {
            act.authorization.emplace_back(perm);
        }
        tx.actions.emplace_back(std::move(act));
    }
    set_transaction_headers(tx);
    for (const auto& a : signers) {
        tx.sign(get_private_key(a, "active"), control->get_chain_id());
    }
    return push_tx(std::move(tx), produce_and_check);
}

variant golos_tester::get_chaindb_struct(name code, uint64_t scope, name tbl, uint64_t id,
    const string& n
) const {
    variant r;
    try {
        r = _chaindb.object_by_pk({code, scope, tbl}, id).value;
    } catch (...) {
        // key not found
    }
    return r;
}

variant golos_tester::get_chaindb_singleton(name code, uint64_t scope, name tbl,
    const string& n
) const {
    return get_chaindb_struct(code, scope, tbl, tbl, n);
}

vector<variant> golos_tester::get_all_chaindb_rows(name code, uint64_t scope, name tbl, bool strict) const {
    vector<variant> all;
    auto info = _chaindb.lower_bound({code, scope, tbl, N(primary)}, cyberway::chaindb::cursor_kind::ManyRecords,nullptr, 0);
    cyberway::chaindb::cursor_request cursor = {code, info.cursor};
    auto v = _chaindb.object_at_cursor(cursor).value;
    if (strict) {
        BOOST_TEST_REQUIRE(!v.is_null());
    } else if (v.is_null()) {
        return all;
    }

    auto prev = _chaindb.current(cursor);
    do {
        all.push_back(v);
        auto pk = _chaindb.next(cursor);
        if (pk == cyberway::chaindb::primary_key::End) {
            break;
        }
        prev = pk;
        v = _chaindb.object_at_cursor(cursor).value;
    } while (!v.is_null());
    return all;
}

void golos_tester::delegate_authority(account_name from, std::vector<account_name> to,
        account_name code, action_name type, permission_name req,
        permission_name parent, permission_name prov) {

    authority auth(1, {});
    for (auto u : to) {
        auth.accounts.emplace_back(permission_level_weight{.permission = {u, prov}, .weight = 1});
    }
    std::sort(auth.accounts.begin(), auth.accounts.end(),
        [](const permission_level_weight& l, const permission_level_weight& r) {
            return std::tie(l.permission.actor, l.permission.permission) <
                std::tie(r.permission.actor, r.permission.permission);
        });
    set_authority(from, req, auth, parent, { { from, config::active_name } }, {
        eosio::testing::base_tester::get_private_key(from, name{config::active_name}.to_string())
    });
    link_authority(from, code, req, type);
}

signed_block_ptr golos_tester::wait_block(const uint32_t n) {
    BOOST_TEST_REQUIRE(control->head_block_num() <= n);
    auto b = control->head_block_state()->block;
    while (control->head_block_num() < n) {
        b = produce_block();
    }
    return b;
}

}} // eosio::tesing
