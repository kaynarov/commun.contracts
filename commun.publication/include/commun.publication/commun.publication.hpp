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

    static void deactivate(name self, symbol_code commun_code, const gallery_types::mosaic& mosaic) {
        vertices vertices_table(self, commun_code.raw());
        auto vertex = vertices_table.find(mosaic.tracery);
        eosio::check(vertex != vertices_table.end(), "SYSTEM: Permlink doesn't exist.");

        auto now = eosio::current_time_point();

        eosio::check(vertex->childcount == 0 || now > mosaic.close_date, "comment with child comments can't be removed during the active period");

        if (vertex->parent_tracery) {
            auto parent_vertex = vertices_table.find(vertex->parent_tracery);
            if (parent_vertex != vertices_table.end()) {
                vertices_table.modify(parent_vertex, eosio::same_payer, [&](auto& item) { item.childcount--; });
            }
        }
        vertices_table.erase(*vertex);
    }

    void create(symbol_code commun_code, mssgid_t message_id, mssgid_t parent_id,
        std::string header, std::string body, std::vector<std::string> tags, std::string metadata,
        std::optional<uint16_t> weight);
    void update(symbol_code commun_code, mssgid_t message_id, std::string header, std::string body,
        std::vector<std::string> tags, std::string metadata);
    void settags(symbol_code commun_code, name leader, mssgid_t message_id,
        std::vector<std::string> add_tags, std::vector<std::string> remove_tags, std::string reason);
    void remove(symbol_code commun_code, mssgid_t message_id);
    void report(symbol_code commun_code, name reporter, mssgid_t message_id, std::string reason);
    void lock(symbol_code commun_code, name leader, mssgid_t message_id, string reason);
    void unlock(symbol_code commun_code, name leader, mssgid_t message_id, string reason);
    void upvote(symbol_code commun_code, name voter, mssgid_t message_id, std::optional<uint16_t> weight);
    void downvote(symbol_code commun_code, name voter, mssgid_t message_id, std::optional<uint16_t> weight);
    void unvote(symbol_code commun_code, name voter, mssgid_t message_id);
    void claim(symbol_code commun_code, mssgid_t message_id, name gem_owner,
        std::optional<name> gem_creator, std::optional<bool> eager);
    void hold(symbol_code commun_code, mssgid_t message_id, name gem_owner, std::optional<name> gem_creator);
    void transfer(symbol_code commun_code, mssgid_t message_id, name gem_owner, std::optional<name> gem_creator, name recipient);

    /**
     * \brief action is used by user to create a reblog on the post/comment
     *
     * \param commun_code symbol of community POINT.
     * \param rebloger account of rebloger
     * \param message_id post to reblog
     * \param header header of the reblog
     * \param body body of the reblog
     */
    void reblog(symbol_code commun_code, name rebloger, mssgid_t message_id, std::string header, std::string body);
    void erasereblog(symbol_code commun_code, name rebloger, mssgid_t message_id);
    void setproviders(symbol_code commun_code, name recipient, std::vector<name> providers);

    void provide(name grantor, name recipient, asset quantity, std::optional<uint16_t> fee);
    void advise(symbol_code commun_code, name leader, std::set<mssgid_t> favorites);
    //TODO: void checkadvice (symbol_code commun_code, name leader);
    void ban(symbol_code commun_code, mssgid_t message_id);

    void ontransfer(name from, name to, asset quantity, std::string memo) {
        on_points_transfer(_self, from, to, quantity, memo);
    }

private:
    gallery_types::providers_t get_providers(symbol_code commun_code, name account, uint16_t gems_per_period, std::optional<uint16_t> weight);
    accparams::const_iterator get_acc_param(accparams& accparams_table, symbol_code commun_code, name account);
    uint16_t get_gems_per_period(symbol_code commun_code, int64_t mosaic_active_period = 0);
    static int64_t get_amount_to_freeze(int64_t balance, int64_t frozen, uint16_t gems_per_period, std::optional<uint16_t> weight);
    void set_vote(symbol_code commun_code, name voter, const mssgid_t &message_id, std::optional<uint16_t> weight, bool damn);
    bool validate_permlink(std::string permlink);
    bool check_mssg_exists(symbol_code commun_code, const mssgid_t& message_id, name actor = name());
    void check_auth(const std::string& s, name actor = name());
};

} // commun
