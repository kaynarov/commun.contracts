#include "objects.hpp"

namespace commun {

using namespace eosio;

class commun_social: public contract {
public:
    using contract::contract;

    [[eosio::action]] void pin(name pinner, name pinning);
    [[eosio::action]] void unpin(name pinner, name pinning);

    [[eosio::action]] void block(name blocker, name blocking);
    [[eosio::action]] void unblock(name blocker, name blocking);

    [[eosio::action]] void updatemeta(name account,
        std::string avatar_url, std::string cover_url, std::string biography,
        std::string facebook, std::string telegram, std::string whatsapp, std::string wechat);
    [[eosio::action]] void deletemeta(name account);
};

}
