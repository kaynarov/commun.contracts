#pragma once
#include <commun.publication/objects.hpp>
#include <eosio/transaction.hpp>

namespace commun {

using namespace eosio;

/**
 * \brief This class implements comn.publish contract behaviour
 * \ingroup publish_class
 */
class publication : public gallery_base<publication>, public contract {

public:
    using contract::contract;
    
    static bool can_remove_vertex(const vertex_t& arg, const gallery_types::mosaic& mosaic, const structures::community& community) {
        auto now = eosio::current_time_point();
        return arg.childcount == 0
            || now > (mosaic.collection_end_date + eosio::seconds(community.moderation_period + community.extra_reward_period));
    }

    static void deactivate(name self, symbol_code commun_code, const gallery_types::mosaic& mosaic) {
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
     * \brief The \ref init action is used by commun.list contract to initialize gallery for a community with specified point symbol.
     *
     * \param commun_code a point symbol of community
     *
     * This action is unavailable for user and can be called only internally.
     * It requires signature of the gallery contract account.
     */
    void init(symbol_code commun_code);

    /**
     * \brief The \ref emit action is used by the client to emit rewards for specified point symbol.
     *
     * \param commun_code a point symbol of community
     *
     * This action is unavailable for user and can be called only by the client.
     * It requires signature of the client.
     */
    void emit(symbol_code commun_code);

    /**
        \brief The create action is used by account to create a message.

        \param commun_code symbol of community POINT
        \param message_id post/comment to create
        \param parent_id if has author, parent post/comment to create comment
        \param header header of the message (not stored in DB, only validation)
        \param body body of the message (not stored in DB, only validation)
        \param tags tags of the message (not stored in DB)
        \param metadata metadata of the message (not stored in DB)
        \param weight weight of freezing points for the message (only for posts, optional)

        Performing the action requires message_id.author signature.
    */
    void create(symbol_code commun_code, mssgid_t message_id, mssgid_t parent_id,
        std::string header, std::string body, std::vector<std::string> tags, std::string metadata,
        std::optional<uint16_t> weight);

    /**
        \brief The update action is used by account to update a message.

        \param commun_code symbol of community POINT
        \param message_id post/comment to create
        \param header header of the message (not stored in DB)
        \param body body of the message (not stored in DB)
        \param tags tags of the message (not stored in DB)
        \param metadata metadata of the message (not stored in DB)

        Performing the action requires message_id.author signature.
        If message was locked and unlocked, leaders can lock it again after update.
        If client signature provided, it doesn't check the presence of message and doesn't allow leaders lock it after unlock.
    */
    void update(symbol_code commun_code, mssgid_t message_id, std::string header, std::string body,
        std::vector<std::string> tags, std::string metadata);

    /**
        \brief The settags action is used by leader to edit message tags.

        \param commun_code symbol of community POINT
        \param leader account of leader
        \param message_id post/comment (message)
        \param add_tags tags adding to the message
        \param remove_tags tags removing from the message
        \param reason reason of tags edit

        Action do not stores any state in DB, it only checks input parameters.

        Performing the action requires the leader signature.
        If client signature provided, it doesn't check the presence of message.
    */
    void settags(symbol_code commun_code, name leader, mssgid_t message_id,
        std::vector<std::string> add_tags, std::vector<std::string> remove_tags, std::string reason);

    /**
        \brief The remove action is used by message author to remove the message.

        \param commun_code symbol of community POINT
        \param message_id post/comment (message) to remove

        Performing the action requires the message author signature.
        If client signature provided, it doesn't check the presence of message and do not does any changes in DB (do not claims gems).
    */
    void remove(symbol_code commun_code, mssgid_t message_id);

    /**
        \brief The report action is used by user to report the message

        \param commun_code symbol of community POINT.
        \param reporter account which reporting message
        \param message_id post/comment (message)
        \param reason reason of report

        Action do not stores any state in DB, it only checks input parameters.

        Performing the action requires reporter signature.
        If also client signature provided, it doesn't check the presence of message.
    */
    void report(symbol_code commun_code, name reporter, mssgid_t message_id, std::string reason);

    /**
        \brief The lock action is used by leader to lock the message

        \param commun_code symbol of community POINT.
        \param leader account which locking message
        \param message_id post/comment (message)
        \param reason reason of locking

        Locking message stops receiving reward by message.

        Performing the action requires leader signature.
    */
    void lock(symbol_code commun_code, name leader, mssgid_t message_id, string reason);

    /**
        \brief The unlock action is used by leader to unlock the message

        \param commun_code symbol of community POINT.
        \param leader account which unlocking message
        \param message_id post/comment (message)
        \param reason reason of locking

        Performing the action requires leader signature.
    */
    void unlock(symbol_code commun_code, name leader, mssgid_t message_id, string reason);

    /**
        \brief The upvote action is used by account to upvote the message.

        \param commun_code symbol of community POINT.
        \param voter account which voting for the message
        \param message_id post/comment (message)
        \param weight weight of gem freezing for voting (optional)

        Performing the action requires voter signature.
        If also client signature provided, this action doesn't check that weight is not 0, that message exists, active and in collection period, and that voter has enough points. In this case action doesn't store anything in DB.
    */
    void upvote(symbol_code commun_code, name voter, mssgid_t message_id, std::optional<uint16_t> weight);

    /**
        \brief The downvote action is used by account to downvote the message.

        \param commun_code symbol of community POINT.
        \param voter account which voting for the message
        \param message_id post/comment (message)
        \param weight weight of gem freezing for voting (optional)

        Performing the action requires voter signature.
        If also client signature provided, this action doesn't check that weight is not 0, that message exists, active and in collection period, and that voter has enough points. In this case action doesn't store anything in DB.
    */
    void downvote(symbol_code commun_code, name voter, mssgid_t message_id, std::optional<uint16_t> weight);

    /**
        \brief The unvote action is used by account to remove vote on the message.

        \param commun_code symbol of community POINT.
        \param voter account which voting for the message
        \param message_id post/comment (message)

        Performing the action requires voter signature.
        If also client signature provided, this action doesn't check that message exists. In this case action doesn't store anything in DB.
    */
    void unvote(symbol_code commun_code, name voter, mssgid_t message_id);

    /**
        \brief The claim action allows to claim reward accumulated in message.

        \param commun_code symbol of community POINT.
        \param message_id post/comment (message)
        \param gem_owner account which owns the gem
        \param gem_creator account which created the gem
        \param eager early claim message

        Performing the action requires gem_owner or gem_creator signature.
    */
    void claim(symbol_code commun_code, mssgid_t message_id, name gem_owner,
        std::optional<name> gem_creator, std::optional<bool> eager);
    // TODO: removed from MVP
    void hold(symbol_code commun_code, mssgid_t message_id, name gem_owner, std::optional<name> gem_creator);
    // TODO: removed from MVP
    void transfer(symbol_code commun_code, mssgid_t message_id, name gem_owner, std::optional<name> gem_creator, name recipient);

    // TODO: removed from MVP
    // /**
    //     \brief The reblog action is used by user to create a reblog on the post/comment

    //     \param commun_code symbol of community POINT.
    //     \param rebloger account of rebloger
    //     \param message_id post to reblog
    //     \param header header of the reblog
    //     \param body body of the reblog

    //     Action do not stores any state in DB, it only checks input parameters.

    //     Performing the action requires rebloger signature.
    //     If also client signature provided, it doesn't check the presence of message.
    // */
    void reblog(symbol_code commun_code, name rebloger, mssgid_t message_id, std::string header, std::string body);

    // TODO: removed from MVP
    // /**
    //     \brief The erasereblog action is used by user to erase the reblog of the post/comment

    //     \param commun_code symbol of community POINT.
    //     \param rebloger account of rebloger
    //     \param message_id post to reblog

    //     Action do not stores any state in DB, it only checks input parameters.

    //     Performing the action requires rebloger signature.
    //     If also client signature provided, it doesn't check the presence of message.
    // */
    void erasereblog(symbol_code commun_code, name rebloger, mssgid_t message_id);
    // TODO: removed from MVP
    void setproviders(symbol_code commun_code, name recipient, std::vector<name> providers);

    // TODO: removed from MVP
    void provide(name grantor, name recipient, asset quantity, std::optional<uint16_t> fee);
    // TODO: removed from MVP
    void advise(symbol_code commun_code, name leader, std::set<mssgid_t> favorites);
    //TODO: void checkadvice (symbol_code commun_code, name leader);

    /**
        \brief The ban action is used leaders to ban the message.

        \param commun_code symbol of community POINT
        \param message_id post/comment (message) to ban

        Performing the action requires authority of (top) leaders.
    */
    void ban(symbol_code commun_code, mssgid_t message_id);

    void ontransfer(name from, name to, asset quantity, std::string memo) {
        on_points_transfer(_self, from, to, quantity, memo);
    }

private:
    gallery_types::providers_t get_providers(symbol_code commun_code, name account, uint16_t gems_per_period, std::optional<uint16_t> weight);
    accparams::const_iterator get_acc_param(accparams& accparams_table, symbol_code commun_code, name account);
    uint16_t get_gems_per_period(symbol_code commun_code);
    static int64_t get_amount_to_freeze(int64_t balance, int64_t frozen, uint16_t gems_per_period, std::optional<uint16_t> weight);
    void set_vote(symbol_code commun_code, name voter, const mssgid_t &message_id, std::optional<uint16_t> weight, bool damn);
    bool validate_permlink(std::string permlink);
    bool check_mssg_exists(symbol_code commun_code, const mssgid_t& message_id);
    void require_client_auth(const std::string& s = "Action enable only for client");
};

} // commun
