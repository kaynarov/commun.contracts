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

class gallery_base {
public:
    using mosaic_key_t = std::tuple<name, uint64_t>;

protected:

struct structures {
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
        using key_t = mosaic_key_t;
        key_t by_key()const { return std::make_tuple(creator, tracery); }
        using by_rating_t = std::tuple<uint8_t, int64_t>;
        by_rating_t by_comm_rating()const { return std::make_tuple(status, comm_rating); }
        by_rating_t by_lead_rating()const { return std::make_tuple(status, lead_rating); }
    };
    
    struct gem {
        uint64_t id;
        uint64_t mosaic_id;
        time_point claim_date; //the owner must pay off the creator to change this (if owner != creator)
        
        int64_t points;
        int64_t shares;
        
        name owner;
        name creator;
        uint16_t fee = 0;
        
        uint64_t primary_key() const { return id; }
        using key_t = std::tuple<uint64_t, name, name>;
        key_t by_key()const { return std::make_tuple(mosaic_id, owner, creator); }
        key_t by_creator()const { return std::make_tuple(mosaic_id, creator, owner); }
        using by_claim_t = std::tuple<uint64_t, name, time_point>;
        by_claim_t by_claim()const { return std::make_tuple(mosaic_id, owner, claim_date); }
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
        int64_t masaic_active_period;
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
        
        time_point latest_reward;

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
        std::vector<mosaic_key_t> favorites;
        uint64_t primary_key()const { return id; }
    };
};

    using mosaic_id_index = eosio::indexed_by<"mosaicid"_n, eosio::const_mem_fun<structures::mosaic, uint64_t, &structures::mosaic::primary_key> >;
    using mosaic_key_index = eosio::indexed_by<"bykey"_n, eosio::const_mem_fun<structures::mosaic, structures::mosaic::key_t, &structures::mosaic::by_key> >;
    using mosaic_comm_index = eosio::indexed_by<"bycommrating"_n, eosio::const_mem_fun<structures::mosaic, structures::mosaic::by_rating_t, &structures::mosaic::by_comm_rating> >;
    using mosaic_lead_index = eosio::indexed_by<"byleadrating"_n, eosio::const_mem_fun<structures::mosaic, structures::mosaic::by_rating_t, &structures::mosaic::by_lead_rating> >;
    using mosaics = eosio::multi_index<"mosaic"_n, structures::mosaic, mosaic_id_index, mosaic_key_index, mosaic_comm_index, mosaic_lead_index>;
    
    using gem_id_index = eosio::indexed_by<"gemid"_n, eosio::const_mem_fun<structures::gem, uint64_t, &structures::gem::primary_key> >;
    using gem_key_index = eosio::indexed_by<"bykey"_n, eosio::const_mem_fun<structures::gem, structures::gem::key_t, &structures::gem::by_key> >;
    using gem_creator_index = eosio::indexed_by<"bycreator"_n, eosio::const_mem_fun<structures::gem, structures::gem::key_t, &structures::gem::by_creator> >;
    using gem_claim_index = eosio::indexed_by<"byclaim"_n, eosio::const_mem_fun<structures::gem, structures::gem::by_claim_t, &structures::gem::by_claim> >;
    using gems = eosio::multi_index<"gem"_n, structures::gem, gem_id_index, gem_key_index, gem_creator_index, gem_claim_index>;
    
    using inclusions = eosio::multi_index<"inclusion"_n, structures::inclusion>;
    
    using prov_id_index = eosio::indexed_by<"provid"_n, eosio::const_mem_fun<structures::provision, uint64_t, &structures::provision::primary_key> >;
    using prov_key_index = eosio::indexed_by<"bykey"_n, eosio::const_mem_fun<structures::provision, structures::provision::key_t, &structures::provision::by_key> >;
    using provs = eosio::multi_index<"provision"_n, structures::provision, prov_id_index, prov_key_index>;

    using advices = eosio::multi_index<"advice"_n, structures::advice>;
    
    using params = eosio::multi_index<"param"_n, structures::param>;
    using stats = eosio::multi_index<"stat"_n, structures::stat>;    

private:
            
    bool check_leader(symbol_code commun_code, name arg) { return true; }; //TODO
    
    bool transfer(name from, name to, const asset &quantity) {
        if (to && !point::balance_exists(config::commun_point_name, to, quantity.symbol.code())) {
            return false;
        }
        
        if (quantity.amount) {
            //TODO: payment?
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
        
        inclusions inclusions_table(_self, account.value);
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
    
    static int64_t get_points_sum(const asset& quantity, const std::optional<std::vector<std::pair<name, int64_t> > >& providers) {
        int64_t ret = quantity.amount;
        if (providers.has_value()) {
            for (const auto& p : *providers) {
                ret += p.second;
            }
        }
        return ret;
    }
    
    void chop_gem(name _self, symbol commun_symbol, const structures::gem& gem,
                    const std::optional<name>& recipient = std::optional<name>()) {
    
        auto commun_code = commun_symbol.code();
        mosaics mosaics_table(_self, commun_code.raw());
        auto mosaic = mosaics_table.find(gem.mosaic_id);
        eosio::check(mosaic != mosaics_table.end(), "mosaic doesn't exist");
        
        int64_t reward = 0;
        bool damn = gem.shares < 0;
        if (damn == (mosaic->banned())) {
            reward = damn ? 
                safe_prop(mosaic->reward, -gem.shares, mosaic->damn_shares) :
                safe_prop(mosaic->reward,  gem.shares, mosaic->shares);
        }
        asset frozen_points(gem.points, commun_symbol);
        asset reward_points(reward, commun_symbol);
        
        freeze(_self, gem.owner, -frozen_points);
        if (recipient.has_value() && (eosio::current_time_point() > mosaic->created + eosio::seconds(config::dust_holding_period)) &&
                                     (get_reserve_amount(frozen_points + reward_points) < config::dust_cost_limit)) {
            eosio::check(transfer(gem.owner, *recipient, frozen_points) && transfer(_self, *recipient, reward_points), 
                "balance doesn't exist");
        }
        else {
            auto owner_points = reward_points;
            if (gem.creator != gem.owner) {
                auto creator_reward = safe_pct(reward, gem.fee);
                if (transfer(_self, gem.creator, asset(creator_reward, commun_symbol))) {
                    owner_points.amount -= creator_reward;
                }                
                
                provs provs_table(_self, commun_code.raw());
                auto provs_index = provs_table.get_index<"bykey"_n>();
                auto prov_itr = provs_index.find(std::make_tuple(gem.owner, gem.creator));
                if (prov_itr != provs_index.end()) {
                    provs_index.modify(prov_itr, name(), [&](auto& item) {
                        item.total  += owner_points.amount;
                        item.frozen -= frozen_points.amount;
                    });
                }
            }
            if (!transfer(_self, gem.owner, owner_points)) {
                eosio::check(transfer(_self, point::get_issuer(config::commun_point_name, commun_code), owner_points), "the issuer's balance doesn't exist");
            }
        }
        
        if (mosaic->gem_count > 1 || mosaic->lead_rating) {
            mosaics_table.modify(mosaic, name(), [&](auto& item) {
                if (!damn) {
                    item.points -= gem.points;
                    item.shares -= gem.shares;
                }
                else {
                    item.damn_shares -= gem.points;
                    item.damn_shares += gem.shares;
                }
                item.reward -= reward;
                item.gem_count--;
            });
        }
        else {
            mosaics_table.erase(mosaic);
        }
    }
    
    void freeze_points_in_gem(name _self, bool can_refill, symbol commun_symbol, uint64_t mosaic_id, time_point claim_date, 
                              int64_t points, int64_t shares, name owner, name creator = name()) {
        check(points > 0, "points must be positive");
        freeze(_self, owner, asset(points, commun_symbol), creator);
        auto commun_code = commun_symbol.code();
        
        uint16_t fee = 0;
        if (owner != creator) {
            provs provs_table(_self, commun_code.raw());
            auto provs_index = provs_table.get_index<"bykey"_n>();
            auto prov_itr = provs_index.find(std::make_tuple(owner, creator));
            eosio::check(prov_itr != provs_index.end(), "no points provided");
            eosio::check(prov_itr->available() >= points, "not enough provided points");
            provs_index.modify(prov_itr, name(), [&](auto& item) { item.frozen += points; });
            fee = prov_itr->fee;
        }

        gems gems_table(_self, commun_code.raw());
        
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
            
            params params_table(_self, commun_code.raw());
            const auto& param = params_table.get(commun_code.raw(), "param does not exists");
            
            eosio::check(get_reserve_amount(asset(points, commun_symbol)) >= config::min_gem_cost, 
                ("insufficient quantity for " + owner.to_string()).c_str());
                
            uint8_t gem_num = 0;
            auto now = eosio::current_time_point();
            auto gems_idx = gems_table.get_index<"byclaim"_n>();
            auto gem_itr = gems_idx.lower_bound(std::make_tuple(mosaic_id, owner, time_point()));
            while ((gem_num < config::auto_claim_num) &&
                   (gem_itr != gems_idx.end()) && 
                   (gem_itr->mosaic_id == mosaic_id) && 
                   (gem_itr->owner == owner) &&
                   (gem_itr->claim_date < now)) {
                
                chop_gem(_self, commun_symbol, *gem_itr);
                gem_itr = gems_idx.erase(gem_itr);
                gem_num++;
            }
            
            
            gems_table.emplace(creator, [&]( auto &item ) { item = structures::gem {
                .id = gems_table.available_primary_key(),
                .mosaic_id = mosaic_id,
                .claim_date = claim_date,
                .points = points,
                .shares = shares,
                .owner = owner,
                .creator = creator,
                .fee = fee
            };});
        }
        
        mosaics mosaics_table(_self, commun_code.raw());
        auto mosaic = mosaics_table.find(mosaic_id);
        eosio::check(mosaic != mosaics_table.end(), "mosaic doesn't exist");
        
        mosaics_table.modify(mosaic, name(), [&](auto& item) {
            if (shares < 0) {
                item.damn_points += points;
                item.damn_shares -= shares;
            }
            else {
                item.points += points;
                item.shares += shares;

            }
            if (!refilled) {
                eosio::check(item.gem_count < std::numeric_limits<decltype(item.gem_count)>::max(), "gem_count overflow");
                item.gem_count++;
            }
        });
    }
                              
    int64_t pay_royalties(name _self, symbol_code commun_code, uint64_t mosaic_id, name mosaic_creator, int64_t shares) {
    
        gems gems_table(_self, commun_code.raw());
    
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
        
        mosaics mosaics_table(_self, commun_code.raw());
        auto mosaic = mosaics_table.find(mosaic_id);
        eosio::check(mosaic != mosaics_table.end(), "mosaic doesn't exist");
        
        mosaics_table.modify(mosaic, name(), [&](auto& item) { item.shares += ret; });
        
        return ret;
    }
    
    void maybe_issue_reward(name _self, const structures::param& param) {
        auto commun_code = param.commun_symbol.code();
        auto now = eosio::current_time_point();
        
        stats stats_table(_self, commun_code.raw());
        const auto& stat = stats_table.get(commun_code.raw(), "SYSTEM: stat does not exists");
        
        eosio::check(now >= stat.latest_reward, "SYSTEM: incorrect latest_reward");
        
        int64_t passed_seconds = (now - stat.latest_reward).to_seconds();
        if (passed_seconds >= config::reward_mosaics_period) {
            action(
                permission_level{config::commun_emit_name, config::mscsreward_perm_name},
                config::commun_emit_name,
                "mscsreward"_n,
                std::make_tuple(param.commun_symbol, passed_seconds, _self)
            ).send();
            stats_table.modify(stat, name(), [&]( auto& s) { s.latest_reward  = now; });
        }
    }
    
public:
    static inline int64_t get_frozen_amount(name gallery_contract_account, name owner, symbol_code sym_code) {
        inclusions inclusions_table(gallery_contract_account, owner.value);
        auto incl = inclusions_table.find(sym_code.raw());
        return incl != inclusions_table.end() ? incl->quantity.amount : 0;
    }
protected:
    void on_transfer(name _self, name from, name to, asset quantity, std::string memo) {
        (void) memo;
        if (_self != to) { return; }

        auto total_reward = quantity.amount;
        auto commun_code = quantity.symbol.code();            
        params params_table(_self, commun_code.raw());
        const auto& param = params_table.get(commun_code.raw(), "param does not exists");
        
        mosaics mosaics_table(_self, commun_code.raw());
        auto by_comm_idx = mosaics_table.get_index<"bycommrating"_n>();
        
        auto by_comm_first_itr = by_comm_idx.lower_bound(std::make_tuple(structures::mosaic::ACTIVE, std::numeric_limits<int64_t>::max()));
        auto by_comm_max = param.comm_grades.size();
        auto mosaic_num = 0;
        auto points_sum = 0;
        for (auto by_comm_itr = by_comm_first_itr; (mosaic_num < by_comm_max) &&
                                                   (by_comm_itr != by_comm_idx.end()) && 
                                                   (by_comm_itr->status == structures::mosaic::ACTIVE); by_comm_itr++, mosaic_num++) {
            if (by_comm_itr->points > by_comm_itr->damn_points) {
                points_sum += by_comm_itr->points - by_comm_itr->damn_points;
            }
        }
        std::map<uint64_t, int64_t> ranked_mosaics;
        mosaic_num = 0;
        for (auto by_comm_itr = by_comm_first_itr; (mosaic_num < by_comm_max) &&
                                                   (by_comm_itr != by_comm_idx.end()) && 
                                                   (by_comm_itr->status == structures::mosaic::ACTIVE); by_comm_itr++, mosaic_num++) {
            
            auto cur_grades = param.comm_grades[mosaic_num];
            if (by_comm_itr->points > by_comm_itr->damn_points) {
                cur_grades += safe_prop(param.comm_points_grade_sum, by_comm_itr->points - by_comm_itr->damn_points, points_sum);
            }
            ranked_mosaics[by_comm_itr->id] = cur_grades;
        }
        auto by_lead_idx = mosaics_table.get_index<"byleadrating"_n>();
        
        auto by_lead_max = param.lead_grades.size();
        mosaic_num = 0;
        for (auto by_lead_itr = by_lead_idx.lower_bound(std::make_tuple(structures::mosaic::ACTIVE, std::numeric_limits<int64_t>::max())); 
                                                   (mosaic_num < by_lead_max) &&
                                                   (by_lead_itr != by_lead_idx.end()) && 
                                                   (by_lead_itr->status != structures::mosaic::BANNED); by_lead_itr++, mosaic_num++) {

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
            auto cur_reward = safe_prop(total_reward, itr->second, total_reward);
            
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
        
        params params_table(_self, commun_code.raw());
        eosio::check(params_table.find(commun_code.raw()) == params_table.end(), "the gallery with this symbol already exists");
        
        params_table.emplace(_self, [&](auto& p) { p = {
            .id = commun_code.raw(),
            .commun_symbol = commun_symbol,
            .collection_period = config::default_collection_period,
            .evaluation_period = config::default_evaluation_period,
            .masaic_active_period = config::default_masaic_active_period,
            .max_royalty = config::default_max_royalty,
            .ban_threshold = config::default_ban_threshold,
            .rewarded_num = config::default_rewarded_num,
            .comm_points_grade_sum = config::default_comm_points_grade_sum,
            .comm_grades = std::vector<int64_t>(config::default_comm_grades.begin(), config::default_comm_grades.end()),
            .lead_grades = std::vector<int64_t>(config::default_lead_grades.begin(), config::default_lead_grades.end())
        };});
        
        stats stats_table(_self, commun_code.raw());
        eosio::check(stats_table.find(commun_code.raw()) == stats_table.end(), "SYSTEM: stat already exists");
        
        stats_table.emplace(_self, [&](auto& s) { s = {
            .id = commun_code.raw(),
            .latest_reward = eosio::current_time_point()
        };});
    }

    void create_mosaic(name _self, name creator, uint64_t tracery, asset quantity, uint16_t royalty,
                                        std::optional<std::vector<std::pair<name, int64_t> > > providers) {
        require_auth(creator);
        auto commun_symbol = quantity.symbol;
        auto commun_code   = commun_symbol.code();
        
        params params_table(_self, commun_code.raw());
        const auto& param = params_table.get(commun_code.raw(), "param does not exists");
        check(royalty <= param.max_royalty, "incorrect royalty");
        check(!providers.has_value() || providers->size() <= config::max_providers_num, "too many providers");
        check(quantity.amount || providers.has_value(), "can't freeze zero points");
        
        maybe_issue_reward(_self, param);
        
        mosaics mosaics_table(_self, commun_code.raw());
        uint64_t mosaic_id = mosaics_table.available_primary_key();
        auto now = eosio::current_time_point();
        
        mosaics_table.emplace(creator, [&]( auto &item ) { item = structures::mosaic {
            .id = mosaic_id,
            .creator = creator,
            .tracery = tracery,
            .royalty = royalty,
            .created = now,
            .gem_count = 0,
            .points = 0,
            .shares = 0,
        };});
        
        auto claim_date = now + eosio::seconds(param.masaic_active_period);
        
        if (providers.has_value()) {
            for (const auto& p : *providers) {
                freeze_points_in_gem(_self, false, commun_symbol, mosaic_id, claim_date, p.second, p.second, p.first, creator);
            }
        }
        
        if (quantity.amount) {
            freeze_points_in_gem(_self, false, commun_symbol, mosaic_id, claim_date, quantity.amount, quantity.amount, creator, creator);
        }
    }
        
    void add_to_mosaic(name _self, name mosaic_creator, uint64_t tracery, asset quantity, bool damn, name gem_creator, 
                                       std::optional<std::vector<std::pair<name, int64_t> > > providers) {
        require_auth(gem_creator);
        auto commun_symbol = quantity.symbol;
        auto commun_code = commun_symbol.code();
        
        mosaics mosaics_table(_self, commun_code.raw());
        auto mosaics_idx = mosaics_table.get_index<"bykey"_n>();
        auto mosaic = mosaics_idx.find(std::make_tuple(mosaic_creator, tracery));
        eosio::check(mosaic != mosaics_idx.end(), "mosaic doesn't exist");
        
        params params_table(_self, commun_code.raw());
        const auto& param = params_table.get(commun_code.raw(), "param does not exists");
        check(eosio::current_time_point() <= mosaic->created + eosio::seconds(param.collection_period), "collection period is over");
        
        maybe_issue_reward(_self, param);
        
        auto points_sum = get_points_sum(quantity, providers);
        check(points_sum, "can't freeze zero points");
        auto shares_abs = damn ?
            calc_bancor_amount(mosaic->damn_points, mosaic->damn_shares, config::shares_cw, points_sum) :
            calc_bancor_amount(mosaic->points,      mosaic->shares,      config::shares_cw, points_sum);
        
        if (!damn) {
            shares_abs -= pay_royalties(_self, commun_code, mosaic->id, mosaic_creator, safe_pct(mosaic->royalty, shares_abs));
        }

        auto claim_date = mosaic->created + eosio::seconds(param.masaic_active_period);
        
        if (providers.has_value()) {
            for (const auto& p : *providers) {
                auto cur_shares_abs = safe_prop(shares_abs, p.second, points_sum);
                freeze_points_in_gem(_self, true, commun_symbol, mosaic->id, claim_date, p.second, damn ? -cur_shares_abs : cur_shares_abs, p.first, gem_creator);
            }
        }
        
        if (quantity.amount) {
            auto cur_shares_abs = safe_prop(shares_abs, quantity.amount, points_sum);
            freeze_points_in_gem(_self, true, commun_symbol, mosaic->id, claim_date, quantity.amount, damn ? -cur_shares_abs : cur_shares_abs, gem_creator, gem_creator);
        }
    }
    
    void claim_gem(name _self, name mosaic_creator, uint64_t tracery, symbol_code commun_code, name gem_owner, std::optional<name> gem_creator, std::optional<name> recipient) {
        mosaics mosaics_table(_self, commun_code.raw());
        auto mosaics_idx = mosaics_table.get_index<"bykey"_n>();
        auto mosaic = mosaics_idx.find(std::make_tuple(mosaic_creator, tracery));
        eosio::check(mosaic != mosaics_idx.end(), "mosaic doesn't exist");
        
        auto now = eosio::current_time_point();
        
        params params_table(_self, commun_code.raw());
        const auto& param = params_table.get(commun_code.raw(), "param does not exists");
        
        maybe_issue_reward(_self, param);
        
        check(now > mosaic->created + eosio::seconds(param.evaluation_period), "evaluation period isn't over yet");
        gems gems_table(_self, commun_code.raw());
        auto gems_idx = gems_table.get_index<"bykey"_n>();
        auto gem = gems_idx.find(std::make_tuple(mosaic->id, gem_owner, gem_creator.value_or(gem_owner)));
        eosio::check(gem != gems_idx.end(), "nothing to claim");
        if (now < gem->claim_date) {
            require_auth(gem_owner);
        }
        chop_gem(_self, param.commun_symbol, *gem, recipient);
        gems_idx.erase(gem);
    }
    
    void provide_points(name _self, name grantor, name recipient, asset quantity, std::optional<uint16_t> fee) {
        require_auth(grantor);
        provs provs_table(_self, quantity.symbol.code().raw());
        auto provs_index = provs_table.get_index<"bykey"_n>();
        auto prov_itr = provs_index.find(std::make_tuple(grantor, recipient));
        bool exists = prov_itr != provs_index.end();
        bool enable = !quantity.amount && !fee.has_value();
        
        if (exists && enable) {
            provs_index.modify(prov_itr, name(), [&](auto& item) {
                item.total += quantity.amount;
                if (fee.has_value()) {
                    item.fee = *fee;
                }
            });
        }
        else if(enable) { // !exists
            provs_table.emplace(grantor, [&] (auto &item) { item = structures::provision {
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
    
    void advise_mosaics(name _self, symbol_code commun_code, name leader, std::vector<mosaic_key_t> favorites) {
        require_auth(leader);
        params params_table(_self, commun_code.raw());
        const auto& param = params_table.get(commun_code.raw(), "param does not exists");
        eosio::check(check_leader(commun_code, leader), (leader.to_string() + " is not a leader").c_str());
        eosio::check(favorites.size() <= config::advice_weight.size(), "a surfeit of advice");
        
        mosaics mosaics_table(_self, commun_code.raw());
        auto mosaics_idx = mosaics_table.get_index<"bykey"_n>();

        advices advices_table(_self, commun_code.raw());
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
            advices_table.emplace(leader, [&] (auto &item) { item = structures::advice {
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
        params params_table(_self, commun_code.raw());
        const auto& param = params_table.get(commun_code.raw(), "param does not exists");
        eosio::check(check_leader(commun_code, leader), (leader.to_string() + " is not a leader").c_str());
        
        mosaics mosaics_table(_self, commun_code.raw());
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

class [[eosio::contract("cmmn.gallery")]] gallery : public gallery_base, public contract {
public:
    using contract::contract;
    
    [[eosio::action]] void create(symbol commun_symbol) {
        create_gallery(_self, commun_symbol);
    }

    [[eosio::action]] void createmosaic(name creator, uint64_t tracery, asset quantity, uint16_t royalty,
                                        std::optional<std::vector<std::pair<name, int64_t> > > providers) {
        create_mosaic(_self, creator, tracery, quantity, royalty, providers);
    }
        
    [[eosio::action]] void addtomosaic(name mosaic_creator, uint64_t tracery, asset quantity, bool damn, name gem_creator, 
                                       std::optional<std::vector<std::pair<name, int64_t> > > providers) {
        add_to_mosaic(_self, mosaic_creator, tracery, quantity, damn, gem_creator, providers);
    }
    
    [[eosio::action]] void claimgem(name mosaic_creator, uint64_t tracery, symbol_code commun_code, name gem_owner, std::optional<name> gem_creator, std::optional<name> recipient) {
        claim_gem(_self, mosaic_creator, tracery, commun_code, gem_owner, gem_creator, recipient);
    }
    
    [[eosio::action]] void provide(name grantor, name recipient, asset quantity, std::optional<uint16_t> fee) {
        provide_points(_self, grantor, recipient, quantity, fee);
    }
    
    [[eosio::action]] void advise(symbol_code commun_code, name leader, std::vector<mosaic_key_t> favorites) {
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
