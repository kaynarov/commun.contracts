#include <commun.list.hpp>
#include <commun.token.hpp>

using namespace commun;

void commun_list::create(symbol symbol, name token, name ctrl, name vesting, name emit,
                         name charge, name posting, name social, name referral) {
    require_auth(token);

    auto issuer = commun::token::get_issuer(token, symbol.code()); // TODO verification of validity of a token

    tables::community community_tbl(_self, _self.value);
    auto community_index = community_tbl.get_index<"bytoken"_n>();
    eosio_assert(community_index.find({symbol.raw(), token.value}) == community_index.end(), "community exists");

    community_tbl.emplace(token, [&](auto& item) {
        item.id = community_tbl.available_primary_key();
        item.symbol = symbol;
        item.ctrl = ctrl;
        item.emit = emit;
        item.token = token;
        item.charge = charge;
        item.social = social;
        item.vesting = vesting;
        item.posting = posting;
        item.referral = referral;
    });
}

void commun_list::update(symbol symbol, name token, name ctrl, name vesting, name emit,
                         name charge, name posting, name social, name referral) {
    require_auth(token);

    auto issuer = commun::token::get_issuer(token, symbol.code()); // TODO verification of validity of a token

    tables::community community_tbl(_self, _self.value);
    auto community_index = community_tbl.get_index<"bytoken"_n>();
    auto community_point = community_index.find({symbol.raw(), token.value});
    eosio_assert(community_point != community_index.end(), "community not exists");

    community_index.modify(community_point, token, [&](auto& item) {
        item.ctrl = ctrl;
        item.emit = emit;
        item.charge = charge;
        item.social = social;
        item.vesting = vesting;
        item.posting = posting;
        item.referral = referral;
    });
}

EOSIO_DISPATCH(commun::commun_list, (create)(update))
