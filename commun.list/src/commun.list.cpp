#include <commun.list.hpp>
#include <bancor.token/bancor.token.hpp>
#include <golos.vesting/golos.vesting.hpp>
#include <commun/config.hpp>

using namespace commun;

void commun_list::create(std::string community_name, symbol_code token_name,
                         structures::community_contracts contracts) {

    eosio_assert(commun::bancor::exist(config::bancor_name, token_name), "not found token");
    eosio_assert(golos::vesting::exists(config::vesting_name, token_name), "not found vesting token");

    require_auth(_self);

    tables::community community_tbl(_self, _self.value);

    eosio_assert(community_tbl.find(token_name.raw()) == community_tbl.end(), "community token exists");

    auto community_index = community_tbl.get_index<"byname"_n>();
    eosio_assert(community_index.find(community_name) == community_index.end(), "community exists");

    community_tbl.emplace(_self, [&](auto& item) {
        item.token_name = token_name;
        item.community_name = community_name;
        item.contracts = contracts;
    });
}

EOSIO_DISPATCH(commun::commun_list, (create))
