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
#include <commun.ctrl/commun.ctrl.hpp>
#include <commun.point/commun.point.hpp>
#include <commun.emit/commun.emit.hpp>
#include <commun.emit/config.hpp>
#include <commun.list/commun.list.hpp>
#include <eosio/event.hpp>

namespace commun {

using std::string;
using structures::opus_info;

namespace gallery_types {
    using providers_t = std::vector<std::pair<name, int64_t> >;

    /**
     * \brief struct represents a mosaic table in a db
     * \ingroup publish_tables
     *
     * Contains information about a mosaic
     */
    struct mosaic {
   
        mosaic() = default;
        uint64_t tracery; //!< the mosaic tracery, used as primary key
        name creator;     //!< the mosaic creator
        
        name opus;        //!< the mosaic opus, an example: post/comment for publications
        uint16_t royalty;

        time_point lock_date = time_point();
        time_point close_date;
        uint16_t gem_count;
        
        int64_t points;
        int64_t shares;
        int64_t damn_points = 0;
        int64_t damn_shares = 0;
        
        int64_t reward = 0;
        
        int64_t comm_rating = 0;
        int64_t lead_rating = 0;
        
        bool meritorious = false;
        bool active = true;
        bool banned = false;
        bool locked = false;

        void lock() {
            locked = true;
            active = false;
            meritorious = false;
        }

        void unlock() {
            locked = false;
            active = true;
            meritorious = true;
        }

        void ban() {
            banned = true;
            locked = false;
            active = false;
            meritorious = false;
        };
       
        uint64_t primary_key() const { return tracery; }
        using by_rating_t = std::tuple<bool, int64_t, int64_t>;
        by_rating_t by_comm_rating()const { return std::make_tuple(active, comm_rating, lead_rating); }
        using by_lead_rating_t = std::tuple<bool, bool, int64_t, int64_t>;
        by_lead_rating_t by_lead_rating()const { return std::make_tuple(banned, locked, lead_rating, comm_rating); }
        using by_close_t = std::tuple<uint8_t, time_point>;
        by_close_t by_close()const { return std::make_tuple(active, close_date); }
    };
    
    struct gem {
        uint64_t id;
        uint64_t tracery;
        time_point claim_date;
        
        int64_t points;
        int64_t shares;
        
        int64_t pledge_points;
        
        name owner;
        name creator;
        
        uint64_t primary_key() const { return id; }
        using key_t = std::tuple<uint64_t, name, name>;
        key_t by_key()const { return std::make_tuple(tracery, owner, creator); }
        key_t by_creator()const { return std::make_tuple(tracery, creator, owner); }
        using by_claim_t = std::tuple<name, time_point>;
        by_claim_t by_claim()const { return std::make_tuple(owner, claim_date); }
        time_point by_claim_joint()const { return claim_date; }
    };
    
    struct [[eosio::table]] inclusion {
        asset quantity;
            // just an idea:
            // use as inclusion not only points, but also other gems. 
            // this will allow to buy shares in the mosaics without liquid points, creating more sophisticated collectables
            // (pntinclusion / geminclusion)
        
        uint64_t primary_key()const { return quantity.symbol.code().raw(); }
    };
    
    struct [[eosio::table]] stat {
        uint64_t id;
        int64_t unclaimed = 0;
        int64_t retained = 0;

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
        name leader;
        std::set<uint64_t> favorites;
        uint64_t primary_key()const { return leader.value; }
    };
    
    using mosaic_id_index = eosio::indexed_by<"mosaicid"_n, eosio::const_mem_fun<gallery_types::mosaic, uint64_t, &gallery_types::mosaic::primary_key> >;
    using mosaic_comm_index = eosio::indexed_by<"bycommrating"_n, eosio::const_mem_fun<gallery_types::mosaic, gallery_types::mosaic::by_rating_t, &gallery_types::mosaic::by_comm_rating> >;
    using mosaic_lead_index = eosio::indexed_by<"byleadrating"_n, eosio::const_mem_fun<gallery_types::mosaic, gallery_types::mosaic::by_lead_rating_t, &gallery_types::mosaic::by_lead_rating> >;
    using mosaic_close_index = eosio::indexed_by<"byclose"_n, eosio::const_mem_fun<gallery_types::mosaic, gallery_types::mosaic::by_close_t, &gallery_types::mosaic::by_close> >;
    using mosaics = eosio::multi_index<"mosaic"_n, gallery_types::mosaic, mosaic_id_index, mosaic_comm_index, mosaic_lead_index, mosaic_close_index>;
    
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

    using stats = eosio::multi_index<"stat"_n, gallery_types::stat>;
    
namespace events {
    
    struct mosaic_key {
        symbol_code commun_code;
        uint64_t tracery;
    };
    
    struct mosaic {
        uint64_t tracery;
        name creator;
        uint16_t gem_count;
        int64_t shares;
        int64_t damn_shares;
        asset reward;
        bool banned;
    };
    
    struct gem {
        uint64_t tracery;
        name owner;
        name creator;
        asset points;
        asset pledge_points;
        bool damn;
        int64_t shares;
    };
    
    struct chop {
        uint64_t tracery;
        name owner;
        name creator;
        asset reward;
    };
    
    struct top {
        symbol_code commun_code;
        uint64_t tracery;
        uint16_t place;
        int64_t comm_rating;
        int64_t lead_rating;
    };
    
}// events
}// gallery_types


template<typename T>
class gallery_base {
    void send_mosaic_event(name _self, symbol commun_symbol, const gallery_types::mosaic& mosaic) {
        gallery_types::events::mosaic data {
            .tracery = mosaic.tracery,
            .creator = mosaic.creator,
            .gem_count = mosaic.gem_count,
            .shares = mosaic.shares,
            .damn_shares = mosaic.damn_shares,
            .reward = asset(mosaic.reward, commun_symbol),
            .banned = mosaic.banned
        };
        eosio::event(_self, "mosaicstate"_n, data).send();
    }
    
    void send_mosaic_erase_event(name _self, symbol_code commun_code, uint64_t tracery) {
        gallery_types::events::mosaic_key data {
            .commun_code = commun_code,
            .tracery = tracery
        };
        eosio::event(_self, "mosaicerase"_n, data).send();
    }
    
    void send_gem_event(name _self, symbol commun_symbol, const gallery_types::gem& gem) {
        gallery_types::events::gem data {
            .tracery = gem.tracery,
            .owner = gem.owner,
            .creator = gem.creator,
            .points = asset(gem.points, commun_symbol),
            .pledge_points = asset(gem.pledge_points, commun_symbol),
            .damn = gem.shares < 0,
            .shares = std::abs(gem.shares)
        };
        eosio::event(_self, "gemstate"_n, data).send();
    }
    
    void send_chop_event(name _self, const gallery_types::gem& gem, asset reward) {
        gallery_types::events::chop data {
            .tracery = gem.tracery,
            .owner = gem.owner,
            .creator = gem.creator,
            .reward = reward
        };
        eosio::event(_self, "chopevent"_n, data).send();
    }
    
    void send_top_event(name _self, symbol_code commun_code, const gallery_types::mosaic& mosaic, uint16_t place) {
        gallery_types::events::top data {
            .commun_code = commun_code,
            .tracery = mosaic.tracery,
            .place = place,
            .comm_rating = mosaic.comm_rating,
            .lead_rating = mosaic.lead_rating
        };
        eosio::event(_self, "topstate"_n, data).send();
    }
    
private:
    bool send_points(name from, name to, const asset &quantity) {
        if (to && !point::balance_exists(config::point_name, to, quantity.symbol.code())) {
            return false;
        }
        
        if (quantity.amount) {
            action(
                permission_level{config::point_name, config::transfer_permission},
                config::point_name,
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
        auto balance_exists = point::balance_exists(config::point_name, account, commun_code);
        eosio::check(balance_exists, quantity.amount > 0 ? "balance doesn't exist" : "SYSTEM: points are frozen while balance doesn't exist");

        auto balance_amount = point::get_balance(config::point_name, account, commun_code).amount;
        
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
        return point::get_reserve_quantity(config::point_name, quantity, false).amount;
    }
    
    template<typename GemIndex, typename GemItr>
    bool chop_gem(name _self, symbol commun_symbol, GemIndex& gem_idx, GemItr& gem_itr, bool by_user, bool has_reward, bool no_rewards = false) {
        const auto& gem = *gem_itr;
        auto commun_code = commun_symbol.code();
        auto& community = commun_list::get_community(config::list_name, commun_code);

        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto mosaic = mosaics_table.find(gem.tracery);
        eosio::check(mosaic != mosaics_table.end(), "mosaic doesn't exist");
        bool ready_to_claim = (mosaic->close_date + eosio::seconds(community.moderation_period + community.active_period)) <= eosio::current_time_point();
        ready_to_claim = ready_to_claim && (gem.claim_date != config::eternity);
        if (by_user) {
            eosio::check(ready_to_claim || has_auth(gem.owner) 
                || (has_auth(gem.creator) && !has_reward && gem.claim_date != config::eternity), "lack of necessary authority");
        } else if (!ready_to_claim) {
            gem_idx.modify(gem_itr, eosio::same_payer, [&](auto& item) {
                item.claim_date = mosaic->close_date;
            });
            return false;
        }

        int64_t reward = 0;
        bool damn = gem.shares < 0;
        if (!no_rewards && damn == mosaic->banned) {
            reward = damn ? 
                safe_prop(mosaic->reward, -gem.shares, mosaic->damn_shares) :
                safe_prop(mosaic->reward,  gem.shares, mosaic->shares);
        }
        asset frozen_points(gem.points + gem.pledge_points, commun_symbol);
        asset reward_points(reward, commun_symbol);
        
        freeze(_self, gem.owner, -frozen_points);
        send_chop_event(_self, gem, reward_points);

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
        if (!send_points(_self, gem.owner, reward_points)) {
            eosio::check(send_points(_self, point::get_issuer(config::point_name, commun_code), reward_points), "the issuer's balance doesn't exist");
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
            send_mosaic_event(_self, commun_symbol, *mosaic);
        }
        else {
            gallery_types::stats stats_table(_self, commun_code.raw());
            const auto stat = get_stat(_self, stats_table, commun_code);
            stats_table.modify(stat, name(), [&]( auto& s) { s.unclaimed += mosaic->reward - reward; });
            if (mosaic->active) {
                T::deactivate(_self, commun_code, *mosaic);
            }
            send_mosaic_erase_event(_self, commun_code, mosaic->tracery);
            mosaics_table.erase(mosaic);
        }
        return true;
    }
    
    void freeze_points_in_gem(name _self, bool creating, symbol commun_symbol, uint64_t tracery, time_point claim_date, 
                              int64_t points, int64_t shares, int64_t pledge_points, name owner, name creator) {
        check(points >= 0, "SYSTEM: points can't be negative");
        check(pledge_points >= 0, "SYSTEM: pledge_points can't be negative");
        if (!shares && !points && !pledge_points && !creating) {
            return;
        }
        
        auto commun_code = commun_symbol.code();
        
        gallery_types::gems gems_table(_self, commun_code.raw());
        
        bool refilled = false;
        
        if (!creating) {
            auto gems_idx = gems_table.get_index<"bykey"_n>();
            auto gem = gems_idx.find(std::make_tuple(tracery, owner, creator));
            if (gem != gems_idx.end()) {
                eosio::check((shares < 0) == (gem->shares < 0), "gem type mismatch");
                eosio::check(!pledge_points, "SYSTEM: pledge_points must be zero");
                gems_idx.modify(gem, name(), [&](auto& item) {
                    item.points += points;
                    item.shares += shares;
                });
                send_gem_event(_self, commun_symbol, *gem);
                refilled = true;
            }
        }
        
        if (!refilled) {
            auto& community = commun_list::get_community(config::list_name, commun_code);

            uint8_t gem_num = 0;
            auto max_claim_date = eosio::current_time_point() - eosio::seconds(community.moderation_period + community.active_period);
            auto claim_idx = gems_table.get_index<"byclaim"_n>();
            auto chop_gem_of = [&](name account) {
                auto gem_itr = claim_idx.lower_bound(std::make_tuple(account, time_point()));
                while ((gem_itr != claim_idx.end()) && (gem_itr->owner == account) && (gem_itr->claim_date < max_claim_date)) {
                    if (!chop_gem(_self, commun_symbol, claim_idx, gem_itr, false, true)) {
                        ++gem_itr;
                        continue;
                    }
                    claim_idx.erase(gem_itr);
                    ++gem_num;
                    break;
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
                if (!chop_gem(_self, commun_symbol, joint_idx, gem_itr, false, true, true)) {
                    ++gem_itr;
                    continue;
                }
                gem_itr = joint_idx.erase(gem_itr);
                ++gem_num;
            }
            
            gems_table.emplace(creator, [&]( auto &item ) { 
                item = gallery_types::gem {
                    .id = gems_table.available_primary_key(),
                    .tracery = tracery,
                    .claim_date = claim_date,
                    .points = points,
                    .shares = shares,
                    .pledge_points = pledge_points,
                    .owner = owner,
                    .creator = creator
                };
                send_gem_event(_self, commun_symbol, item);
            });
        }
        freeze(_self, owner, asset(points + pledge_points, commun_symbol), creator);
        
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto mosaic = mosaics_table.find(tracery);
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

    int64_t pay_royalties(name _self, symbol commun_symbol, uint64_t tracery, name mosaic_creator, int64_t shares) {
    
        auto commun_code = commun_symbol.code();
        gallery_types::gems gems_table(_self, commun_code.raw());
    
        auto gems_idx = gems_table.get_index<"bycreator"_n>();
        auto first_gem_itr = gems_idx.lower_bound(std::make_tuple(tracery, mosaic_creator, name()));
        int64_t pre_shares_sum = 0;
        for (auto gem_itr = first_gem_itr; (gem_itr != gems_idx.end()) && 
                                           (gem_itr->tracery == tracery) && 
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
                                           (gem_itr->tracery == tracery) && 
                                           (gem_itr->creator == mosaic_creator); ++gem_itr) {
            if (gem_itr->shares > 0) {
                int64_t cur_shares = safe_prop(shares, gem_itr->shares, pre_shares_sum);
                gems_idx.modify(gem_itr, name(), [&](auto& item) {
                    item.shares += cur_shares;
                });
                send_gem_event(_self, commun_symbol, *gem_itr);
                ret += cur_shares;
            }
        }
        
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto mosaic = mosaics_table.find(tracery);
        eosio::check(mosaic != mosaics_table.end(), "mosaic doesn't exist");
        
        mosaics_table.modify(mosaic, name(), [&](auto& item) { item.shares += ret; });
        
        return ret;
    }

    void freeze_in_gems(name _self, bool creating, uint64_t tracery, time_point claim_date, name creator, 
                        asset quantity, gallery_types::providers_t providers, bool damn, 
                        int64_t points_sum, int64_t shares_abs, int64_t pledge_points) {

        int64_t total_shares_fee = 0;
        auto commun_symbol = quantity.symbol;
        auto commun_code = commun_symbol.code();          
        
        gallery_types::provs provs_table(_self, commun_code.raw());
        auto provs_index = provs_table.get_index<"bykey"_n>();
        
        auto left_pledge = pledge_points;
        auto left_points = points_sum;
        
        for (const auto& p : providers) {
            auto prov_itr = provs_index.find(std::make_tuple(p.first, creator));
            eosio::check(prov_itr != provs_index.end(), "no points provided");
            
            int64_t cur_pledge = safe_prop(left_pledge, p.second, left_points);
            int64_t cur_points = p.second - cur_pledge;
            int64_t cur_shares_abs = safe_prop(shares_abs, p.second, points_sum);
            int64_t cur_shares_fee = safe_pct(prov_itr->fee, cur_shares_abs);
            cur_shares_abs   -= cur_shares_fee;
            total_shares_fee += cur_shares_fee;

            freeze_points_in_gem(_self, creating, commun_symbol, tracery, claim_date, 
                cur_points, damn ? -cur_shares_abs : cur_shares_abs, cur_pledge, p.first, creator);
            
            eosio::check(prov_itr->available() >= p.second, "not enough provided points");
            provs_index.modify(prov_itr, name(), [&](auto& item) { item.frozen += p.second; });
            
            left_pledge -= cur_pledge;
            left_points -= p.second;
        }
        
        if (quantity.amount || total_shares_fee || creating) {
            int64_t cur_pledge = std::min(left_pledge, quantity.amount);
            int64_t cur_points = quantity.amount - cur_pledge;
            int64_t cur_shares_abs = safe_prop(shares_abs, quantity.amount, points_sum);
            cur_shares_abs += total_shares_fee;
            
            freeze_points_in_gem(_self, creating, commun_symbol, tracery, claim_date, 
                cur_points, damn ? -cur_shares_abs : cur_shares_abs, cur_pledge, creator, creator);
        }
        
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto mosaic = mosaics_table.find(tracery);
        send_mosaic_event(_self, commun_symbol, *mosaic);

        archive_old_mosaics(_self, commun_code);
    }
    
    struct claim_info_t {
        uint64_t tracery;
        bool has_reward;
        bool premature;
        symbol commun_symbol;
    };
    
    claim_info_t get_claim_info(name _self, uint64_t tracery, symbol_code commun_code, bool eager) {
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        const auto& mosaic = mosaics_table.get(tracery, "mosaic doesn't exist");
        
        auto now = eosio::current_time_point();

        auto& community = commun_list::get_community(config::list_name, commun_code);
        
        claim_info_t ret{mosaic.tracery, mosaic.reward != 0, now <= mosaic.close_date + eosio::seconds(community.moderation_period + community.active_period), community.commun_symbol};
        check(!ret.premature || eager, "moderation period isn't over yet");
        
        emit::maybe_issue_reward(commun_code, _self);
        
        return ret;
    }
    
    void set_gem_holder(name _self, uint64_t tracery, symbol_code commun_code, name gem_owner, name gem_creator, name holder) {
        require_auth(gem_owner);
        auto& community = commun_list::get_community(config::list_name, commun_code);
        
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        const auto& mosaic = mosaics_table.get(tracery, "mosaic doesn't exist");
        gallery_types::gems gems_table(_self, commun_code.raw());
        auto gems_idx = gems_table.get_index<"bykey"_n>();
        auto gem = gems_idx.find(std::make_tuple(tracery, gem_owner, gem_creator));
        eosio::check(gem != gems_idx.end(), "gem doesn't exist");
        eosio::check(gem->claim_date != config::eternity || gem_owner != holder, "gem is already held");
        
        if (gem_owner != holder && gem->points) {
            require_auth(holder);
            asset frozen_points(gem->points + gem->pledge_points, community.commun_symbol);
            freeze(_self, gem_owner, -frozen_points);
            freeze(_self, holder, frozen_points);
        }
        
        gems_idx.modify(gem, holder, [&](auto& item) {
            item.owner = holder;
            item.claim_date = config::eternity;
        });
    }
    
    void archive_old_mosaics(name _self, symbol_code commun_code) {
        auto community = commun_list::get_community(config::list_name, commun_code);

        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto mosaics_idx = mosaics_table.get_index<"byclose"_n>();
        
        auto now = eosio::current_time_point();
        auto max_close_date = now - (eosio::seconds(community.moderation_period) + eosio::seconds(community.active_period));
        
        for (size_t i = 0; i < config::auto_archives_num; i++) {
            auto mosaic = mosaics_idx.lower_bound(std::make_tuple(true, time_point()));
            if ((mosaic == mosaics_idx.end()) || (mosaic->close_date > max_close_date)) {
                break;
            }
            mosaics_idx.modify(mosaic, name(), [&](auto& item) { item.active = false; });
            T::deactivate(_self, commun_code, *mosaic);
        }
    }
    
public:
    static inline int64_t get_frozen_amount(name gallery_contract_account, name owner, symbol_code sym_code) {
        gallery_types::inclusions inclusions_table(gallery_contract_account, owner.value);
        auto incl = inclusions_table.find(sym_code.raw());
        return incl != inclusions_table.end() ? incl->quantity.amount : 0;
    }
protected:

    static int64_t get_points_sum(int64_t quantity_amount, const gallery_types::providers_t& providers) {
        int64_t ret = quantity_amount;
        for (const auto& p : providers) {
            check(p.second > 0, "provided points must be positive");
            ret += p.second;
        }

        return ret;
    }

    auto get_stat(name _self, gallery_types::stats& stats_table, symbol_code commun_code) {
        const auto stats_itr = stats_table.find(commun_code.raw());
        if (stats_itr != stats_table.end()) {
            return stats_itr;
        }
        return stats_table.emplace(_self, [&](auto& s) { s = {
            .id = commun_code.raw(),
        };});
    }

    void on_points_transfer(name _self, name from, name to, asset quantity, std::string memo) {
        (void) memo;
        if (_self != to) { return; }

        auto commun_code = quantity.symbol.code();            
        auto& community = commun_list::get_community(config::list_name, commun_code);
        
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto by_comm_idx = mosaics_table.get_index<"bycommrating"_n>();
        
        auto by_comm_first_itr = by_comm_idx.lower_bound(std::make_tuple(true, MAXINT64, MAXINT64));
        auto by_comm_max = config::default_comm_grades.size();
        auto mosaic_num = 0;
        auto points_sum = 0;
        for (auto by_comm_itr = by_comm_first_itr; (mosaic_num < by_comm_max) &&
                                                   (by_comm_itr != by_comm_idx.end()) && 
                                                   (by_comm_itr->comm_rating > 0); by_comm_itr++, mosaic_num++) {
            points_sum += by_comm_itr->comm_rating;
        }
        std::map<uint64_t, int64_t> ranked_mosaics;
        mosaic_num = 0;
        for (auto by_comm_itr = by_comm_first_itr; (mosaic_num < by_comm_max) &&
                                                   (by_comm_itr != by_comm_idx.end()) && 
                                                   (by_comm_itr->comm_rating > 0); by_comm_itr++, mosaic_num++) {

            ranked_mosaics[by_comm_itr->tracery] = config::default_comm_grades[mosaic_num] + 
                                    safe_prop(config::default_comm_points_grade_sum, by_comm_itr->comm_rating, points_sum);
        }
        auto by_lead_idx = mosaics_table.get_index<"byleadrating"_n>();
        
        auto by_lead_max = config::default_lead_grades.size();
        mosaic_num = 0;
        for (auto by_lead_itr = by_lead_idx.lower_bound(std::make_tuple(false, MAXINT64, MAXINT64)); 
                                                   (mosaic_num < by_lead_max) &&
                                                   (by_lead_itr != by_lead_idx.end()) && 
                                                   (by_lead_itr->lead_rating >= community.min_lead_rating); by_lead_itr++, mosaic_num++) {

            ranked_mosaics[by_lead_itr->tracery] += config::default_lead_grades[mosaic_num];
        }
        std::vector<std::pair<uint64_t, int64_t> > top_mosaics;
        top_mosaics.reserve(ranked_mosaics.size());
        for (const auto& m : ranked_mosaics) {
            top_mosaics.emplace_back(m);
        }
        
        auto middle = top_mosaics.begin() + std::min(size_t(community.rewarded_mosaic_num), top_mosaics.size());
        std::partial_sort(top_mosaics.begin(), middle, top_mosaics.end(),
            [](const std::pair<uint64_t, int64_t>& lhs, const std::pair<uint64_t, int64_t>& rhs) { return lhs.second > rhs.second; });
        
        uint64_t grades_sum = 0;
        for (auto itr = top_mosaics.begin(); itr != middle; itr++) {
            grades_sum += itr->second;
        }
        
        gallery_types::stats stats_table(_self, commun_code.raw());
        const auto& stat = get_stat(_self, stats_table, commun_code);
        auto total_reward = quantity.amount + stat->retained;
        auto left_reward  = total_reward;
        
        uint16_t place = 0;
        for (auto itr = top_mosaics.begin(); itr != middle; itr++) {
            auto cur_reward = safe_prop(total_reward, itr->second, grades_sum);
            
            auto mosaic = mosaics_table.find(itr->first);
            mosaics_table.modify(mosaic, name(), [&](auto& item) {
                if (item.meritorious) {
                    item.reward += cur_reward;
                    left_reward -= cur_reward;
                    send_mosaic_event(_self, quantity.symbol, item);
                }
                else {
                    item.meritorious = true;
                }
            });
            send_top_event(_self, commun_code, *mosaic, place++);
        }
        stats_table.modify(stat, name(), [&]( auto& s) { s.retained = left_reward; });
    }

    void create_mosaic(name _self, name creator, uint64_t tracery, name opus,
                       asset quantity, uint16_t royalty, gallery_types::providers_t providers) {
        require_auth(creator);
        auto commun_symbol = quantity.symbol;
        auto commun_code   = commun_symbol.code();

        auto& community = commun_list::get_community(config::list_name, commun_symbol);
        check(royalty <= community.author_percent, "incorrect royalty");
        check(providers.size() <= config::max_providers_num, "too many providers");
        
        const auto& op = community.get_opus(opus);
        
        auto points_sum = get_points_sum(quantity.amount, providers);
        eosio::check(op.mosaic_pledge <= points_sum, "points are not enough for a pledge");
        eosio::check(op.min_mosaic_inclusion <= points_sum, "points are not enough for mosaic inclusion");
        eosio::check((providers.size() + 1) * op.min_gem_inclusion <= points_sum, "points are not enough for gem inclusion");
        
        emit::maybe_issue_reward(commun_code, _self);
        
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        eosio::check(mosaics_table.find(tracery) == mosaics_table.end(), "mosaic already exists");
        
        auto now = eosio::current_time_point();
        auto claim_date = now + eosio::seconds(community.collection_period);
        
        mosaics_table.emplace(creator, [&]( auto &item ) { item = gallery_types::mosaic {
            .tracery = tracery,
            .creator = creator,
            .opus = opus,
            .royalty = royalty,
            .close_date = claim_date,
            .gem_count = 0,
            .points = 0,
            .shares = 0
        };});

        freeze_in_gems(_self, true, tracery, claim_date, creator, quantity, providers, false, points_sum, points_sum, op.mosaic_pledge);
    }

    void add_to_mosaic(name _self, uint64_t tracery, asset quantity, bool damn, name gem_creator, gallery_types::providers_t providers) {
        require_auth(gem_creator);
        auto commun_symbol = quantity.symbol;
        auto commun_code = commun_symbol.code();
        
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto mosaic = mosaics_table.find(tracery);
        eosio::check(mosaic != mosaics_table.end(), "mosaic doesn't exist");

        auto community = commun_list::get_community(config::list_name, commun_symbol);
        check(eosio::current_time_point() <= mosaic->close_date, "collection period is over");
        check(!mosaic->banned, "mosaic banned");
        check(mosaic->active, "mosaic is archival, probably collection_period or mosaic_active_period is incorrect");
        
        emit::maybe_issue_reward(commun_code, _self);
        
        auto points_sum = get_points_sum(quantity.amount, providers);
        eosio::check((providers.size() + 1) * community.get_opus(mosaic->opus).min_gem_inclusion <= points_sum, 
            "points are not enough for gem inclusion");
        
        auto shares_abs = damn ?
            calc_bancor_amount(mosaic->damn_points, mosaic->damn_shares, config::shares_cw, points_sum, false) :
            calc_bancor_amount(mosaic->points,      mosaic->shares,      config::shares_cw, points_sum, false);
        
        if (!damn) {
            shares_abs -= pay_royalties(_self, commun_symbol, tracery, mosaic->creator, safe_pct(mosaic->royalty, shares_abs));
        }

        freeze_in_gems(_self, false, tracery, mosaic->close_date, gem_creator, quantity, providers, damn, points_sum, shares_abs, 0);
    }
    
    void hold_gem(name _self, uint64_t tracery, symbol_code commun_code, name gem_owner, name gem_creator) {
        set_gem_holder(_self, tracery, commun_code, gem_owner, gem_creator, gem_owner);
    }
    
    void transfer_gem(name _self, uint64_t tracery, symbol_code commun_code, name gem_owner, name gem_creator, name recipient) {
        eosio::check(gem_owner != recipient, "cannot transfer to self");
        set_gem_holder(_self, tracery, commun_code, gem_owner, gem_creator, recipient);
    }
    
    void claim_gem(name _self, uint64_t tracery, symbol_code commun_code, name gem_owner, name gem_creator, bool eager) {
        auto now = eosio::current_time_point();
        auto claim_info = get_claim_info(_self, tracery, commun_code, eager);
        
        gallery_types::gems gems_table(_self, commun_code.raw());
        auto gems_idx = gems_table.get_index<"bykey"_n>();
        auto gem = gems_idx.find(std::make_tuple(claim_info.tracery, gem_owner, gem_creator));
        eosio::check(gem != gems_idx.end(), "nothing to claim");
        chop_gem(_self, claim_info.commun_symbol, gems_idx, gem, true, claim_info.has_reward, claim_info.premature);
        gems_idx.erase(gem);
    }
    
    void claim_gems_by_creator(name _self, uint64_t tracery, symbol_code commun_code, name gem_creator, bool eager) {
        auto now = eosio::current_time_point();
        auto claim_info = get_claim_info(_self, tracery, commun_code, eager);
        
        gallery_types::gems gems_table(_self, commun_code.raw());
        auto gems_idx = gems_table.get_index<"bycreator"_n>();
        auto gem = gems_idx.lower_bound(std::make_tuple(claim_info.tracery, gem_creator, name()));
        eosio::check((gem != gems_idx.end()) && (gem->tracery == claim_info.tracery) && (gem->creator == gem_creator), "nothing to claim");
        
        while ((gem != gems_idx.end()) && (gem->tracery == claim_info.tracery) && (gem->creator == gem_creator)) {
            chop_gem(_self, claim_info.commun_symbol, gems_idx, gem, true, claim_info.has_reward, claim_info.premature);
            gem = gems_idx.erase(gem);
        }
        
    }
    
    void provide_points(name _self, name grantor, name recipient, asset quantity, std::optional<uint16_t> fee) {
        require_auth(grantor);
        commun_list::check_community_exists(config::list_name, quantity.symbol);
        
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
    
    void advise_mosaics(name _self, symbol_code commun_code, name leader, std::set<uint64_t> favorites) {
        require_auth(leader);
        commun_list::check_community_exists(config::list_name, commun_code);
        eosio::check(control::in_the_top(config::control_name, commun_code, leader), (leader.to_string() + " is not a leader").c_str());
        eosio::check(favorites.size() <= config::advice_weight.size(), "a surfeit of advice");
        
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());

        gallery_types::advices advices_table(_self, commun_code.raw());
        auto prev_advice = advices_table.find(leader.value);
        if (prev_advice != advices_table.end()) {
            eosio::check(prev_advice->favorites != favorites, "no changes in favorites");
            auto prev_weight = config::advice_weight.at(prev_advice->favorites.size() - 1);
            for (auto& m : prev_advice->favorites) {
                auto mosaic = mosaics_table.find(m);
                if (mosaic != mosaics_table.end()) {
                    mosaics_table.modify(mosaic, name(), [&](auto& item) { item.lead_rating -= prev_weight; });
                }
            }
            if (favorites.empty()) {
                advices_table.erase(prev_advice);
                return;
            }
            advices_table.modify(prev_advice, leader, [&](auto& item) { item.favorites = favorites; });
        }
        else {
            eosio::check(!favorites.empty(), "no changes in favorites");
            advices_table.emplace(leader, [&] (auto &item) { item = gallery_types::advice {
                .leader = leader,
                .favorites = favorites
            };});
        }
        
        auto weight = config::advice_weight.at(favorites.size() - 1);
        for (auto& m : favorites) {
            auto mosaic = mosaics_table.find(m);
            eosio::check(mosaic != mosaics_table.end(), "mosaic doesn't exist");
            mosaics_table.modify(mosaic, name(), [&](auto& item) { item.lead_rating += weight; });
        }
    }

    void update_mosaic(name _self, symbol_code commun_code, name creator, uint64_t tracery) {
        require_auth(creator);
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto& mosaic = mosaics_table.get(tracery, "mosaic doesn't exist");
        eosio::check(mosaic.active, "mosaic is inactive");
        mosaics_table.modify(mosaic, eosio::same_payer, [&](auto& m) {
            m.lock_date = time_point();
        });
    }

    void lock_mosaic(name _self, symbol_code commun_code, name leader, uint64_t tracery) {
        require_auth(leader);
        check(control::in_the_top(config::control_name, commun_code, leader), (leader.to_string() + " is not a leader").c_str());
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto& mosaic = mosaics_table.get(tracery, "mosaic doesn't exist");
        check(mosaic.active, "mosaic is inactive");
        check(mosaic.lock_date == time_point(), "Mosaic should be modified to lock again.");
        mosaics_table.modify(mosaic, eosio::same_payer, [&](auto& m) {
            m.lock();
            m.lock_date = eosio::current_time_point();
        });
    }

    void unlock_mosaic(name _self, symbol_code commun_code, name leader, uint64_t tracery) {
        require_auth(leader);
        check(control::in_the_top(config::control_name, commun_code, leader), (leader.to_string() + " is not a leader").c_str());
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto& mosaic = mosaics_table.get(tracery, "mosaic doesn't exist");
        check(mosaic.locked, "mosaic not locked");
        check(!mosaic.banned, "mosaic banned");
        auto now = eosio::current_time_point();
        auto& community = commun_list::get_community(config::list_name, commun_code);
        eosio::check(now <= mosaic.close_date + eosio::seconds(community.moderation_period), "cannot unlock mosaic after moderation period");
        mosaics_table.modify(mosaic, eosio::same_payer, [&](auto& m) {
            m.unlock();
            m.close_date += (now - mosaic.lock_date);
        });
    }

    void ban_mosaic(name _self, symbol_code commun_code, uint64_t tracery) {
        require_auth(point::get_issuer(config::point_name, commun_code));
        
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto mosaic = mosaics_table.find(tracery);
        eosio::check(mosaic != mosaics_table.end(), "mosaic doesn't exist");
        eosio::check(mosaic->active, "mosaic is inactive");
        mosaics_table.modify(mosaic, name(), [&](auto& item) { item.ban(); });
        T::deactivate(_self, commun_code, *mosaic);
    }
};

class [[eosio::contract("comn.gallery")]] gallery : public gallery_base<gallery>, public contract {
public:
    using contract::contract;
    static void deactivate(name self, symbol_code commun_code, const gallery_types::mosaic& mosaic) {};

    [[eosio::action]] void createmosaic(name creator, uint64_t tracery, name opus, asset quantity, uint16_t royalty, gallery_types::providers_t providers) {
        create_mosaic(_self, creator, tracery, opus, quantity, royalty, providers);
    }
        
    [[eosio::action]] void addtomosaic(uint64_t tracery, asset quantity, bool damn, name gem_creator, gallery_types::providers_t providers) {
        add_to_mosaic(_self, tracery, quantity, damn, gem_creator, providers);
    }
    
    [[eosio::action]] void hold(uint64_t tracery, symbol_code commun_code, name gem_owner, std::optional<name> gem_creator) {
        hold_gem(_self, tracery, commun_code, gem_owner, gem_creator.value_or(gem_owner));
    }
    
    [[eosio::action]] void transfer(uint64_t tracery, symbol_code commun_code, name gem_owner, std::optional<name> gem_creator, name recipient) {
        transfer_gem(_self, tracery, commun_code, gem_owner, gem_creator.value_or(gem_owner), recipient);
    }
    
    [[eosio::action]] void claim(uint64_t tracery, symbol_code commun_code, name gem_owner, 
                                   std::optional<name> gem_creator, std::optional<bool> eager) {
        claim_gem(_self, tracery, commun_code, gem_owner, gem_creator.value_or(gem_owner), eager.value_or(false));
    }
    
    [[eosio::action]] void provide(name grantor, name recipient, asset quantity, std::optional<uint16_t> fee) {
        provide_points(_self, grantor, recipient, quantity, fee);
    }
    
    [[eosio::action]] void advise(symbol_code commun_code, name leader, std::set<uint64_t> favorites) {
        advise_mosaics(_self, commun_code, leader, favorites);
    }
    
    //TODO: [[eosio::action]] void checkadvice (symbol_code commun_code, name leader);

    [[eosio::action]] void update(symbol_code commun_code, name creator, uint64_t tracery) {
        update_mosaic(_self, commun_code, creator, tracery);
    }

    [[eosio::action]] void lock(symbol_code commun_code, name leader, uint64_t tracery, string reason) {
        eosio::check(!reason.empty(), "Reason cannot be empty.");
        lock_mosaic(_self, commun_code, leader, tracery);
    }

    [[eosio::action]] void unlock(symbol_code commun_code, name leader, uint64_t tracery, string reason) {
        eosio::check(!reason.empty(), "Reason cannot be empty.");
        unlock_mosaic(_self, commun_code, leader, tracery);
    }

    [[eosio::action]] void ban(symbol_code commun_code, uint64_t tracery) {
        ban_mosaic(_self, commun_code, tracery);
    }
    
    void ontransfer(name from, name to, asset quantity, std::string memo) {
        on_points_transfer(_self, from, to, quantity, memo);
    }      
};
    
} /// namespace commun

#define GALLERY_ACTIONS (createmosaic)(addtomosaic)(hold)(transfer)(claim)(provide)(advise)(update)(lock)(unlock)(ban)
