#pragma once
#include <commun.publication/objects.hpp>
#include <eosio/transaction.hpp>
#include <commun/dispatchers.hpp>

namespace commun {

using namespace eosio;

/**
 * \brief This class implements \a c.gallery contract behaviour
 * \ingroup gallery_class
 */
class
/// @cond
[[eosio::contract("commun.publication")]]
/// @endcond
publication : public gallery_base<publication>, public contract {

public:
    using contract::contract;
    
    static bool can_remove_vertex(const vertex_struct& arg, const gallery_types::mosaic_struct& mosaic, const structures::community& community) {
        auto now = eosio::current_time_point();
        return arg.childcount == 0
            || now > (mosaic.collection_end_date + eosio::seconds(community.moderation_period + community.extra_reward_period));
    }

    static void deactivate(name self, symbol_code commun_code, const gallery_types::mosaic_struct& mosaic) {
        vertices vertices_table(self, commun_code.raw());
        auto vertex = vertices_table.find(mosaic.tracery);
        eosio::check(vertex != vertices_table.end(), "SYSTEM: Permlink doesn't exist.");
        eosio::check(can_remove_vertex(*vertex, mosaic, commun_list::get_community(commun_code)), "comment with child comments can't be removed during the active period");

        if (vertex->parent_tracery) {
            auto parent_vertex = vertices_table.find(vertex->parent_tracery);
            if (parent_vertex != vertices_table.end()) {
                vertices_table.modify(parent_vertex, eosio::same_payer, [&](auto& item) { item.childcount--; });
            }
        }
        vertices_table.erase(*vertex);
    }

    /**
        \brief The \ref init action is used by \a c.list contract to configure the gallery parameters for a community specified by a point symbol.
        \param commun_code community symbol, same as point symbol

        This action is unavailable for user and can be called only internally.
        \signreq
            — the \a c.list contract account .
    */
    [[eosio::action]] void init(symbol_code commun_code);

    /**
        \brief The \ref emit action is used to emit points specified by parameter for rewarding community members. Points are issued directly in \a c.emit contract, not in \a c.gallery one.
        \param commun_code community symbol, same as point symbol

        This action is unavailable for user and can be called only internally.
        \signreq
            — <i>trusted community client</i> .
    */
    [[eosio::action]] void emit(symbol_code commun_code);

    /**
        \brief The \ref create action is used by a user to create a message.

        \param commun_code community symbol, same as point symbol
        \param message_id identifier of the message (post or comment) to be created
        \param parent_id identifier of the parent message. The field \a parent_id.author should be empty if a post is created
        \param header title of the message (not stored in DB, only checked). The length must not exceed 256 characters
        \param body body of the message (not stored in DB, only checked). The body must not be empty
        \param tags list of tags of the message (not stored in DB)
        \param metadata metadata of the message (recommended format is JSON. The data is not stored in DB)
        \param weight weight of points «frozen» for creating the message. This parameter is optional and specified only when creating a post

        \signreq
            — account from the field \a message_id.author ;
            — <i>trusted community client</i> .
    */
    [[eosio::action]] void create(symbol_code commun_code, mssgid message_id, mssgid parent_id,
        std::string header, std::string body, std::vector<std::string> tags, std::string metadata,
        std::optional<uint16_t> weight);

    /**
        \brief The \ref update action is used to edit a message sent previously.

        \param commun_code point symbol of community
        \param message_id identifier of the message (post or comment) being edited
        \param header title of the message (not stored in DB)
        \param body body of the message (not stored in DB)
        \param tags list of tags of the message (not stored in DB)
        \param metadata metadata of the message (recommended format is JSON. The data is not stored in DB)

        \signreq
            — account from the field \a message_id.author ;
            — <i>trusted community client</i> .

        \note
        Community leaders can lock a message even after it has been corrected regardless of whether it was previously locked temporarily or not. However, the message can not be locked again if the \ref update is also signed by the trusted community client because the message is not checked by leaders in this case.
    */
    [[eosio::action]] void update(symbol_code commun_code, mssgid message_id, std::string header, std::string body,
        std::vector<std::string> tags, std::string metadata);

    /**
        \brief The \ref settags action is used by a leader to update the message tags.

        \param commun_code community symbol, same as point symbol
        \param leader account of the leader
        \param message_id identifier of the message (post or comment) whose tags are being updated
        \param add_tags list of new tags to be added to the message
        \param remove_tags list of tags to be removed from the message
        \param reason reason of tags change

        The \ref settags action does not store any data in DB, it only checks the input parameters.


        \signreq
            — \a leader ;  
            — <i>trusted community client</i> (optional) .

        \note
        Changes to tags of a message are not checked by leaders if the \ref settags action is signed by the <i>trusted community client</i>.
    */
    [[eosio::action]] void settags(symbol_code commun_code, name leader, mssgid message_id,
        std::vector<std::string> add_tags, std::vector<std::string> remove_tags, std::string reason);

    /**
        \brief The \ref remove action is used by message author to remove the message.

        \param commun_code community symbol, same as point symbol
        \param message_id identifier of the message (post or comment) to be removed

        \signreq
            — \a author of message ;  
            — <i>trusted community client</i> (optional) .

        \note
        Deleting a message and updating the corresponding records in DB are not checked if the \ref remove action is also signed by the <i>trusted community client</i>.
    */
    [[eosio::action]] void remove(symbol_code commun_code, mssgid message_id);

    /**
        \brief The \ref report action is used by a user to notify community leaders about undesirable or suspicious message content.

        \param commun_code community symbol, same as point symbol
        \param reporter account of user who reports about suspicious message content to leaders
        \param message_id identifier of the message (post or comment) whose content is considered undesirable
        \param reason reason of reporting (this field should not be empty)

        The action does not store any data in DB, it only checks the input parameters.
        It can only be performed if the message state is active, that is, user opinions are collected.

        \signreq
            — the \a reporter account ;
            — <i>trusted community client</i> .

        \note
        A message content is not checked for suspicion by leaders if the action is also signed by the <i>trusted community client</i>.
    */
    [[eosio::action]] void report(symbol_code commun_code, name reporter, mssgid message_id, std::string reason);

    /**
        \brief The \ref lock action is used by a leader to temporarily lock a message that is considered undesirable.

        \param commun_code community symbol, same as point symbol
        \param leader account of leader who is locking the message
        \param message_id identifier of the message (post or comment) to be locked
        \param reason reason of locking (this field should not be empty)

        This action stops receiving a reward allocated for the message.

        \signreq
            — the \a leader account .
    */
    [[eosio::action]] void lock(symbol_code commun_code, name leader, mssgid message_id, string reason);

    /**
        \brief The \ref unlock action is used by a leader to unlock a message which was previously locked.

        \param commun_code community symbol, same as point symbol
        \param leader account of leader who is unlocking the message
        \param message_id identifier of the message (post or comment) to be unlocked
        \param reason reason of unlocking (this field should not be empty)

        \signreq
            — the \a leader account .
    */
    [[eosio::action]] void unlock(symbol_code commun_code, name leader, mssgid message_id, string reason);

    /**
        \brief The \ref upvote action is used by a community member to cast a vote in the form of «upvote» for a message.

        \param commun_code community symbol, same as point symbol
        \param voter account voting for the message
        \param message_id identifier of the message (post or comment)
        \param weight weight (in percent) of voter's gem «frozen» for voting. This parameter is optional, it should not equal to «0» if specified. The voter should provide a sufficient number of points for voting

        \signreq
            — the \a voter account ;  
            — <i>trusted community client</i> .

        \note
        If the action is also signed by the <i>trusted community client</i>, then the action does not check whether or not:
        - the specified weight equal to «0»;
        - the message exists;
        - the message is in ACTIVE state;
        - the gathering opinions period is over;
        - the voter has enough points.

        In this case the action also does not store anything in DB.
    */
    [[eosio::action]] void upvote(symbol_code commun_code, name voter, mssgid message_id, std::optional<uint16_t> weight);

    /**
        \brief The \ref downvote action is used by a community member to cast a vote in the form of «downvote» for a message.

        \param commun_code community symbol, same as point symbol
        \param voter account voting for the message
        \param message_id identifier of the message (post or comment)
        \param weight weight of voter's gem «frozen» for voting. This parameter is optional

        \signreq
            — the \a voter account ;  
            — <i>trusted community client</i> .

        \note
        If the action is also signed by the <i>trusted community client</i>, then the action does not check whether or not:
        - the specified weight equal to «0»;
        - the message exists;
        - the message is in ACTIVE state;
        - the gathering opinions period is over;
        - the voter has enough points.

        In this case the action also does not store anything in DB.
    */
    [[eosio::action]] void downvote(symbol_code commun_code, name voter, mssgid message_id, std::optional<uint16_t> weight);

    /**
        \brief The \ref unvote action is used by a community member to revoke a vote cast previously for a message.

        \param commun_code community symbol, same as point symbol
        \param voter account voting for the message
        \param message_id identifier of the message (post or comment) 

        \signreq
            — the \a voter account ;  
            — <i>trusted community client</i> (optional) .

        \note
        If the action is also signed by the <i>trusted community client</i>, then the action does not store anything in DB and does not check whether or not:
        - the message exists;  
        - the vote exists.
    */
    [[eosio::action]] void unvote(symbol_code commun_code, name voter, mssgid message_id);

    /**
        \brief The \ref claim action allows a user to get back the points previously allocated and «frozen» by her/him for a message, as well as expected share of reward accumulated for this message.

        \param commun_code community symbol, same as point symbol
        \param message_id identifier of the message (post or comment)
        \param gem_owner account who owns the gem
        \param gem_creator account who created the gem
        \param eager flag indicating the timeliness of the request; \a true — the request was sent in advance

        <b> \a CASE_1: The request is sent before calculation of rewards for the message</b>  
        User gets points, but does not get a reward.
        \signreq
            — the \a gem_owner account (or \a gem_creator) .

        <b> \a CASE_2: The request is sent after calculation of rewards for the message</b>  
        User gets points and a reward.
        \nosignreq
    */
    [[eosio::action]] void claim(symbol_code commun_code, mssgid message_id, name gem_owner,
        std::optional<name> gem_creator, std::optional<bool> eager);
    // TODO: removed from MVP
    void hold(symbol_code commun_code, mssgid message_id, name gem_owner, std::optional<name> gem_creator);
    // TODO: removed from MVP
    void transfer(symbol_code commun_code, mssgid message_id, name gem_owner, std::optional<name> gem_creator, name recipient);

    /**
         \brief The reblog action is used by user to create a reblog on the post/comment

         \param commun_code symbol of community POINT.
         \param rebloger account of rebloger
         \param message_id post to reblog
         \param header header of the reblog
         \param body body of the reblog

         Action do not stores any state in DB, it only checks input parameters.

         Performing the action requires rebloger signature.
         If also client signature provided, it doesn't check the presence of message.

         \signreq
            — the \a rebloger account ;
            — <i>trusted community client</i> .
    */
    [[eosio::action]] void reblog(symbol_code commun_code, name rebloger, mssgid message_id, std::string header, std::string body);


    /**
         \brief The erasereblog action is used by user to erase the reblog of the post/comment

         \param commun_code symbol of community POINT.
         \param rebloger account of rebloger
         \param message_id post to reblog

         Action do not stores any state in DB, it only checks input parameters.

         Performing the action requires rebloger signature.
         If also client signature provided, it doesn't check the presence of message.

         \signreq
            — the \a rebloger account ;
            — <i>trusted community client</i> .
    */
    [[eosio::action]] void erasereblog(symbol_code commun_code, name rebloger, mssgid message_id);
    // TODO: removed from MVP
    void setproviders(symbol_code commun_code, name recipient, std::vector<name> providers);

    // TODO: removed from MVP
    void provide(name grantor, name recipient, asset quantity, std::optional<uint16_t> fee);
    // TODO: removed from MVP
    void advise(symbol_code commun_code, name leader, std::set<mssgid> favorites);
    //TODO: void checkadvice (symbol_code commun_code, name leader);

    /**
        \brief The \ref ban action is used by leaders to lock a message for keeps.

        \param commun_code community symbol, same as point symbol
        \param message_id identifier of the message (post or comment) to be banned

        Collected reward to the author of the banned message and voted users will not be paid.

        \signreq
            — <i>minority of community leaders</i> .
    */
    [[eosio::action]] void ban(symbol_code commun_code, mssgid message_id);

    ON_TRANSFER(COMMUN_POINT) void ontransfer(name from, name to, asset quantity, std::string memo) {
        on_points_transfer(_self, from, to, quantity, memo);
    }

private:
    gallery_types::providers_t get_providers(symbol_code commun_code, name account, uint16_t gems_per_period, std::optional<uint16_t> weight);
    accparams::const_iterator get_acc_param(accparams& accparams_table, symbol_code commun_code, name account);
    uint16_t get_gems_per_period(symbol_code commun_code);
    static int64_t get_amount_to_freeze(int64_t balance, int64_t frozen, uint16_t gems_per_period, std::optional<uint16_t> weight);
    void set_vote(symbol_code commun_code, name voter, const mssgid &message_id, std::optional<uint16_t> weight, bool damn);
    bool validate_permlink(std::string permlink);
    bool check_mssg_exists(symbol_code commun_code, const mssgid& message_id);
    void require_client_auth(const std::string& s = "Action enable only for client");
};

} // commun
