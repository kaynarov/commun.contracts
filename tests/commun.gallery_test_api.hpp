#pragma once
#include "test_api_helper.hpp"
#include <commun.gallery/include/commun.gallery/config.hpp>
#include <commun.list/include/commun.list/config.hpp>

namespace eosio { namespace testing {

struct commun_gallery_api: base_contract_api {
private:

public:
    commun_gallery_api(golos_tester* tester, account_name code, symbol sym)
    :   base_contract_api(tester, code)
    ,   _symbol(sym) {}
    
    symbol _symbol;
    
    action_result createmosaic(account_name creator, uint64_t tracery, account_name opus, asset quantity, uint16_t royalty,
                                        std::vector<std::pair<account_name, int64_t> > providers = 
                                        std::vector<std::pair<account_name, int64_t> >()) {
        auto a = args()
            ("creator", creator)
            ("tracery", tracery)
            ("opus", opus)
            ("quantity", quantity)
            ("royalty", royalty)
            ("providers", providers);
        
        return push(N(createmosaic), creator, a);
    }
    
    action_result addtomosaic(uint64_t tracery, asset quantity, bool damn, account_name gem_creator, 
                                       std::vector<std::pair<account_name, int64_t> > providers = 
                                       std::vector<std::pair<account_name, int64_t> >()) {
        auto a = args()
            ("tracery", tracery)
            ("quantity", quantity)
            ("damn", damn)
            ("gem_creator", gem_creator)
            ("providers", providers);
        
        return push(N(addtomosaic), gem_creator, a);
    }
    
    action_result claim(uint64_t tracery, account_name gem_owner, 
                    account_name gem_creator = account_name(), bool eager = false, account_name signer = account_name()) {
        auto a = args()
            ("tracery", tracery)
            ("commun_code", _symbol.to_symbol_code())
            ("gem_owner", gem_owner);
        if (gem_creator) {
            a("gem_creator", gem_creator);
        }
        if (eager) {
            a("eager", true);
        }
        
        return push(N(claim), signer ? signer : gem_owner, a);
    }

    // TODO: removed from MVP
    // action_result hold(uint64_t tracery, account_name gem_owner, account_name gem_creator = account_name()) {
    //     auto a = args()
    //         ("tracery", tracery)
    //         ("commun_code", _symbol.to_symbol_code())
    //         ("gem_owner", gem_owner);
    //     if (gem_creator) {
    //         a("gem_creator", gem_creator);
    //     }
    //     return push(N(hold), gem_owner, a);
    // }

    // TODO: removed from MVP
    // action_result transfer(uint64_t tracery, account_name gem_owner, 
    //                     account_name gem_creator, account_name recipient, bool recipient_sign = true) {
    //     auto a = args()
    //         ("tracery", tracery)
    //         ("commun_code", _symbol.to_symbol_code())
    //         ("gem_owner", gem_owner);
    //     if (gem_creator) {
    //         a("gem_creator", gem_creator);
    //     }
    //     a("recipient", recipient);
        
    //     return recipient_sign ? 
    //         push_msig(N(transfer), {{gem_owner, config::active_name}, {recipient, config::active_name}}, {gem_owner, recipient}, a) :
    //         push(N(transfer), gem_owner, a);
    // }

    // TODO: removed from MVP
    // action_result provide(account_name grantor, account_name recipient, asset quantity, 
    //                                 std::optional<uint16_t> fee = std::optional<uint16_t>()) {
    //     auto a = args()
    //         ("grantor", grantor)
    //         ("recipient", recipient)
    //         ("quantity", quantity);
    //     if (fee.has_value()) {
    //         a("fee", *fee);
    //     }
    //     return push(N(provide), grantor, a);
    // }

    // TODO: removed from MVP
    // action_result advise(account_name leader, std::vector<uint64_t> favorites) { // vector is to test if duplicated
    //     return push(N(advise), leader, args()
    //         ("commun_code", _symbol.to_symbol_code())
    //         ("leader", leader)
    //         ("favorites", favorites)
    //     );
    // }

    action_result update(account_name creator, uint64_t tracery) {
        return push(N(update), creator, args()
            ("commun_code", _symbol.to_symbol_code())
            ("tracery", tracery)
        );
    }

    action_result lock(account_name leader, uint64_t tracery, const std::string& reason) {
        return push(N(lock), leader, args()
            ("commun_code", _symbol.to_symbol_code())
            ("leader", leader)
            ("tracery", tracery)
            ("reason", reason)
        );
    }

    action_result unlock(account_name leader, uint64_t tracery, const std::string& reason) {
        return push(N(unlock), leader, args()
            ("commun_code", _symbol.to_symbol_code())
            ("leader", leader)
            ("tracery", tracery)
            ("reason", reason)
        );
    }

    action_result ban(account_name issuer, uint64_t tracery) {
        return push(N(ban), issuer, args()
            ("commun_code", _symbol.to_symbol_code())
            ("tracery", tracery)
        );
    }
    
    int64_t get_frozen(account_name acc) {
        auto v = get_struct(acc, N(inclusion), _symbol.to_symbol_code().value, "inclusion");
        if (v.is_object()) {
            auto o = mvo(v);
            return o["quantity"].as<asset>().get_amount();
        }
        return 0;
    }

    const int64_t default_mosaic_pledge = 100;
    const int64_t default_min_mosaic_inclusion = 1000;
    const int64_t default_min_gem_inclusion = 200;
    commun::structures::opus_info default_opus{                                                     \
        eosio::chain::name("regulatum"),
        default_mosaic_pledge,
        default_min_mosaic_inclusion,
        default_min_gem_inclusion
    };
};


}} // eosio::testing
