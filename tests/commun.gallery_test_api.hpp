#pragma once
#include "test_api_helper.hpp"
#include <commun.gallery/include/commun.gallery/config.hpp>

using mosaic_key_t = std::pair<cyberway::chain::account_name, uint64_t>;

namespace eosio { namespace testing {


struct commun_gallery_api: base_contract_api {
private:

public:
    commun_gallery_api(golos_tester* tester, account_name code, symbol sym)
    :   base_contract_api(tester, code)
    ,   _symbol(sym) {}
    
    symbol _symbol;
    
    action_result create(symbol commun_symbol) {
        return push(N(create), _code, args()("commun_symbol", commun_symbol));
    }
    
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
    
    action_result addtomosaic(account_name mosaic_creator, uint64_t tracery, asset quantity, bool damn, account_name gem_creator, 
                                       std::vector<std::pair<account_name, int64_t> > providers = 
                                       std::vector<std::pair<account_name, int64_t> >()) {
        auto a = args()
            ("mosaic_creator", mosaic_creator)
            ("tracery", tracery)
            ("quantity", quantity)
            ("damn", damn)
            ("gem_creator", gem_creator)
            ("providers", providers);
        
        return push(N(addtomosaic), gem_creator, a);
    }
      
    action_result claim(account_name mosaic_creator, uint64_t tracery, account_name gem_owner, 
                    account_name gem_creator = account_name(), bool eager = false, account_name signer = account_name()) {
        auto a = args()
            ("mosaic_creator", mosaic_creator)
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
    
    action_result hold(account_name mosaic_creator, uint64_t tracery, account_name gem_owner, account_name gem_creator = account_name()) {
        auto a = args()
            ("mosaic_creator", mosaic_creator)
            ("tracery", tracery)
            ("commun_code", _symbol.to_symbol_code())
            ("gem_owner", gem_owner);
        if (gem_creator) {
            a("gem_creator", gem_creator);
        }
        return push(N(hold), gem_owner, a);
    }
    
    action_result transfer(account_name mosaic_creator, uint64_t tracery, account_name gem_owner, 
                        account_name gem_creator, account_name recipient, bool recipient_sign = true) {
        auto a = args()
            ("mosaic_creator", mosaic_creator)
            ("tracery", tracery)
            ("commun_code", _symbol.to_symbol_code())
            ("gem_owner", gem_owner);
        if (gem_creator) {
            a("gem_creator", gem_creator);
        }
        a("recipient", recipient);
        
        return recipient_sign ? 
            push_msig(N(transfer), {{gem_owner, config::active_name}, {recipient, config::active_name}}, {gem_owner, recipient}, a) :
            push(N(transfer), gem_owner, a);
    }
    
    action_result provide(account_name grantor, account_name recipient, asset quantity, 
                                    std::optional<uint16_t> fee = std::optional<uint16_t>()) {
        auto a = args()
            ("grantor", grantor)
            ("recipient", recipient)
            ("quantity", quantity);
        if (fee.has_value()) {
            a("fee", *fee);
        }
        return push(N(provide), grantor, a);
    }

    action_result advise(account_name leader, std::vector<mosaic_key_t> favorites) {
        return push(N(advise), leader, args()
            ("commun_code", _symbol.to_symbol_code())
            ("leader", leader)
            ("favorites", favorites));
    }
    
    action_result slap(account_name leader, account_name mosaic_creator, uint64_t tracery) {
        return push(N(slap), leader, args()
            ("commun_code", _symbol.to_symbol_code())
            ("leader", leader)
            ("mosaic_creator", mosaic_creator)
            ("tracery", tracery));
    }
    
    int64_t get_frozen(account_name acc) {
        auto v = get_struct(acc, N(inclusion), _symbol.to_symbol_code().value, "inclusion");
        if (v.is_object()) {
            auto o = mvo(v);
            return o["quantity"].as<asset>().get_amount();
        }
        return 0;
    }
};


}} // eosio::testing
