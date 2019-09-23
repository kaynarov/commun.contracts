#pragma once
#include <commun.publication/objects.hpp>
#include <commun.publication/parameters.hpp>
#include <eosio/transaction.hpp>

namespace commun {

using namespace eosio;

class publication : public gallery_base<publication>, public contract {

public:
    using contract::contract;
    
    static void deactivate(name self, symbol_code commun_code, const gallery_types::mosaic& mosaic) {
        vertices vertices_table(self, commun_code.raw());
        auto vertices_index = vertices_table.get_index<"bykey"_n>();
        auto vertex = vertices_index.find(std::make_tuple(mosaic.creator, mosaic.tracery));
        eosio::check(vertex != vertices_index.end(), "SYSTEM: Permlink doesn't exist.");
        
        gallery_types::params params_table(self, commun_code.raw());
        const auto& param = params_table.get(commun_code.raw(), "SYSTEM: param does not exists");
        auto active_period_end = mosaic.created + eosio::seconds(param.mosaic_active_period);
        auto now = eosio::current_time_point();
        
        eosio::check(vertex->childcount == 0 || now > active_period_end, "comment with child comments can't be deleted during the active period");
        
        if (vertex->parent_creator) {
            auto parent_vertex = vertices_index.find(std::make_tuple(vertex->parent_creator, vertex->parent_tracery));
            if (parent_vertex != vertices_index.end()) {
                vertices_index.modify(parent_vertex, eosio::same_payer, [&](auto& item) { item.childcount--; });
            }
        }
        vertices_table.erase(*vertex);
    }

    void createmssg(symbol_code commun_code, mssgid_t message_id, mssgid_t parent_id,
        std::string headermssg, std::string bodymssg, std::string languagemssg, std::vector<std::string> tags, std::string jsonmetadata,
        uint16_t curators_prcnt);
    void updatemssg(symbol_code commun_code, mssgid_t message_id, std::string headermssg, std::string bodymssg,
                        std::string languagemssg, std::vector<std::string> tags, std::string jsonmetadata);
    void deletemssg(symbol_code commun_code, mssgid_t message_id);
    void upvote(symbol_code commun_code, name voter, mssgid_t message_id, uint16_t weight);
    void downvote(symbol_code commun_code, name voter, mssgid_t message_id, uint16_t weight);
    void unvote(symbol_code commun_code, name voter, mssgid_t message_id);
    void claim(symbol_code commun_code, mssgid_t message_id, name gem_owner, 
                    std::optional<name> gem_creator, std::optional<bool> eager);
    void hold(symbol_code commun_code, mssgid_t message_id, name gem_owner, std::optional<name> gem_creator);
    void transfer(symbol_code commun_code, mssgid_t message_id, name gem_owner, std::optional<name> gem_creator, name recipient);

    void setparams(symbol_code commun_code, std::vector<posting_params> params);
    void reblog(symbol_code commun_code, name rebloger, mssgid_t message_id, std::string headermssg, std::string bodymssg);
    void erasereblog(symbol_code commun_code, name rebloger, mssgid_t message_id);
    void setproviders(symbol_code commun_code, name recipient, std::vector<name> providers);
    void setfrequency(symbol_code commun_code, name account, uint16_t actions_per_day);
    
    void provide(name grantor, name recipient, asset quantity, std::optional<uint16_t> fee);
    void advise(symbol_code commun_code, name leader, std::vector<mssgid_t> favorites);
    //TODO: void checkadvice (symbol_code commun_code, name leader);
    void slap(symbol_code commun_code, name leader, mssgid_t message_id);
    
    void ontransfer(name from, name to, asset quantity, std::string memo) {
        on_points_transfer(_self, from, to, quantity, memo);
    } 
    
private:
    gallery_types::providers_t get_providers(symbol_code commun_code, name account, uint16_t weight = config::_100percent);
    accparams::const_iterator get_acc_param(accparams& accparams_table, symbol_code commun_code, name account);
    static int64_t get_amount_to_freeze(int64_t balance, int64_t frozen, uint16_t actions_per_day, int64_t mosaic_active_period, 
                                        int64_t actual_limit = std::numeric_limits<int64_t>::max());
    const posting_state& params(symbol_code commun_code);
    void set_vote(symbol_code commun_code, name voter, const mssgid_t &message_id, int16_t weight);
    bool validate_permlink(std::string permlink);
};

} // commun
