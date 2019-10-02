#include "objects.hpp"

namespace commun {

using namespace eosio;

class commun_list: public contract {
public:
    using contract::contract;

    [[eosio::action]] void create(symbol_code commun_code, std::string community_name);

    [[eosio::action]] void addinfo(symbol_code commun_code, std::string community_name, 
                                   std::string ticker, std::string avatar, std::string cover_img_link, 
                                   std::string description, std::string rules);

    static inline auto get_community(name list_contract_account, symbol_code commun_code) {
        tables::community community_tbl(list_contract_account, list_contract_account.value);
        return community_tbl.get(commun_code.raw(), "community not exists");
    }
};

}
