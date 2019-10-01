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
};

}
