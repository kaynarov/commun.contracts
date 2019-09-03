#pragma once
#include "objects.hpp"
#include "parameters.hpp"
#include <eosio/transaction.hpp>

namespace commun {

using namespace eosio;

class publication : public gallery_base, public contract {
    
public:
    using contract::contract;

    void create_message(symbol_code commun_code, mssgid_t message_id, mssgid_t parent_id,
        std::string headermssg, std::string bodymssg, std::string languagemssg, std::vector<std::string> tags, std::string jsonmetadata,
        uint16_t curators_prcnt);
    void update_message(symbol_code commun_code, mssgid_t message_id, std::string headermssg, std::string bodymssg,
                        std::string languagemssg, std::vector<std::string> tags, std::string jsonmetadata);
    void delete_message(symbol_code commun_code, mssgid_t message_id);
    void upvote(symbol_code commun_code, name voter, mssgid_t message_id, uint16_t weight);
    void downvote(symbol_code commun_code, name voter, mssgid_t message_id, uint16_t weight);
    void unvote(symbol_code commun_code, name voter, mssgid_t message_id);
    void claim(name mosaic_creator, uint64_t tracery, symbol_code commun_code, name gem_owner, 
                    std::optional<name> gem_creator, std::optional<name> recipient, std::optional<bool> eager);

    void set_params(symbol_code commun_code, std::vector<posting_params> params);
    void reblog(symbol_code commun_code, name rebloger, mssgid_t message_id, std::string headermssg, std::string bodymssg);
    void erase_reblog(symbol_code commun_code, name rebloger, mssgid_t message_id);
    void setproviders(symbol_code commun_code, name recipient, std::vector<name> providers);
    void setfrequency(symbol_code commun_code, name account, uint16_t actions_per_day);
    
    void provide(name grantor, name recipient, asset quantity, std::optional<uint16_t> fee);
    void advise(symbol_code commun_code, name leader, std::vector<mosaic_key_t> favorites);
    //TODO: void checkadvice (symbol_code commun_code, name leader);
    void slap(symbol_code commun_code, name leader, name mosaic_creator, uint64_t tracery);
private:
    gallery_base::opt_providers_t get_providers(symbol_code commun_code, name account, uint16_t weight = config::_100percent);
    accparams::const_iterator get_acc_param(accparams& accparams_table, symbol_code commun_code, name account);
    static int64_t get_amount_to_freeze(int64_t balance, int64_t frozen, uint16_t actions_per_day, int64_t mosaic_active_period, 
                                        int64_t actual_limit = std::numeric_limits<int64_t>::max());
    const posting_state& params(symbol_code commun_code);
    void set_vote(symbol_code commun_code, name voter, const mssgid_t &message_id, int16_t weight);
    bool validate_permlink(std::string permlink);
};

} // commun
