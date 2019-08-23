#pragma once
#include "objects.hpp"
#include "parameters.hpp"
#include <eosio/transaction.hpp>

namespace golos {

using namespace eosio;

class publication : public contract {
public:
    using contract::contract;

    void create_message(structures::mssgid message_id, structures::mssgid parent_id,
        std::string headermssg, std::string bodymssg, std::string languagemssg, std::vector<std::string> tags,
        std::string jsonmetadata);
    void update_message(structures::mssgid message_id, std::string headermssg, std::string bodymssg,
                        std::string languagemssg, std::vector<std::string> tags, std::string jsonmetadata);
    void delete_message(structures::mssgid message_id);
    void upvote(name voter, structures::mssgid message_id, uint16_t weight);
    void downvote(name voter, structures::mssgid message_id, uint16_t weight);
    void unvote(name voter, structures::mssgid message_id);
    void set_params(std::vector<posting_params> params);
    void reblog(name rebloger, structures::mssgid message_id, std::string headermssg, std::string bodymssg);
    void erase_reblog(name rebloger, structures::mssgid message_id);
private:
    const posting_state& params();
    void set_vote(name voter, const structures::mssgid &message_id, int16_t weight);
    bool validate_permlink(std::string permlink);
};

} // golos
