#pragma once
#include "test_api_helper.hpp"
#include <commun.gallery/include/commun.gallery/config.hpp>

using mosaic_key_t = std::tuple<cyberway::chain::account_name, uint64_t>;

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
    
    action_result createmosaic(account_name creator, uint64_t tracery, asset quantity, uint16_t royalty,
                                        std::optional<std::vector<std::pair<account_name, int64_t> > > providers = 
                                        std::optional<std::vector<std::pair<account_name, int64_t> > >()) {
        auto a = args()
            ("creator", creator)
            ("tracery", tracery)
            ("quantity", quantity)
            ("royalty", royalty);
        if (providers.has_value()) {
            a("providers", *providers);
        }
        
        return push(N(createmosaic), creator, a);
    }
    
    action_result addtomosaic(account_name mosaic_creator, uint64_t tracery, asset quantity, bool damn, account_name gem_creator, 
                                       std::optional<std::vector<std::pair<account_name, int64_t> > > providers = 
                                       std::optional<std::vector<std::pair<account_name, int64_t> > >()) {
        auto a = args()
            ("mosaic_creator", mosaic_creator)
            ("tracery", tracery)
            ("quantity", quantity)
            ("damn", damn)
            ("gem_creator", gem_creator);
        if (providers.has_value()) {
            a("providers", *providers);
        }
        
        return push(N(addtomosaic), gem_creator, a);
    }
      
    action_result claimgem(account_name mosaic_creator, uint64_t tracery, symbol_code commun_code, account_name gem_owner, 
                    std::optional<account_name> gem_creator = std::optional<account_name>(), 
                    std::optional<account_name> recipient = std::optional<account_name>(), 
                    account_name signer = account_name()) {
        auto a = args()
            ("mosaic_creator", mosaic_creator)
            ("tracery", tracery)
            ("commun_code", commun_code)
            ("gem_owner", gem_owner);
        if (gem_creator.has_value()) {
            a("gem_creator", *gem_creator);
        }
        if (recipient.has_value()) {
            a("recipient", *recipient);
        }
        
        return push(N(claimgem), signer ? signer : gem_owner, a);
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

    action_result advise(symbol_code commun_code, account_name leader, std::vector<mosaic_key_t> favorites) {
        return push(N(advise), leader, args()
            ("commun_code", commun_code)
            ("leader", leader)
            ("favorites", favorites));
    }
    
    action_result slap(symbol_code commun_code, account_name leader, account_name mosaic_creator, uint64_t tracery) {
        return push(N(advise), leader, args()
            ("commun_code", commun_code)
            ("leader", leader)
            ("mosaic_creator", mosaic_creator)
            ("tracery", tracery));
    }
};


}} // eosio::testing
