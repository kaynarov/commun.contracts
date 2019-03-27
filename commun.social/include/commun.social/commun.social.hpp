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

    [[eosio::action]] void createreput(name account);
    [[eosio::action]] void changereput(name voter, name author, int64_t rshares);

    [[eosio::action]] void updatemeta(std::string avatar_url, std::string cover_url, 
                                      std::string biography, std::string facebook, 
                                      std::string telegram, std::string whatsapp, 
                                      std::string wechat);
    [[eosio::action]] void deletemeta(name account);

    [[eosio::action]] void deletereput(name account);

    static inline bool is_blocking(name code, name blocker, name blocking) {
        tables::pinblock_table table(code, blocker.value);
        auto itr = table.find(blocking.value);
        return (itr != table.end() && itr->blocking);
    }

private:
    bool record_is_empty(structures::pinblock_record record);
};

}
