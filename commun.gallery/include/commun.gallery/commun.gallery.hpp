/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include "config.hpp"

#include <string>
#include <cmath>
#include <eosio/system.hpp>
#include <commun/util.hpp>
#include <commun.point/commun.point.hpp>
#include <commun.emit/commun.emit.hpp>
#include <commun.emit/config.hpp>

namespace commun {

using std::string;
using namespace eosio;

namespace gallery_types {
    using mosaic_key_t = std::tuple<name, uint64_t>;
    using providers_t = std::vector<std::pair<name, int64_t> >; 
    
    struct mosaic {
   
        mosaic() = default;
        uint64_t id;
        name creator;
        uint64_t tracery;
        uint16_t royalty;
        
        time_point created;
        uint16_t gem_count;
        
        int64_t points;
        int64_t shares;
        int64_t damn_points = 0;
        int64_t damn_shares = 0;
        
        int64_t reward = 0;
        
        int64_t comm_rating = 0;
        int64_t lead_rating = 0;
        
        enum status_t: uint8_t {ACTIVE, ARCHIVED, BANNED};
        uint8_t status = ACTIVE;

        std::vector<name> slaps;
        
        void ban() { status = BANNED; }
        void archive() { status = ARCHIVED; }
        bool banned()const { return status == BANNED; }
       
        uint64_t primary_key() const { return id; }
        using key_t = gallery_types::mosaic_key_t;
        key_t by_key()const { return std::make_tuple(creator, tracery); }
        using by_rating_t = std::tuple<uint8_t, int64_t, int64_t>;
        by_rating_t by_comm_rating()const { return std::make_tuple(status, comm_rating, lead_rating); }
        by_rating_t by_lead_rating()const { return std::make_tuple(status, lead_rating, comm_rating); }
    };
    
    struct gem {
        uint64_t id;
        uint64_t mosaic_id;
        time_point claim_date;
        
        int64_t points;
        int64_t shares;
        
        name owner;
        name creator;
        
        uint64_t primary_key() const { return id; }
        using key_t = std::tuple<uint64_t, name, name>;
        key_t by_key()const { return std::make_tuple(mosaic_id, owner, creator); }
        key_t by_creator()const { return std::make_tuple(mosaic_id, creator, owner); }
        using by_claim_t = std::tuple<name, time_point>;
        by_claim_t by_claim()const { return std::make_tuple(owner, claim_date); }
        time_point by_claim_joint()const { return claim_date; }
        
        void check_claim(time_point now, bool has_reward) const {
            eosio::check(now >= claim_date || has_auth(owner) || (has_auth(creator) && !has_reward), "lack of necessary authority");
        }
    };
    
    struct [[eosio::table]] inclusion {
        asset quantity;
            // just an idea:
            // use as inclusion not only points, but also other gems. 
            // this will allow to buy shares in the mosaics without liquid points, creating more sophisticated collectables
            // (pntinclusion / geminclusion)
        
        uint64_t primary_key()const { return quantity.symbol.code().raw(); }
    };


    struct [[eosio::table]] param {
        uint64_t id;
        symbol commun_symbol;
        
        int64_t collection_period;
        int64_t evaluation_period;
        int64_t mosaic_active_period;
        uint16_t max_royalty; // <= 100%
        uint8_t ban_threshold;
        uint8_t rewarded_num;
        
        int64_t comm_points_grade_sum;
        std::vector<int64_t> comm_grades;
        std::vector<int64_t> lead_grades;
        uint64_t primary_key()const { return id; }
    };
    
    struct [[eosio::table]] stat {
        uint64_t id;
        int64_t unclaimed = 0;

        uint64_t primary_key()const { return id; }
    };
    
    struct [[eosio::table]] provision {
        uint64_t id;
        name grantor;
        name recipient;
        uint16_t fee;
        int64_t total  = 0;
        int64_t frozen = 0;
        int64_t available()const { return total - frozen; };
        
        uint64_t primary_key()const { return id; }
        using key_t = std::tuple<name, name>;
        key_t by_key()const { return std::make_tuple(grantor, recipient); }
    };
    
    struct [[eosio::table]] advice {
        uint64_t id;
        name leader;
        std::vector<gallery_types::mosaic_key_t> favorites;
        uint64_t primary_key()const { return id; }
    };
    
    using mosaic_id_index = eosio::indexed_by<"mosaicid"_n, eosio::const_mem_fun<gallery_types::mosaic, uint64_t, &gallery_types::mosaic::primary_key> >;
    using mosaic_key_index = eosio::indexed_by<"bykey"_n, eosio::const_mem_fun<gallery_types::mosaic, gallery_types::mosaic::key_t, &gallery_types::mosaic::by_key> >;
    using mosaic_comm_index = eosio::indexed_by<"bycommrating"_n, eosio::const_mem_fun<gallery_types::mosaic, gallery_types::mosaic::by_rating_t, &gallery_types::mosaic::by_comm_rating> >;
    using mosaic_lead_index = eosio::indexed_by<"byleadrating"_n, eosio::const_mem_fun<gallery_types::mosaic, gallery_types::mosaic::by_rating_t, &gallery_types::mosaic::by_lead_rating> >;
    using mosaics = eosio::multi_index<"mosaic"_n, gallery_types::mosaic, mosaic_id_index, mosaic_key_index, mosaic_comm_index, mosaic_lead_index>;
    
    using gem_id_index = eosio::indexed_by<"gemid"_n, eosio::const_mem_fun<gallery_types::gem, uint64_t, &gallery_types::gem::primary_key> >;
    using gem_key_index = eosio::indexed_by<"bykey"_n, eosio::const_mem_fun<gallery_types::gem, gallery_types::gem::key_t, &gallery_types::gem::by_key> >;
    using gem_creator_index = eosio::indexed_by<"bycreator"_n, eosio::const_mem_fun<gallery_types::gem, gallery_types::gem::key_t, &gallery_types::gem::by_creator> >;
    using gem_claim_index = eosio::indexed_by<"byclaim"_n, eosio::const_mem_fun<gallery_types::gem, gallery_types::gem::by_claim_t, &gallery_types::gem::by_claim> >;
    using gem_claim_joint_index = eosio::indexed_by<"byclaimjoint"_n, eosio::const_mem_fun<gallery_types::gem, time_point, &gallery_types::gem::by_claim_joint> >;
    using gems = eosio::multi_index<"gem"_n, gallery_types::gem, gem_id_index, gem_key_index, gem_creator_index, gem_claim_index, gem_claim_joint_index>;
    
    using inclusions = eosio::multi_index<"inclusion"_n, gallery_types::inclusion>;
    
    using prov_id_index = eosio::indexed_by<"provid"_n, eosio::const_mem_fun<gallery_types::provision, uint64_t, &gallery_types::provision::primary_key> >;
    using prov_key_index = eosio::indexed_by<"bykey"_n, eosio::const_mem_fun<gallery_types::provision, gallery_types::provision::key_t, &gallery_types::provision::by_key> >;
    using provs = eosio::multi_index<"provision"_n, gallery_types::provision, prov_id_index, prov_key_index>;

    using advices = eosio::multi_index<"advice"_n, gallery_types::advice>;
    
    using params = eosio::multi_index<"param"_n, gallery_types::param>;
    using stats = eosio::multi_index<"stat"_n, gallery_types::stat>;   
}


template<typename T>
class gallery_base { 
private:
    bool check_leader(symbol_code commun_code, name arg) { return true; }; //TODO
    
    bool transfer(name from, name to, const asset &quantity) {
        if (to && !point::balance_exists(config::commun_point_name, to, quantity.symbol.code())) {
            return false;
        }
        
        if (quantity.amount) {
            action(
                permission_level{config::commun_point_name, config::transfer_permission},
                config::commun_point_name,
                "transfer"_n,
                std::make_tuple(from, to, quantity, string())
            ).send();
        }
        return true; 
    };
    
    void freeze(name _self, name account, const asset &quantity, name ram_payer = name()) {
        
        if (!quantity.amount) {
            return;
        }
        auto commun_code = quantity.symbol.code();
        auto balance_exists = point::balance_exists(config::commun_point_name, account, commun_code);
        eosio::check(balance_exists, quantity.amount > 0 ? "balance doesn't exist" : "SYSTEM: points are frozen while balance doesn't exist");

        auto balance_amount = point::get_balance(config::commun_point_name, account, commun_code).amount;
        
        gallery_types::inclusions inclusions_table(_self, account.value);
        auto incl = inclusions_table.find(quantity.symbol.code().raw());
        eosio::check((incl == inclusions_table.end()) || ((0 < incl->quantity.amount) && (incl->quantity.amount <= balance_amount)), 
            "SYSTEM: invalid value of frozen points");
        
        auto new_val = (incl != inclusions_table.end()) ? quantity.amount + incl->quantity.amount : quantity.amount;
        eosio::check(new_val >= 0, "SYSTEM: impossible combination of quantity and frozen points");
        eosio::check(new_val <= balance_amount, "overdrawn balance");
        if (new_val) {
            if (incl != inclusions_table.end()) {
                inclusions_table.modify(incl, name(), [&](auto& item) { item.quantity.amount = new_val; });
            }
            else {
                inclusions_table.emplace(ram_payer ? ram_payer : account, [&](auto& item) { item.quantity = quantity; });
            }
        }
        else { // => (incl != inclusions_table.end())
            inclusions_table.erase(incl);
        }
    }
    
    static inline int64_t get_reserve_amount(asset quantity) {
        return point::get_reserve_quantity(config::commun_point_name, quantity, false).amount;
    }
    
    static int64_t get_points_sum(int64_t quantity_amount, const gallery_types::providers_t& providers) {
        int64_t ret = quantity_amount;
        for (const auto& p : providers) {
            check(p.second > 0, "provided points must be positive");
            ret += p.second;
        }

        return ret;
    }
    
    static int64_t get_min_cost(const gallery_types::providers_t& providers) {
        return (providers.size() + 1) * config::min_gem_cost;
    }
    
    void chop_gem(name _self, symbol commun_symbol, const gallery_types::gem& gem, bool no_rewards = false) {
    
        auto commun_code = commun_symbol.code();
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto mosaic = mosaics_table.find(gem.mosaic_id);
        eosio::check(mosaic != mosaics_table.end(), "mosaic doesn't exist");
        
        int64_t reward = 0;
        bool damn = gem.shares < 0;
        if (!no_rewards && damn == (mosaic->banned())) {
            reward = damn ? 
                safe_prop(mosaic->reward, -gem.shares, mosaic->damn_shares) :
                safe_prop(mosaic->reward,  gem.shares, mosaic->shares);
        }
        asset frozen_points(gem.points, commun_symbol);
        asset reward_points(reward, commun_symbol);
        
        freeze(_self, gem.owner, -frozen_points);

        if (gem.creator != gem.owner) {
            
            gallery_types::provs provs_table(_self, commun_code.raw());
            auto provs_index = provs_table.get_index<"bykey"_n>();
            auto prov_itr = provs_index.find(std::make_tuple(gem.owner, gem.creator));
            if (prov_itr != provs_index.end()) {
                provs_index.modify(prov_itr, name(), [&](auto& item) {
                    item.total  += reward_points.amount;
                    item.frozen -= frozen_points.amount;
                });
            }
        }
        if (!transfer(_self, gem.owner, reward_points)) {
            eosio::check(transfer(_self, point::get_issuer(config::commun_point_name, commun_code), reward_points), "the issuer's balance doesn't exist");
        }
        
        if (mosaic->gem_count > 1 || mosaic->lead_rating) {
            mosaics_table.modify(mosaic, name(), [&](auto& item) {
                if (!damn) {
                    item.points -= gem.points;
                    item.shares -= gem.shares;
                    item.comm_rating -= gem.points;
                }
                else {
                    item.damn_points -= gem.points;
                    item.damn_shares += gem.shares;
                    item.comm_rating += gem.points;
                }
                item.reward -= reward;
                item.gem_count--;
            });
        }
        else {
            gallery_types::stats stats_table(_self, commun_code.raw());
            const auto& stat = stats_table.get(commun_code.raw(), "SYSTEM: stat does not exists");
            stats_table.modify(stat, name(), [&]( auto& s) { s.unclaimed += mosaic->reward - reward; });
            
            T::prepare_erase(_self, commun_code, *mosaic);
            mosaics_table.erase(mosaic);
        }
    }
    
    void freeze_points_in_gem(name _self, bool can_refill, symbol commun_symbol, uint64_t mosaic_id, time_point claim_date, 
                              int64_t points, int64_t shares, name owner, name creator) {
        check(points >= 0, "SYSTEM: points can't be negative");
        if (!shares && ! points) {
            return;
        }
        
        auto commun_code = commun_symbol.code();

        gallery_types::gems gems_table(_self, commun_code.raw());
        
        bool refilled = false;
        
        if (can_refill) {
            auto gems_idx = gems_table.get_index<"bykey"_n>();
            auto gem = gems_idx.find(std::make_tuple(mosaic_id, owner, creator));
            if (gem != gems_idx.end()) {
                eosio::check((shares < 0) == (gem->shares < 0), "gem type mismatch");
                gems_idx.modify(gem, name(), [&](auto& item) {
                    item.points += points;
                    item.shares += shares;
                });
                refilled = true;
            }
        }
        
        if (!refilled) {
            gallery_types::params params_table(_self, commun_code.raw());
            const auto& param = params_table.get(commun_code.raw(), "param does not exists");
            
            uint8_t gem_num = 0;
            auto max_claim_date = eosio::current_time_point();
            auto claim_idx = gems_table.get_index<"byclaim"_n>();
            auto chop_gem_of = [&](name account) {
                auto gem_itr = claim_idx.lower_bound(std::make_tuple(account, time_point()));
                if ((gem_itr != claim_idx.end()) && (gem_itr->owner == account) && (gem_itr->claim_date < max_claim_date)) {
                    chop_gem(_self, commun_symbol, *gem_itr);
                    claim_idx.erase(gem_itr);
                    ++gem_num;
                }
            };
            chop_gem_of(owner);
            if (owner != creator) {
                chop_gem_of(creator);
            }
            max_claim_date -= eosio::seconds(config::forced_chopping_delay);
            auto joint_idx = gems_table.get_index<"byclaimjoint"_n>();
            auto gem_itr = joint_idx.begin();
            
            while ((gem_itr != joint_idx.end()) && (gem_itr->claim_date < max_claim_date) && (gem_num < config::auto_claim_num)) {
                chop_gem(_self, commun_symbol, *gem_itr, true);
                gem_itr = joint_idx.erase(gem_itr);
                ++gem_num;
            }
            
            gems_table.emplace(creator, [&]( auto &item ) { item = gallery_types::gem {
                .id = gems_table.available_primary_key(),
                .mosaic_id = mosaic_id,
                .claim_date = claim_date,
                .points = points,
                .shares = shares,
                .owner = owner,
                .creator = creator
            };});
        }
        freeze(_self, owner, asset(points, commun_symbol), creator);
        
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto mosaic = mosaics_table.find(mosaic_id);
        eosio::check(mosaic != mosaics_table.end(), "mosaic doesn't exist");
        
        mosaics_table.modify(mosaic, name(), [&](auto& item) {
            if (shares < 0) {
                item.damn_points += points;
                item.damn_shares -= shares;
                item.comm_rating -= points;
            }
            else {
                item.points += points;
                item.shares += shares;
                item.comm_rating += points;
            }
            if (!refilled) {
                eosio::check(item.gem_count < std::numeric_limits<decltype(item.gem_count)>::max(), "gem_count overflow");
                item.gem_count++;
            }
        });
    }
                              
    int64_t pay_royalties(name _self, symbol_code commun_code, uint64_t mosaic_id, name mosaic_creator, int64_t shares) {
    
        gallery_types::gems gems_table(_self, commun_code.raw());
    
        auto gems_idx = gems_table.get_index<"bycreator"_n>();
        auto first_gem_itr = gems_idx.lower_bound(std::make_tuple(mosaic_id, mosaic_creator, name()));
        int64_t pre_shares_sum = 0;
        for (auto gem_itr = first_gem_itr; (gem_itr != gems_idx.end()) && 
                                           (gem_itr->mosaic_id == mosaic_id) && 
                                           (gem_itr->creator == mosaic_creator); ++gem_itr) {
            if (gem_itr->shares > 0) {
                pre_shares_sum += gem_itr->shares;
            }
        }
        
        if (!pre_shares_sum) {
            return 0;
        }
        
        int64_t ret = 0;
        
        for (auto gem_itr = first_gem_itr; (gem_itr != gems_idx.end()) && 
                                           (gem_itr->mosaic_id == mosaic_id) && 
                                           (gem_itr->creator == mosaic_creator); ++gem_itr) {
            if (gem_itr->shares > 0) {
                int64_t cur_shares = safe_prop(shares, gem_itr->shares, pre_shares_sum);
                gems_idx.modify(gem_itr, name(), [&](auto& item) {
                    item.shares += cur_shares;
                });
                ret += cur_shares;
            }
        }
        
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto mosaic = mosaics_table.find(mosaic_id);
        eosio::check(mosaic != mosaics_table.end(), "mosaic doesn't exist");
        
        mosaics_table.modify(mosaic, name(), [&](auto& item) { item.shares += ret; });
        
        return ret;
    }
    
    void maybe_issue_reward(name _self, const gallery_types::param& param) {
        if (emit::it_is_time_to_reward(config::commun_emit_name, param.commun_symbol.code(), false)) {
            action(
                permission_level{config::commun_emit_name, config::reward_perm_name},
                config::commun_emit_name,
                "issuereward"_n,
                std::make_tuple(param.commun_symbol, false)
            ).send();
        }
    }

    void freeze_in_gems(name _self, bool can_refill, uint64_t mosaic_id, time_point claim_date, 
                      name creator, asset quantity, gallery_types::providers_t providers, bool damn, int64_t shares_abs = -1) {
        
        int64_t points_sum = get_points_sum(quantity.amount, providers);
        if (shares_abs < 0) {
            shares_abs = points_sum;
        }         

        int64_t total_fee = 0;
        auto commun_symbol = quantity.symbol;
        auto commun_code = commun_symbol.code();          
        
        gallery_types::provs provs_table(_self, commun_code.raw());
        auto provs_index = provs_table.get_index<"bykey"_n>();
        
        for (const auto& p : providers) {
            auto prov_itr = provs_index.find(std::make_tuple(p.first, creator));
            eosio::check(prov_itr != provs_index.end(), "no points provided");
            int64_t cur_fee = safe_pct(prov_itr->fee, p.second);
            
            auto cur_shares_abs = safe_prop(shares_abs, p.second - cur_fee, points_sum);
            total_fee += cur_fee;
            freeze_points_in_gem(_self, can_refill, commun_symbol, mosaic_id, claim_date, p.second, damn ? -cur_shares_abs : cur_shares_abs, p.first, creator);
            eosio::check(prov_itr->available() >= p.second, "not enough provided points");
            provs_index.modify(prov_itr, name(), [&](auto& item) { item.frozen += p.second; });
        }
        
        if (quantity.amount || total_fee) {
            auto cur_shares_abs = safe_prop(shares_abs, quantity.amount + total_fee, points_sum);
            freeze_points_in_gem(_self, can_refill, commun_symbol, mosaic_id, claim_date, quantity.amount, damn ? -cur_shares_abs : cur_shares_abs, creator, creator);
        }
    }
    
    struct claim_info_t {
        uint64_t mosaic_id;
        bool has_reward;
        bool premature;
        symbol commun_symbol;
    };
    
    claim_info_t get_claim_info(name _self, name mosaic_creator, uint64_t tracery, symbol_code commun_code, bool eager) {
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto mosaics_idx = mosaics_table.get_index<"bykey"_n>();
        auto mosaic = mosaics_idx.find(std::make_tuple(mosaic_creator, tracery));
        eosio::check(mosaic != mosaics_idx.end(), "mosaic doesn't exist");
        
        auto now = eosio::current_time_point();
        
        gallery_types::params params_table(_self, commun_code.raw());
        const auto& param = params_table.get(commun_code.raw(), "param does not exists");
        
        claim_info_t ret{mosaic->id, mosaic->reward != 0, now <= mosaic->created + eosio::seconds(param.evaluation_period), param.commun_symbol};
        check(!ret.premature || eager, "evaluation period isn't over yet");
        
        maybe_issue_reward(_self, param);
        
        return ret;
    }
    
public:
    static inline int64_t get_frozen_amount(name gallery_contract_account, name owner, symbol_code sym_code) {
        gallery_types::inclusions inclusions_table(gallery_contract_account, owner.value);
        auto incl = inclusions_table.find(sym_code.raw());
        return incl != inclusions_table.end() ? incl->quantity.amount : 0;
    }
protected:
    void on_transfer(name _self, name from, name to, asset quantity, std::string memo) {
        (void) memo;
        if (_self != to) { return; }

        auto total_reward = quantity.amount;
        auto commun_code = quantity.symbol.code();            
        gallery_types::params params_table(_self, commun_code.raw());
        const auto& param = params_table.get(commun_code.raw(), "param does not exists");
        
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto by_comm_idx = mosaics_table.get_index<"bycommrating"_n>();
        
        auto by_comm_first_itr = by_comm_idx.lower_bound(std::make_tuple(gallery_types::mosaic::ACTIVE, std::numeric_limits<int64_t>::max()));
        auto by_comm_max = param.comm_grades.size();
        auto mosaic_num = 0;
        auto points_sum = 0;
        for (auto by_comm_itr = by_comm_first_itr; (mosaic_num < by_comm_max) &&
                                                   (by_comm_itr != by_comm_idx.end()) && 
                                                   (by_comm_itr->status == gallery_types::mosaic::ACTIVE); by_comm_itr++, mosaic_num++) {
            if (by_comm_itr->points > by_comm_itr->damn_points) {
                points_sum += by_comm_itr->points - by_comm_itr->damn_points;
            }
        }
        std::map<uint64_t, int64_t> ranked_mosaics;
        mosaic_num = 0;
        for (auto by_comm_itr = by_comm_first_itr; (mosaic_num < by_comm_max) &&
                                                   (by_comm_itr != by_comm_idx.end()) && 
                                                   (by_comm_itr->status == gallery_types::mosaic::ACTIVE); by_comm_itr++, mosaic_num++) {
            
            auto cur_grades = param.comm_grades[mosaic_num];
            if (by_comm_itr->points > by_comm_itr->damn_points) {
                cur_grades += safe_prop(param.comm_points_grade_sum, by_comm_itr->points - by_comm_itr->damn_points, points_sum);
            }
            ranked_mosaics[by_comm_itr->id] = cur_grades;
        }
        auto by_lead_idx = mosaics_table.get_index<"byleadrating"_n>();
        
        auto by_lead_max = param.lead_grades.size();
        mosaic_num = 0;
        for (auto by_lead_itr = by_lead_idx.lower_bound(std::make_tuple(gallery_types::mosaic::ACTIVE, std::numeric_limits<int64_t>::max())); 
                                                   (mosaic_num < by_lead_max) &&
                                                   (by_lead_itr != by_lead_idx.end()) && 
                                                   (by_lead_itr->status != gallery_types::mosaic::BANNED) &&
                                                   (by_lead_itr->lead_rating > 0); by_lead_itr++, mosaic_num++) {

            ranked_mosaics[by_lead_itr->id] += param.lead_grades[mosaic_num];
        }
        std::vector<std::pair<uint64_t, int64_t> > top_mosaics;
        top_mosaics.reserve(ranked_mosaics.size());
        for (const auto& m : ranked_mosaics) {
            top_mosaics.emplace_back(m);
        }
        
        auto middle = top_mosaics.begin() + std::min(size_t(param.rewarded_num), top_mosaics.size());
        std::partial_sort(top_mosaics.begin(), middle, top_mosaics.end(),
            [](const std::pair<uint64_t, int64_t>& lhs, const std::pair<uint64_t, int64_t>& rhs) { return lhs.second > rhs.second; });
        
        uint64_t grades_sum = 0;
        for (auto itr = top_mosaics.begin(); itr != middle; itr++) {
            grades_sum += itr->second;
        }
        auto left_reward  = total_reward;
        for (auto itr = top_mosaics.begin(); itr != middle; itr++) {
            auto cur_reward = safe_prop(total_reward, itr->second, grades_sum);
            
            auto mosaic = mosaics_table.find(itr->first);
            mosaics_table.modify(mosaic, name(), [&](auto& item) { item.reward += cur_reward; });
            
            left_reward -= cur_reward;
        }
        auto mosaic = mosaics_table.find(top_mosaics.front().first);
        mosaics_table.modify(mosaic, name(), [&](auto& item) { item.reward += left_reward; });
    }

    void create_gallery(name _self, symbol commun_symbol) {
        require_auth(_self);
        auto commun_code = commun_symbol.code();
        eosio::check(point::balance_exists(config::commun_point_name, _self, commun_code), "point balance does not exist");
        
        gallery_types::params params_table(_self, commun_code.raw());
        eosio::check(params_table.find(commun_code.raw()) == params_table.end(), "the gallery with this symbol already exists");
        
        params_table.emplace(_self, [&](auto& p) { p = {
            .id = commun_code.raw(),
            .commun_symbol = commun_symbol,
            .collection_period = config::default_collection_period,
            .evaluation_period = config::default_evaluation_period,
            .mosaic_active_period = config::default_mosaic_active_period,
            .max_royalty = config::default_max_royalty,
            .ban_threshold = config::default_ban_threshold,
            .rewarded_num = config::default_rewarded_num,
            .comm_points_grade_sum = config::default_comm_points_grade_sum,
            .comm_grades = std::vector<int64_t>(config::default_comm_grades.begin(), config::default_comm_grades.end()),
            .lead_grades = std::vector<int64_t>(config::default_lead_grades.begin(), config::default_lead_grades.end())
        };});
        
        gallery_types::stats stats_table(_self, commun_code.raw());
        eosio::check(stats_table.find(commun_code.raw()) == stats_table.end(), "SYSTEM: stat already exists");
        
        stats_table.emplace(_self, [&](auto& s) { s = {
            .id = commun_code.raw(),
        };});
    }

    void create_mosaic(name _self, name creator, uint64_t tracery, asset quantity, uint16_t royalty, gallery_types::providers_t providers) {
        require_auth(creator);
        auto commun_symbol = quantity.symbol;
        auto commun_code   = commun_symbol.code();
        
        gallery_types::params params_table(_self, commun_code.raw());
        const auto& param = params_table.get(commun_code.raw(), "param does not exists");
        check(royalty <= param.max_royalty, "incorrect royalty");
        check(providers.size() <= config::max_providers_num, "too many providers");
        
        eosio::check(
            get_reserve_amount(asset(get_points_sum(quantity.amount, providers), commun_symbol)) >= get_min_cost(providers), 
            "insufficient quantity");
        
        maybe_issue_reward(_self, param);
        
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        uint64_t mosaic_id = mosaics_table.available_primary_key();
        auto now = eosio::current_time_point();
        
        mosaics_table.emplace(creator, [&]( auto &item ) { item = gallery_types::mosaic {
            .id = mosaic_id,
            .creator = creator,
            .tracery = tracery,
            .royalty = royalty,
            .created = now,
            .gem_count = 0,
            .points = 0,
            .shares = 0,
        };});
        
        auto claim_date = now + eosio::seconds(param.mosaic_active_period);
        
        freeze_in_gems(_self, false, mosaic_id, claim_date, creator, quantity, providers, false);
    }
        
    void add_to_mosaic(name _self, name mosaic_creator, uint64_t tracery, asset quantity, bool damn, name gem_creator, gallery_types::providers_t providers) {
        require_auth(gem_creator);
        auto commun_symbol = quantity.symbol;
        auto commun_code = commun_symbol.code();
        
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto mosaics_idx = mosaics_table.get_index<"bykey"_n>();
        auto mosaic = mosaics_idx.find(std::make_tuple(mosaic_creator, tracery));
        eosio::check(mosaic != mosaics_idx.end(), "mosaic doesn't exist");
        
        gallery_types::params params_table(_self, commun_code.raw());
        const auto& param = params_table.get(commun_code.raw(), "param does not exists");
        check(eosio::current_time_point() <= mosaic->created + eosio::seconds(param.collection_period), "collection period is over");
        
        maybe_issue_reward(_self, param);
        
        auto points_sum = get_points_sum(quantity.amount, providers);
        eosio::check(get_reserve_amount(asset(points_sum, commun_symbol)) >= get_min_cost(providers), "insufficient quantity");
        
        auto shares_abs = damn ?
            calc_bancor_amount(mosaic->damn_points, mosaic->damn_shares, config::shares_cw, points_sum, false) :
            calc_bancor_amount(mosaic->points,      mosaic->shares,      config::shares_cw, points_sum, false);
        
        if (!damn) {
            shares_abs -= pay_royalties(_self, commun_code, mosaic->id, mosaic_creator, safe_pct(mosaic->royalty, shares_abs));
        }

        auto claim_date = mosaic->created + eosio::seconds(param.mosaic_active_period);
        freeze_in_gems(_self, true, mosaic->id, claim_date, gem_creator, quantity, providers, damn, shares_abs);
    }
    
    void claim_gem(name _self, name mosaic_creator, uint64_t tracery, symbol_code commun_code, name gem_owner, name gem_creator, bool eager) {
        auto now = eosio::current_time_point();
        auto claim_info = get_claim_info(_self, mosaic_creator, tracery, commun_code, eager);
        
        gallery_types::gems gems_table(_self, commun_code.raw());
        auto gems_idx = gems_table.get_index<"bykey"_n>();
        auto gem = gems_idx.find(std::make_tuple(claim_info.mosaic_id, gem_owner, gem_creator));
        eosio::check(gem != gems_idx.end(), "nothing to claim");
        gem->check_claim(now, claim_info.has_reward);
        chop_gem(_self, claim_info.commun_symbol, *gem, claim_info.premature);
        gems_idx.erase(gem);
    }
    
    void claim_gems_by_creator(name _self, name mosaic_creator, uint64_t tracery, symbol_code commun_code, name gem_creator, bool eager) {
        auto now = eosio::current_time_point();
        auto claim_info = get_claim_info(_self, mosaic_creator, tracery, commun_code, eager);
        
        gallery_types::gems gems_table(_self, commun_code.raw());
        auto gems_idx = gems_table.get_index<"bycreator"_n>();
        auto gem = gems_idx.lower_bound(std::make_tuple(claim_info.mosaic_id, gem_creator, name()));
        eosio::check((gem != gems_idx.end()) && (gem->mosaic_id == claim_info.mosaic_id) && (gem->creator == gem_creator), "nothing to claim");
        
        while ((gem != gems_idx.end()) && (gem->mosaic_id == claim_info.mosaic_id) && (gem->creator == gem_creator)) {
            gem->check_claim(now, claim_info.has_reward);
            chop_gem(_self, claim_info.commun_symbol, *gem, claim_info.premature);
            gem = gems_idx.erase(gem);
        }
        
    }
    
    void provide_points(name _self, name grantor, name recipient, asset quantity, std::optional<uint16_t> fee) {
        require_auth(grantor);
        
        eosio::check(grantor != recipient, "grantor == recipient");
        gallery_types::provs provs_table(_self, quantity.symbol.code().raw());
        auto provs_index = provs_table.get_index<"bykey"_n>();
        auto prov_itr = provs_index.find(std::make_tuple(grantor, recipient));
        bool exists = prov_itr != provs_index.end();
        bool enable = quantity.amount || fee.has_value();
        
        if (exists && enable) {
            provs_index.modify(prov_itr, name(), [&](auto& item) {
                item.total += quantity.amount;
                if (fee.has_value()) {
                    item.fee = *fee;
                }
            });
        }
        else if(enable) { // !exists
            provs_table.emplace(grantor, [&] (auto &item) { item = gallery_types::provision {
                .id = provs_table.available_primary_key(),
                .grantor = grantor,
                .recipient = recipient,
                .fee = fee.value_or(0),
                .total = quantity.amount
            };});
        }
        else if (exists) {// !enable
            provs_index.erase(prov_itr);
        }
        else { // !exists && !enable
            eosio::check(false, "no points provided");
        }
    }
    
    void advise_mosaics(name _self, symbol_code commun_code, name leader, std::vector<gallery_types::mosaic_key_t> favorites) {
        require_auth(leader);
        gallery_types::params params_table(_self, commun_code.raw());
        const auto& param = params_table.get(commun_code.raw(), "param does not exists");
        eosio::check(check_leader(commun_code, leader), (leader.to_string() + " is not a leader").c_str());
        eosio::check(favorites.size() <= config::advice_weight.size(), "a surfeit of advice");
        
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto mosaics_idx = mosaics_table.get_index<"bykey"_n>();

        gallery_types::advices advices_table(_self, commun_code.raw());
        auto prev_advice = advices_table.find(leader.value);
        if (prev_advice != advices_table.end()) {
            auto prev_weight = config::advice_weight.at(prev_advice->favorites.size() - 1);
            for (auto& m : prev_advice->favorites) {
                auto mosaic = mosaics_idx.find(m);
                if (mosaic != mosaics_idx.end()) {
                    mosaics_idx.modify(mosaic, name(), [&](auto& item) { item.lead_rating -= prev_weight; });
                }
            }
            advices_table.modify(prev_advice, leader, [&](auto& item) { item.favorites = favorites; });
        }
        else {
            advices_table.emplace(leader, [&] (auto &item) { item = gallery_types::advice {
                .id = leader.value,
                .leader = leader,
                .favorites = favorites
            };});
        }
        
        auto weight = config::advice_weight.at(favorites.size() - 1);
        for (auto& m : favorites) {
            auto mosaic = mosaics_idx.find(m);
            eosio::check(mosaic != mosaics_idx.end(), "mosaic doesn't exist");
            mosaics_idx.modify(mosaic, name(), [&](auto& item) { item.lead_rating += weight; });
        }
    }
    void slap_mosaic(name _self, symbol_code commun_code, name leader, name mosaic_creator, uint64_t tracery) {
        require_auth(leader);
        gallery_types::params params_table(_self, commun_code.raw());
        const auto& param = params_table.get(commun_code.raw(), "param does not exists");
        eosio::check(check_leader(commun_code, leader), (leader.to_string() + " is not a leader").c_str());
        
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto mosaics_idx = mosaics_table.get_index<"bykey"_n>();
        auto mosaic = mosaics_idx.find(std::make_tuple(mosaic_creator, tracery));
        eosio::check(mosaic != mosaics_idx.end(), "mosaic doesn't exist");
        
        mosaics_idx.modify(mosaic, name(), [&](auto& item) {
            for (auto& n : item.slaps) {
                eosio::check(n != leader, "already done");
                if (!check_leader(commun_code, n)) {
                    n = name();
                }
            }
            item.slaps.erase(std::remove_if(item.slaps.begin(), item.slaps.end(), [](const name& n) { return n == name(); }), item.slaps.end());
            if (item.slaps.size() >= param.ban_threshold) {
                item.ban();
            }
        });
    }
    
};

class [[eosio::contract("cmmn.gallery")]] gallery : public gallery_base<gallery>, public contract {
public:
    using contract::contract;
    static void prepare_erase(name self, symbol_code commun_code, const gallery_types::mosaic& mosaic) {};
    
    [[eosio::action]] void create(symbol commun_symbol) {
        create_gallery(_self, commun_symbol);
    }

    [[eosio::action]] void createmosaic(name creator, uint64_t tracery, asset quantity, uint16_t royalty, gallery_types::providers_t providers) {
        create_mosaic(_self, creator, tracery, quantity, royalty, providers);
    }
        
    [[eosio::action]] void addtomosaic(name mosaic_creator, uint64_t tracery, asset quantity, bool damn, name gem_creator, gallery_types::providers_t providers) {
        add_to_mosaic(_self, mosaic_creator, tracery, quantity, damn, gem_creator, providers);
    }
    
    [[eosio::action]] void claimgem(name mosaic_creator, uint64_t tracery, symbol_code commun_code, name gem_owner, std::optional<name> gem_creator) {
        claim_gem(_self, mosaic_creator, tracery, commun_code, gem_owner, gem_creator.value_or(gem_owner), false);
    }
    
    [[eosio::action]] void provide(name grantor, name recipient, asset quantity, std::optional<uint16_t> fee) {
        provide_points(_self, grantor, recipient, quantity, fee);
    }
    
    [[eosio::action]] void advise(symbol_code commun_code, name leader, std::vector<gallery_types::mosaic_key_t> favorites) {
        advise_mosaics(_self, commun_code, leader, favorites);
    }
    
    //TODO: [[eosio::action]] void checkadvice (symbol_code commun_code, name leader);
    
    [[eosio::action]] void slap(symbol_code commun_code, name leader, name mosaic_creator, uint64_t tracery) {
        slap_mosaic(_self, commun_code, leader, mosaic_creator, tracery);
    }
    
    void ontransfer(name from, name to, asset quantity, std::string memo) {
        on_transfer(_self, from, to, quantity, memo);
    }      
};
    
} /// namespace commun

#define GALLERY_ACTIONS (create)(createmosaic)(addtomosaic)(claimgem)(provide)(advise)(slap)
