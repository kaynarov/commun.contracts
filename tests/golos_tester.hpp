#pragma once
#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <fc/variant_object.hpp>

#define UNIT_TEST_ENV

// Note: cannot check nested mvo
#define CHECK_EQUAL_OBJECTS(left, right) { \
    auto l = fc::variant(left); \
    auto r = fc::variant(right); \
    BOOST_TEST_CHECK(l.is_object()); \
    BOOST_TEST_CHECK(r.is_object()); \
    if (l.is_object() && r.is_object()) { \
        BOOST_CHECK_EQUAL_COLLECTIONS(l.get_object().begin(), l.get_object().end(), r.get_object().begin(), r.get_object().end()); }}

#define CHECK_MATCHING_OBJECT(check, reference) { \
    auto a = fc::variant(reference); \
    auto b = fc::variant(check); \
    BOOST_TEST_CHECK(a.is_object()); \
    BOOST_TEST_CHECK(b.is_object()); \
    if (a.is_object() && b.is_object()) { \
        auto filtered = ::eosio::testing::filter_fields(a.get_object(), b.get_object()); \
        BOOST_CHECK_EQUAL_COLLECTIONS(a.get_object().begin(), a.get_object().end(), filtered.begin(), filtered.end()); }}

namespace eosio { namespace testing {

uint64_t hash64(const std::string& arg);

// TODO: maybe use native db struct
struct permission {
    name        perm_name;
    name        parent;
    authority   required_auth;
};

struct contract_error_messages {
protected:
    const string amsg(const string& x) { return base_tester::wasm_assert_msg(x); }
};


class golos_tester : public tester {
protected:
    const name _code;     // shortcut to main tested contract account name

    cyberway::chaindb::chaindb_controller& _chaindb;
    std::map<account_name, abi_serializer> _abis;

public:
    golos_tester(name code): tester(), _code(code), _chaindb(control->chaindb()) {
    }
    ~golos_tester() {
    }

    void install_contract(account_name acc, const std::vector<uint8_t>& wasm, const std::vector<char>& abi, bool produce = true);

    std::vector<permission> get_account_permissions(account_name a);

    action_result push_and_check_action(account_name, action_name, account_name, const variant_object&);
    action_result push_action(account_name code, action_name name, account_name signer, const variant_object& data);
    action_result push_action_msig_tx(account_name code, action_name name,
        std::vector<permission_level> perms, std::vector<account_name> signers, const variant_object& data);
    action_result push_tx(signed_transaction&& tx);

    template<typename Key>
    fc::variant get_chaindb_lower_bound_struct(name code, uint64_t scope, name tbl, name indx, const Key& key, const std::string& n) const {
        variant r;
        try {
            bytes data = fc::raw::pack(key);
            const auto& finfo = _chaindb.lower_bound({code, scope, tbl, indx}, data.data(), data.size());
            r = _chaindb.value_at_cursor({code, finfo.cursor});
        } catch (...) {
            // key not found
        }
        return r;
    }

    fc::variant get_chaindb_struct(name code, uint64_t scope, name tbl, uint64_t id, const std::string& n) const;
    fc::variant get_chaindb_singleton(name code, uint64_t scope, name tbl, const std::string& n) const;
    std::vector<fc::variant> get_all_chaindb_rows(name code, uint64_t scope, name tbl, bool strict) const;
};


}} // eosio::tesing

FC_REFLECT(eosio::testing::permission, (perm_name)(parent)(required_auth))
