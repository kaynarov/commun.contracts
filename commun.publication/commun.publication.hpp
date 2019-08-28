#pragma once
#include "objects.hpp"
#include "parameters.hpp"
#include <eosio/transaction.hpp>

namespace commun {

using namespace eosio;

class publication : public contract {
    
public:
    using contract::contract;

    void create_message(symbol_code commun_code, structures::mssgid message_id, structures::mssgid parent_id,
        std::string headermssg, std::string bodymssg, std::string languagemssg, std::vector<std::string> tags,
        std::string jsonmetadata);
    void update_message(symbol_code commun_code, structures::mssgid message_id, std::string headermssg, std::string bodymssg,
                        std::string languagemssg, std::vector<std::string> tags, std::string jsonmetadata);
    void delete_message(symbol_code commun_code, structures::mssgid message_id);
    void upvote(symbol_code commun_code, name voter, structures::mssgid message_id, uint16_t weight);
    void downvote(symbol_code commun_code, name voter, structures::mssgid message_id, uint16_t weight);
    void unvote(symbol_code commun_code, name voter, structures::mssgid message_id);
    void set_params(symbol_code commun_code, std::vector<posting_params> params);
    void reblog(symbol_code commun_code, name rebloger, structures::mssgid message_id, std::string headermssg, std::string bodymssg);
    void erase_reblog(symbol_code commun_code, name rebloger, structures::mssgid message_id);
private:
    const posting_state& params(symbol_code commun_code);
    void set_vote(symbol_code commun_code, name voter, const structures::mssgid &message_id, int16_t weight);
    bool validate_permlink(std::string permlink);
};

} // commun
