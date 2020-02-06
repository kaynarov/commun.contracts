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
#include <commun/dispatchers.hpp>
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

#define GALLERY_LIBRARY contract("commun.gallery"), contract("commun.publication")

namespace gallery_types {
    using providers_t = std::vector<std::pair<name, int64_t> >;

    /**
     * \brief The structure represents mosaic data table in DB.
     * \ingroup gallery_tables
     *
     * \details The table contains data that uniquely identifies and represents a mosaic.
     *
     * <b>The states which a mosaic may exist in:</b>
     * - ACTIVE — Active state of the mosaic, collecting user opinions (sympathy). Once a mosaic is created, it is assigned ACTIVE status;
     * - MODERATE — Collection of user opinions has been completed. The leaders decide whether to pay reward to author of the mosaic and users who voted;
     * - ARCHIVED — Mosaic is in archiving state; user opinions collection has been completed;
     * - LOCKED — Collection of user opinions is temporarily blocked by leaders (i.e. due to complaints from users about the post). Leaders can put it back in the ACTIVE state to continue collecting users' opinions. The time spent by the mosaic in LOCKED state is determined using the lock_date parameter. The period of collecting opinions (collection_period parameter) is increased by this value. The author can improve content of unlocked mosaic. In this case, the lock_date parameter will be reset to zero. Leaders can lock the mosaic again if the author’s work still does not suit users;
     * - BANNED — Post has been locked by leaders. Collected reward to the author of the post and voted users will not be paid;
     * - HIDDEN — Post has been removed by the author. Mosaic of the post may still exist, since it takes some time to destroy the gems. If the post has collected the sympathy of users, then rewards to these users will be paid;
     * - BANNED_AND_HIDDEN — Post has been removed and blocked by leaders (no rewards will be paid).
     */
    struct mosaic_struct {

        mosaic_struct() = default;
        uint64_t tracery; //!< The mosaic tracery, used as primary key
        name creator;     //!< The mosaic creator
        
        name opus;        //!< Mosaic description type. The parameter indicates what the mosaic describes, such as a post or comment.
        uint16_t royalty; //!< Share (in percent) of royalties to the author for creating the mosaic. When creating a mosaic, its first gem belongs to the author. Part of weight of other gems created by other members is added to the gem of mosaic author. So, part of funds invested in gem is allocated to the mosaic author. 

        time_point lock_date = time_point();   //!< Mosaic lock date. The mosaic is blocked by the leaders if any frauds with the mosaic are detected. After blocking, the collection of gems inside this mosaic is suspended.
        time_point collection_end_date;        //!< Gem collection period
        uint16_t gem_count;   //!< Current number of gems inside the mosaic
        
        int64_t points;   //!< Number of points collected inside this mosaic. Points are added to the mosaic when users vote.
        int64_t shares;   //!< Mosaic weight (post weight) calculated via «bancor» function. Weight of vote depends on the voting time. The earlier vote carries more weight.
        int64_t damn_points = 0;   //!< Number of points related to negative votes
        int64_t damn_shares = 0;   //!< Number of shares related to negative votes
        int64_t pledge_points = 0; //!< Number of tokens pledged. A number of points is invested in creating a mosaic and thereby limits the number of mosaics created by author. These points are «frozen» and cannot be part of the reward.
        
        int64_t reward = 0;   //!< Total reward amount. The reward is formed not at the end of the mosaic collection, but with a certain periodicity. Rewards are allocated to top mosaics which have become the most popular among users. Number of these mosaics is determined by the \a rewarded_mosaic_num parameter in the \a c.list. The \a c.ctrl contract allocates tokens as a share of the annual emission to \a c.gallery contract. The funds received are converted into points and then distributed among worthy mosaics.
        
        int64_t comm_rating = 0;
        int64_t lead_rating = 0;
        
        enum status_t: uint8_t { ACTIVE, MODERATE, ARCHIVED, LOCKED, BANNED, HIDDEN, BANNED_AND_HIDDEN };
        uint8_t status = ACTIVE;   //!< Field indicating a mosaic status. Once a mosaic is created, it is assigned ACTIVE status.
        time_point last_top_date = time_point();
        bool deactivated_xor_locked = false;   //!< Flag indicating the post is inactive or blocked. \a true — post blocked by leaders or archived.
        
        bool banned()const { return status == BANNED || status == BANNED_AND_HIDDEN; }
        bool hidden()const { return status == HIDDEN || status == BANNED_AND_HIDDEN; }
        bool deactivated()const { return deactivated_xor_locked && status != LOCKED; }

        void lock() {
            check(status == ACTIVE, "mosaic is inactive");
            check(lock_date == time_point(), "Mosaic should be modified to lock again.");
            check(!deactivated_xor_locked, "SYSTEM: lock, incorrect deactivated_xor_locked value");
            status = LOCKED;
            lock_date = eosio::current_time_point();
            deactivated_xor_locked = true;
        }

        void unlock(int64_t moderation_period) {
            check(status == LOCKED, "mosaic not locked");
            check(deactivated_xor_locked, "SYSTEM: unlock, incorrect deactivated_xor_locked value");
            auto now = eosio::current_time_point();
            eosio::check(now <= collection_end_date + eosio::seconds(moderation_period), "cannot unlock mosaic after moderation period");
            status = ACTIVE;
            collection_end_date += now - lock_date;
            deactivated_xor_locked = false;
        }

        uint64_t primary_key() const { return tracery; }
        using by_comm_rating_t = std::tuple<uint8_t, int64_t, int64_t>;
        by_comm_rating_t by_comm_rating()const { return std::make_tuple(status, comm_rating, lead_rating); }
        using by_lead_rating_t = std::tuple<int64_t, int64_t>;
        by_lead_rating_t by_lead_rating()const { return std::make_tuple(lead_rating, comm_rating); }
        using by_date_t = std::tuple<bool, time_point>;
        by_date_t by_date()const { return std::make_tuple(deactivated_xor_locked, collection_end_date); }
        using by_status_t = std::tuple<uint8_t, time_point>;
        by_status_t by_status() const { return std::make_tuple(status, collection_end_date); }
    };
    
    /**
     * \brief The structure represents gem data table in DB.
     * \ingroup gallery_tables
     *
     * The table contains data that uniquely identifies and represents a gem in mosaic.
     */
    struct gem_struct {
        uint64_t id; //!< Unique gem identifier
        uint64_t tracery; //!< Mosaic tracery containing the gem
        time_point claim_date; //!< Date when the gem can be broken down. The gem cannot be destroyed until this date. Also, points spent on voting for the mosaic cannot be returned back until this date(such implementation excludes voting for another mosaic with the same points).
        
        int64_t points; //!< Number of points that were frozen during voting
        int64_t shares; //!< Number of shares calculated by the «shares» function
        
        int64_t pledge_points; //!< Number of points pledged to create the gem (this is implemented to limit a number of gems created in community. The parameter is set in \a c.list contract).
        
        name owner; //!< Gem owner
        name creator;
        
        uint64_t primary_key() const { return id; }
        using key_t = std::tuple<uint64_t, name, name>;
        key_t by_key()const { return std::make_tuple(tracery, owner, creator); }
        key_t by_creator()const { return std::make_tuple(tracery, creator, owner); }
        using by_claim_t = std::tuple<name, time_point>;
        by_claim_t by_claim()const { return std::make_tuple(owner, claim_date); }
        time_point by_claim_joint()const { return claim_date; }
    };
    
    /**
     * \brief The structure represents the table in DB containing total number of all «frozen» points for a user.
     * \ingroup gallery_tables
     *
     * The user account name can be found in the scope field of the table. Each created table has two additional fields — code of the table and scope indicating the owner of points.
     */
    struct inclusion_struct {
        asset quantity; //!< Total number of «frozen» user points
            // just an idea:
            // use as inclusion not only points, but also other gems. 
            // this will allow to buy shares in the mosaics without liquid points, creating more sophisticated collectables
            // (pntinclusion / geminclusion)
        
        uint64_t primary_key()const { return quantity.symbol.code().raw(); }
    };
    
    /**
     * \brief The structure represents the mosaic statictic data table in DB.
     * \ingroup gallery_tables
     *
     * The table contains statistic information about total number of unclaimed (blocked) points for all users related to a mosaic.
     */
    struct stat_struct {
        uint64_t id; //!< Mosaic identifier
        int64_t unclaimed = 0; //!< Total number of unclaimed points for all users related to the mosaic
        int64_t retained = 0; //!< Total amount of retained reward related to unclaimed points
        time_point last_reward_date = time_point();

        uint64_t primary_key()const { return id; }
    };
    
    struct provision_struct {
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
    
    struct advice_struct {
        name leader;
        std::set<uint64_t> favorites;
        uint64_t primary_key()const { return leader.value; }
    };
    
    using mosaic_comm_index [[using eosio: non_unique, order("status","asc"), order("comm_rating","desc"), order("lead_rating","desc")]] =
        eosio::indexed_by<"bycommrating"_n, eosio::const_mem_fun<gallery_types::mosaic_struct, gallery_types::mosaic_struct::by_comm_rating_t, &gallery_types::mosaic_struct::by_comm_rating> >;
    using mosaic_lead_index [[using eosio: non_unique, order("lead_rating","desc"), order("comm_rating","desc")]] =
        eosio::indexed_by<"byleadrating"_n, eosio::const_mem_fun<gallery_types::mosaic_struct, gallery_types::mosaic_struct::by_lead_rating_t, &gallery_types::mosaic_struct::by_lead_rating> >;
    using mosaic_coll_end_index [[using eosio: non_unique, order("deactivated_xor_locked","desc"), order("collection_end_date","asc")]] =
        eosio::indexed_by<"bydate"_n, eosio::const_mem_fun<gallery_types::mosaic_struct, gallery_types::mosaic_struct::by_date_t, &gallery_types::mosaic_struct::by_date> >;
    using mosaic_status_index [[using eosio: non_unique, order("status","desc"), order("collection_end_date","asc")]] =
        eosio::indexed_by<"bystatus"_n, eosio::const_mem_fun<gallery_types::mosaic_struct, gallery_types::mosaic_struct::by_status_t, &gallery_types::mosaic_struct::by_status> >;

    using mosaics [[using eosio: order("tracery","asc"), scope_type("symbol_code"), GALLERY_LIBRARY]] = eosio::multi_index<"mosaic"_n, gallery_types::mosaic_struct, mosaic_comm_index, mosaic_lead_index, mosaic_coll_end_index, mosaic_status_index>;
    
    using gem_key_index [[using eosio: order("tracery","asc"), order("owner","asc"), order("creator","asc")]] =
        eosio::indexed_by<"bykey"_n, eosio::const_mem_fun<gallery_types::gem_struct, gallery_types::gem_struct::key_t, &gallery_types::gem_struct::by_key> >;
    using gem_creator_index [[using eosio: order("tracery","asc"), order("creator","asc"), order("owner","asc")]] =
        eosio::indexed_by<"bycreator"_n, eosio::const_mem_fun<gallery_types::gem_struct, gallery_types::gem_struct::key_t, &gallery_types::gem_struct::by_creator> >;
    using gem_claim_index [[using eosio: non_unique, order("owner","asc"), order("claim_date","asc")]] =
        eosio::indexed_by<"byclaim"_n, eosio::const_mem_fun<gallery_types::gem_struct, gallery_types::gem_struct::by_claim_t, &gallery_types::gem_struct::by_claim> >;
    using gem_claim_joint_index [[using eosio: non_unique, order("claim_date","asc")]] =
        eosio::indexed_by<"byclaimjoint"_n, eosio::const_mem_fun<gallery_types::gem_struct, time_point, &gallery_types::gem_struct::by_claim_joint> >;

    using gems [[using eosio: order("id","asc"), scope_type("symbol_code"), GALLERY_LIBRARY]] = eosio::multi_index<"gem"_n, gallery_types::gem_struct, gem_key_index, gem_creator_index, gem_claim_index, gem_claim_joint_index>;
    
    using inclusions [[using eosio: order("quantity._sym","asc"), GALLERY_LIBRARY]] = eosio::multi_index<"inclusion"_n, gallery_types::inclusion_struct>;
    
    using prov_key_index [[using eosio: order("grantor","asc"), order("recipient","asc")]] = eosio::indexed_by<"bykey"_n, eosio::const_mem_fun<gallery_types::provision_struct, gallery_types::provision_struct::key_t, &gallery_types::provision_struct::by_key> >;
    using provs [[using eosio: order("id","asc"), scope_type("symbol_code"), GALLERY_LIBRARY]] = eosio::multi_index<"provision"_n, gallery_types::provision_struct, prov_key_index>;

    using advices [[using eosio: scope_type("symbol_code"), order("leader","asc"), GALLERY_LIBRARY]] = eosio::multi_index<"advice"_n, gallery_types::advice_struct>;

    using stats [[using eosio: scope_type("symbol_code"), order("id","asc"), GALLERY_LIBRARY]] = eosio::multi_index<"stat"_n, gallery_types::stat_struct>;
    
namespace events {
    
    /**
     * \brief The structure represents a mosaic destruction event. The mosaic is destroyed after destruction of the last gem belonging to this mosaic.
     * \ingroup gallery_events
     */
    struct [[using eosio: event("mosaicchop"), GALLERY_LIBRARY]] mosaic_chop_event {
        symbol_code commun_code; //!< Point symbol
        uint64_t tracery; //!< Tracery that breaks down
    };
    
    /**
     * \brief The structure represents a mosaic state change event. Such event is sent when a mosaic state changes.
     * \ingroup gallery_events
     */
    struct [[using eosio: event("mosaicstate"), GALLERY_LIBRARY]] mosaic_state_event {
        uint64_t tracery; //!< Mosaic tracery
        name creator; //!< Mosaic creator
        time_point collection_end_date; //!< End date of collecting user opinions
        uint16_t gem_count; //!< Number of gems inside the mosaic
        int64_t shares; //!< Mosaic weight
        int64_t damn_shares; //!< Weight of gems with negative votes
        asset reward; //!< Amount of currently collected rewards
        bool banned; //!< Flag indicating the blocking of reward at the initiative of community leaders
    };
    
    /**
     * \brief The structure represents a gem state change event. Gem for a mosaic is automatically created when an author creates the mosaic. A state of the gem changes when a user votes.
     * \ingroup gallery_events
     */
    struct [[using eosio: event("gemstate"), GALLERY_LIBRARY]] gem_state_event {
        uint64_t tracery; //!< Mosaic tracery
        name owner; //!< Mosaic owner
        name creator;
        asset points; //!< Number of «frozen» points in the gem
        asset pledge_points; //!< Number of pledged points in the gem
        bool damn; //!< Flag indicating a negative or positive user opinion. \a true is negative one.
        int64_t shares; //!< Weight of gem calculated by the bancor function
    };
    
    /**
     * \brief The structure represents a gem destruction event. Mosaic breaks down after rewarding it, when users can take back their points.
     * \ingroup gallery_events
     */
    struct [[using eosio: event("gemchop"), GALLERY_LIBRARY]] gem_chop_event {
        uint64_t tracery; //!< Mosaic tracery
        name owner; //!< Mosaic owner
        name creator;
        asset reward; //!< Gem owner reward.
        asset unfrozen; //!< Number of returned points that were «frozen» at the time of collecting user opinions. This is total number of points given as sympathies and those that were pledged.
    };
    
    /**
     * \brief The structure represents the event about the selected best mosaics to be rewarded. The number of selected mosaics is determined by the \a rewarded_mosaic_num parameter in the \a c.list contract and defaults to 10.
     * \ingroup gallery_events
     */
    struct [[using eosio: event("mosaictop"), GALLERY_LIBRARY]] mosaic_top_event {
        symbol_code commun_code; //!< Point symbol
        uint64_t tracery; //!< Mosaic tracery
        uint16_t place; //!< Place where the mosaic is located
        int64_t comm_rating;
        int64_t lead_rating;
    };
    
    /**
     * \brief The structure represents an event about the current number of «frozen» user points.
     * \ingroup gallery_events
     */
    struct [[using eosio: event("inclstate"), GALLERY_LIBRARY]] inclusion_state_event {
        name account; //!< User account that changed the current number of «frozen» points. The event occurred due to this account action.
        asset quantity; //!< Current number of «frozen» points belonging to the account
    };
}// events
}// gallery_types


template<typename T>
class gallery_base {
    void send_mosaic_event(name _self, symbol commun_symbol, const gallery_types::mosaic_struct& mosaic) {
        gallery_types::events::mosaic_state_event data {
            .tracery = mosaic.tracery,
            .creator = mosaic.creator,
            .collection_end_date = mosaic.collection_end_date,
            .gem_count = mosaic.gem_count,
            .shares = mosaic.shares,
            .damn_shares = mosaic.damn_shares,
            .reward = asset(mosaic.reward, commun_symbol),
            .banned = mosaic.banned()
        };
        eosio::event(_self, "mosaicstate"_n, data).send();
    }
    
    void send_mosaic_chop_event(name _self, symbol_code commun_code, uint64_t tracery) {
        gallery_types::events::mosaic_chop_event data {
            .commun_code = commun_code,
            .tracery = tracery
        };
        eosio::event(_self, "mosaicchop"_n, data).send();
    }
    
    void send_gem_event(name _self, symbol commun_symbol, const gallery_types::gem_struct& gem) {
        gallery_types::events::gem_state_event data {
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
    
    void send_chop_event(name _self, const gallery_types::gem_struct& gem, asset reward, asset unfrozen) {
        gallery_types::events::gem_chop_event data {
            .tracery = gem.tracery,
            .owner = gem.owner,
            .creator = gem.creator,
            .reward = reward,
            .unfrozen = unfrozen
        };
        eosio::event(_self, "gemchop"_n, data).send();
    }
    
    void send_top_event(name _self, symbol_code commun_code, const gallery_types::mosaic_struct& mosaic, uint16_t place) {
        gallery_types::events::mosaic_top_event data {
            .commun_code = commun_code,
            .tracery = mosaic.tracery,
            .place = place,
            .comm_rating = mosaic.comm_rating,
            .lead_rating = mosaic.lead_rating
        };
        eosio::event(_self, "mosaictop"_n, data).send();
    }
    
    void send_inclusion_event(name _self, name account, asset quantity) {
        gallery_types::events::inclusion_state_event data {
            .account = account,
            .quantity = quantity
        };
        eosio::event(_self, "inclstate"_n, data).send();
    }
    
private:
    bool send_reward(name from, name to, const asset &quantity, uint64_t tracery) {
        if (to && !point::balance_exists(to, quantity.symbol.code())) {
            return false;
        }
        
        if (quantity.amount) {
            action(
                permission_level{from, config::transfer_permission},
                config::point_name,
                "transfer"_n,
                std::make_tuple(from, to, quantity, "reward for " + std::to_string(tracery))
            ).send();
        }
        return true; 
    };
    
    void freeze(name _self, name account, const asset &quantity, name ram_payer = name()) {
        
        if (!quantity.amount) {
            return;
        }
        auto commun_code = quantity.symbol.code();
        auto balance_exists = point::balance_exists(account, commun_code);
        eosio::check(balance_exists, quantity.amount > 0 ? "balance doesn't exist" : "SYSTEM: points are frozen while balance doesn't exist");

        auto balance_amount = point::get_balance(account, commun_code).amount;
        
        gallery_types::inclusions inclusions_table(_self, account.value);
        auto incl = inclusions_table.find(quantity.symbol.code().raw());
        eosio::check((incl == inclusions_table.end()) || ((0 < incl->quantity.amount) && (incl->quantity.amount <= balance_amount)), 
            "SYSTEM: invalid value of frozen points");
        
        auto new_val = (incl != inclusions_table.end()) ? quantity.amount + incl->quantity.amount : quantity.amount;
        eosio::check(new_val >= 0, "SYSTEM: impossible combination of quantity and frozen points");
        eosio::check(new_val <= balance_amount, "overdrawn balance");
        
        send_inclusion_event(_self, account, asset(new_val, quantity.symbol));
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
        return point::get_reserve_quantity(quantity, nullptr).amount;
    }
    
    template<typename GemIndex, typename GemItr>
    bool chop_gem(name _self, symbol commun_symbol, GemIndex& gem_idx, GemItr& gem_itr, bool by_user, bool has_reward, bool no_rewards = false) {
        const auto& gem = *gem_itr;
        auto commun_code = commun_symbol.code();
        auto& community = commun_list::get_community(commun_code);

        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto mosaic = mosaics_table.find(gem.tracery);
        eosio::check(mosaic != mosaics_table.end(), "mosaic doesn't exist");
        auto claim_date = mosaic->collection_end_date + eosio::seconds(community.moderation_period + community.extra_reward_period);
        bool ready_to_claim = claim_date <= eosio::current_time_point() && gem.claim_date != config::eternity;
        if (by_user) {
            eosio::check(ready_to_claim || has_auth(gem.owner) 
                || (has_auth(gem.creator) && !has_reward && gem.claim_date != config::eternity), "lack of necessary authority");
        } else if (!ready_to_claim) {
            gem_idx.modify(gem_itr, eosio::same_payer, [&](auto& item) {
                item.claim_date = claim_date;
            });
            return false;
        }

        int64_t reward = 0;
        bool damn = gem.shares < 0;
        if (!no_rewards && damn == mosaic->banned()) {
            reward = damn ? 
                (community.damned_gem_reward_enabled ? safe_prop(mosaic->reward, -gem.shares, mosaic->damn_shares) : 0) :
                safe_prop(mosaic->reward,  gem.shares, mosaic->shares);
        }
        asset frozen_points(gem.points + gem.pledge_points, commun_symbol);
        asset reward_points(reward, commun_symbol);
        
        freeze(_self, gem.owner, -frozen_points);
        send_chop_event(_self, gem, reward_points, frozen_points);

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
        if (!send_reward(_self, gem.owner, reward_points, gem.tracery)) {
            //shouldn't we use a more specific memo in send_reward here?
            eosio::check(send_reward(_self, point::get_issuer(commun_code), reward_points, gem.tracery), "the issuer's balance doesn't exist");
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
                item.pledge_points -= gem.pledge_points;
                item.reward -= reward;
                item.gem_count--;
            });
            send_mosaic_event(_self, commun_symbol, *mosaic);
        }
        else {
            gallery_types::stats stats_table(_self, commun_code.raw());
            const auto& stat = stats_table.get(commun_code.raw(), "SYSTEM: no stat but community present");
            stats_table.modify(stat, name(), [&]( auto& s) { s.unclaimed += mosaic->reward - reward; });
            if (!mosaic->deactivated()) {
                T::deactivate(_self, commun_code, *mosaic);
            }
            send_mosaic_chop_event(_self, commun_code, mosaic->tracery);
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
        auto& community = commun_list::get_community(commun_code);
        
        gallery_types::gems gems_table(_self, commun_code.raw());
        
        bool refilled = false;
        
        if (!creating) {
            auto gems_idx = gems_table.get_index<"bykey"_n>();
            auto gem = gems_idx.find(std::make_tuple(tracery, owner, creator));
            if (gem != gems_idx.end()) {
                eosio::check((shares < 0) == (gem->shares < 0), "gem type mismatch");
                eosio::check(community.refill_gem_enabled, "can't refill the gem");
                gems_idx.modify(gem, name(), [&](auto& item) {
                    item.points += points;
                    item.shares += shares;
                    item.pledge_points += pledge_points;
                });
                send_gem_event(_self, commun_symbol, *gem);
                refilled = true;
            }
        }
        
        if (!refilled) {
            
            uint8_t gem_num = 0;
            auto max_claim_date = eosio::current_time_point();
            auto claim_idx = gems_table.get_index<"byclaim"_n>();
            auto chop_gem_of = [&](name account) {
                auto gem_itr = claim_idx.lower_bound(std::make_tuple(account, time_point()));
                if ((gem_itr != claim_idx.end()) && (gem_itr->owner == account) && (gem_itr->claim_date < max_claim_date)) {
                    if (chop_gem(_self, commun_symbol, claim_idx, gem_itr, false, true)) {
                        claim_idx.erase(gem_itr);
                    }
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
                if (chop_gem(_self, commun_symbol, joint_idx, gem_itr, false, true, true)) {
                    gem_itr = joint_idx.erase(gem_itr);
                }
                else {
                    ++gem_itr;
                }
                ++gem_num;
            }
            
            gems_table.emplace(creator, [&]( auto &item ) {
                item = gallery_types::gem_struct {
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
            item.pledge_points += pledge_points;
            if (!refilled) {
                eosio::check(item.gem_count < std::numeric_limits<decltype(item.gem_count)>::max(), "gem_count overflow");
                item.gem_count++;
            }
        });
    }

    int64_t pay_royalties(name _self, symbol commun_symbol, uint64_t tracery, name mosaic_creator, int64_t shares) {
        if (!shares) {
            return 0;
        }
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
        
        int64_t ret = 0;
        
        for (auto gem_itr = first_gem_itr; (gem_itr != gems_idx.end()) && 
                                           (gem_itr->tracery == tracery) && 
                                           (gem_itr->creator == mosaic_creator); ++gem_itr) {
            int64_t cur_shares = 0;
            
            if (gem_itr->shares > 0) {
                cur_shares = safe_prop(shares, gem_itr->shares, pre_shares_sum);
            }
            else if (!pre_shares_sum && gem_itr->owner == mosaic_creator) {
                cur_shares = shares;
            }
            
            if (cur_shares > 0) {
                gems_idx.modify(gem_itr, name(), [&](auto& item) {
                    item.shares += cur_shares;
                });
                send_gem_event(_self, commun_symbol, *gem_itr);
                ret += cur_shares;
                
                if (ret == shares) {
                    break;
                }
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

        deactivate_old_mosaics(_self, commun_code);
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

        auto& community = commun_list::get_community(commun_code);
        
        claim_info_t ret{mosaic.tracery, mosaic.reward != 0, now <= mosaic.collection_end_date + eosio::seconds(community.moderation_period + community.extra_reward_period), community.commun_symbol};
        check(!ret.premature || eager, "moderation period isn't over yet");
        
        emit::maybe_issue_reward(commun_code, _self);
        
        return ret;
    }
    
    void set_gem_holder(name _self, uint64_t tracery, symbol_code commun_code, name gem_owner, name gem_creator, name holder) {
        require_auth(gem_owner);
        auto& community = commun_list::get_community(commun_code);
        
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        const auto& mosaic = mosaics_table.get(tracery, "mosaic doesn't exist");
        gallery_types::gems gems_table(_self, commun_code.raw());
        auto gems_idx = gems_table.get_index<"bykey"_n>();
        auto gem = gems_idx.find(std::make_tuple(tracery, gem_owner, gem_creator));
        eosio::check(gem != gems_idx.end(), "gem doesn't exist");
        eosio::check(gem->claim_date != config::eternity || gem_owner != holder, "gem is already held");
        
        //it will prevent the mosaic from being erased, so
        eosio::check(gem->points >= community.get_opus(mosaic.opus).min_mosaic_inclusion, "not enough inclusion");
        
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
    
    void deactivate_old_mosaics(name _self, symbol_code commun_code) {
        auto& community = commun_list::get_community(commun_code);

        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto mosaics_by_date_idx = mosaics_table.get_index<"bydate"_n>();
        auto mosaics_by_status_idx = mosaics_table.get_index<"bystatus"_n>();
        auto now = eosio::current_time_point();
        auto max_collection_end_date = now - (eosio::seconds(community.moderation_period) + eosio::seconds(community.extra_reward_period));

        auto mosaic_by_status = mosaics_by_status_idx.lower_bound(std::make_tuple(gallery_types::mosaic_struct::ACTIVE, time_point()));
        for (size_t i = 0; i < config::auto_deactivate_num && mosaic_by_status != mosaics_by_status_idx.end(); i++, ++mosaic_by_status) {
            if (mosaic_by_status->collection_end_date >= now || mosaic_by_status->status != gallery_types::mosaic_struct::ACTIVE) {
                break;
            }
            mosaics_by_status_idx.modify(mosaic_by_status, name(), [&](auto& item) {
                item.status = gallery_types::mosaic_struct::MODERATE;
            });
        }

        auto mosaic_by_date = mosaics_by_date_idx.lower_bound(std::make_tuple(false, time_point()));
        for (size_t i = 0; i < config::auto_deactivate_num && mosaic_by_date != mosaics_by_date_idx.end(); i++, ++mosaic_by_date) {
            if (mosaic_by_date->collection_end_date >= max_collection_end_date) {
                break;
            }
            check(mosaic_by_date->status != gallery_types::mosaic_struct::LOCKED, "SYSTEM: deactivate_old_mosaics, incorrect status value");
            mosaics_by_date_idx.modify(mosaic_by_date, name(), [&](auto& item) {
                if (item.status < gallery_types::mosaic_struct::ARCHIVED) {
                    item.status = gallery_types::mosaic_struct::ARCHIVED;
                }
                item.deactivated_xor_locked = true;
            });
            T::deactivate(_self, commun_code, *mosaic_by_date);
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

    void on_points_transfer(name _self, name from, name to, asset quantity, std::string memo) {
        (void) memo;
        if (_self != to) { return; }

        auto commun_code = quantity.symbol.code();            
        auto& community = commun_list::get_community(commun_code);
        
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto by_comm_idx = mosaics_table.get_index<"bycommrating"_n>();

        auto by_comm_max = config::default_comm_grades.size();
        size_t mosaic_num = 0;
        int64_t points_sum = 0;

        struct ranked_mosaic final {
            int64_t comm_rating;
            size_t mosaic_num;
            int64_t weight;
        }; // struct ranked_mosaic

        std::map<uint64_t, ranked_mosaic> ranked_mosaics;

        for (auto by_comm_itr = by_comm_idx.lower_bound(std::make_tuple(uint8_t(gallery_types::mosaic_struct::ACTIVE), MAXINT64, MAXINT64));
                                                   (mosaic_num < by_comm_max) &&
                                                   (by_comm_itr != by_comm_idx.end()) &&
                                                   (by_comm_itr->status == gallery_types::mosaic_struct::ACTIVE) &&
                                                   (by_comm_itr->comm_rating > 0); by_comm_itr++, mosaic_num++) {
            ranked_mosaics[by_comm_itr->tracery] = ranked_mosaic {
                .comm_rating = by_comm_itr->comm_rating,
                .mosaic_num = mosaic_num,
                .weight = 0
            };
            points_sum += by_comm_itr->comm_rating;
        }
        for (auto& m: ranked_mosaics) {
            m.second.weight = config::default_comm_grades[m.second.mosaic_num] +
                safe_prop(config::default_comm_points_grade_sum, m.second.comm_rating, points_sum);
        }
        auto by_lead_idx = mosaics_table.get_index<"byleadrating"_n>();
        
        auto by_lead_max = config::default_lead_grades.size();
        mosaic_num = 0;
        for (auto by_lead_itr = by_lead_idx.lower_bound(std::make_tuple(MAXINT64, MAXINT64)); 
                                                   (mosaic_num < by_lead_max) &&
                                                   (by_lead_itr != by_lead_idx.end()) &&
                                                   (by_lead_itr->lead_rating >= community.min_lead_rating); by_lead_itr++, mosaic_num++) {
            if (!by_lead_itr->hidden()) {
                ranked_mosaics[by_lead_itr->tracery].weight += config::default_lead_grades[mosaic_num];
            }
        }
        std::vector<std::pair<uint64_t, int64_t> > top_mosaics;
        top_mosaics.reserve(ranked_mosaics.size());
        for (const auto& m : ranked_mosaics) {
            top_mosaics.emplace_back(m.first, m.second.weight);
        }
        
        auto middle = top_mosaics.begin() + std::min(size_t(community.rewarded_mosaic_num), top_mosaics.size());
        std::partial_sort(top_mosaics.begin(), middle, top_mosaics.end(),
            [](const std::pair<uint64_t, int64_t>& lhs, const std::pair<uint64_t, int64_t>& rhs) { return lhs.second > rhs.second; });
        
        uint64_t grades_sum = 0;
        for (auto itr = top_mosaics.begin(); itr != middle; itr++) {
            grades_sum += itr->second;
        }
        
        gallery_types::stats stats_table(_self, commun_code.raw());
        const auto& stat = stats_table.get(commun_code.raw(), "SYSTEM: no stat but community present");
        auto total_reward = quantity.amount + stat.retained;
        auto left_reward  = total_reward;
        auto now = eosio::current_time_point();

        uint16_t place = 0;
        for (auto itr = top_mosaics.begin(); itr != middle; itr++) {
            auto cur_reward = safe_prop(total_reward, itr->second, grades_sum);
            
            auto mosaic = mosaics_table.find(itr->first);
            mosaics_table.modify(mosaic, name(), [&](auto& item) {
                if (item.last_top_date == stat.last_reward_date) {
                    item.reward += cur_reward;
                    left_reward -= cur_reward;
                    send_mosaic_event(_self, quantity.symbol, item);
                }
                item.last_top_date = now;
            });
            send_top_event(_self, commun_code, *mosaic, place++);
        }
        stats_table.modify(stat, name(), [&]( auto& s) {
            s.retained = left_reward;
            s.last_reward_date = now;
        });
    }

    void init_gallery(name _self, symbol_code commun_code) {
        commun_list::check_community_exists(commun_code);

        gallery_types::stats stats_table(_self, commun_code.raw());
        eosio::check(stats_table.find(commun_code.raw()) == stats_table.end(), "already exists");

        stats_table.emplace(_self, [&](auto& s) { s = {
            .id = commun_code.raw(),
            .last_reward_date = eosio::current_time_point()
        };});
    }

    void emit_for_gallery(name _self, symbol_code commun_code) {
        emit::issue_reward(commun_code, _self);
    }

    void create_mosaic(name _self, name creator, uint64_t tracery, name opus,
                       asset quantity, uint16_t royalty, gallery_types::providers_t providers) {
        require_auth(creator);
        auto commun_symbol = quantity.symbol;
        auto commun_code   = commun_symbol.code();

        auto& community = commun_list::get_community(commun_symbol);
        check(royalty <= community.author_percent, "incorrect royalty");
        check(providers.size() <= config::max_providers_num, "too many providers");
        
        const auto& op = community.get_opus(opus);
        
        auto points_sum = get_points_sum(quantity.amount, providers);
        eosio::check(op.min_mosaic_inclusion <= points_sum, "points are not enough for mosaic inclusion");
        
        emit::maybe_issue_reward(commun_code, _self);
        
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        eosio::check(mosaics_table.find(tracery) == mosaics_table.end(), "mosaic already exists");
        
        auto now = eosio::current_time_point();
        
        mosaics_table.emplace(creator, [&]( auto &item ) { item = gallery_types::mosaic_struct {
            .tracery = tracery,
            .creator = creator,
            .opus = opus,
            .royalty = royalty,
            .collection_end_date = now + eosio::seconds(community.collection_period),
            .gem_count = 0,
            .points = 0,
            .shares = 0
        };});

        auto claim_date = now + eosio::seconds(community.collection_period + community.moderation_period + community.extra_reward_period);
        int64_t pledge_points = std::min(points_sum, op.mosaic_pledge);
        int64_t shares_abs = points_sum > pledge_points ? points_sum : 0;
        freeze_in_gems(_self, true, tracery, claim_date, creator, quantity, providers, false, points_sum, shares_abs, pledge_points);
    }

    void add_to_mosaic(name _self, uint64_t tracery, asset quantity, bool damn, name gem_creator, gallery_types::providers_t providers) {
        require_auth(gem_creator);
        auto commun_symbol = quantity.symbol;
        auto commun_code = commun_symbol.code();
        
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto mosaic = mosaics_table.find(tracery);
        eosio::check(mosaic != mosaics_table.end(), "mosaic doesn't exist");

        auto& community = commun_list::get_community(commun_symbol);
        check(eosio::current_time_point() <= mosaic->collection_end_date, "collection period is over");
        check(!mosaic->banned(), "mosaic banned");
        check(!mosaic->hidden(), "mosaic hidden");
        check(mosaic->status == gallery_types::mosaic_struct::ACTIVE, "mosaic is inactive");
        
        emit::maybe_issue_reward(commun_code, _self);
        const auto& op = community.get_opus(mosaic->opus);
        
        auto points_sum = get_points_sum(quantity.amount, providers);
        eosio::check((providers.size() + 1) * op.min_gem_inclusion <= points_sum, "points are not enough for gem inclusion");
            
        int64_t pledge_points = std::max<int64_t>(std::min(points_sum, op.mosaic_pledge - mosaic->pledge_points), 0);
        
        auto shares_abs = damn ?
            calc_bancor_amount(mosaic->damn_points, mosaic->damn_shares, config::shares_cw, points_sum - pledge_points, false) :
            calc_bancor_amount(mosaic->points,      mosaic->shares,      config::shares_cw, points_sum - pledge_points, false);
        
        if (!damn) {
            shares_abs -= pay_royalties(_self, commun_symbol, tracery, mosaic->creator, safe_pct(mosaic->royalty, shares_abs));
        }

        auto claim_date = mosaic->collection_end_date + eosio::seconds(community.moderation_period + community.extra_reward_period);
        freeze_in_gems(_self, false, tracery, claim_date, gem_creator, quantity, providers, damn, points_sum, shares_abs, pledge_points);
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
    
    bool claim_gems_by_creator(name _self, uint64_t tracery, symbol_code commun_code, name gem_creator, bool eager, 
                               bool strict = true, std::optional<bool> damn = std::optional<bool>()) {
        auto now = eosio::current_time_point();
        auto claim_info = get_claim_info(_self, tracery, commun_code, eager);
        
        gallery_types::gems gems_table(_self, commun_code.raw());
        auto gems_idx = gems_table.get_index<"bycreator"_n>();
        auto gem = gems_idx.lower_bound(std::make_tuple(claim_info.tracery, gem_creator, name()));
        bool gem_found = false;
        while ((gem != gems_idx.end()) && (gem->tracery == claim_info.tracery) && (gem->creator == gem_creator)) {
            if (!damn.has_value() || *damn == (gem->shares < 0)) {
                gem_found = true;
                chop_gem(_self, claim_info.commun_symbol, gems_idx, gem, true, claim_info.has_reward, claim_info.premature);
                gem = gems_idx.erase(gem);
            }
            else {
                ++gem;
            }
        }
        eosio::check(gem_found || !strict, "nothing to claim");
        return gem_found;
    }
    
    void maybe_claim_old_gem(name _self, symbol commun_symbol, name gem_owner) {
        auto commun_code = commun_symbol.code();
        gallery_types::gems gems_table(_self, commun_code.raw());
        auto claim_idx = gems_table.get_index<"byclaim"_n>();
        auto gem_itr = claim_idx.lower_bound(std::make_tuple(gem_owner, time_point()));
        if ((gem_itr != claim_idx.end()) && (gem_itr->owner == gem_owner) && 
            (gem_itr->claim_date < eosio::current_time_point()) && chop_gem(_self, commun_symbol, claim_idx, gem_itr, false, true)) {
                
            claim_idx.erase(gem_itr);
        }
    }
    
    void provide_points(name _self, name grantor, name recipient, asset quantity, std::optional<uint16_t> fee) {
        require_auth(grantor);
        commun_list::check_community_exists(quantity.symbol);
        
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
            provs_table.emplace(grantor, [&] (auto &item) { item = gallery_types::provision_struct {
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
            advices_table.emplace(leader, [&] (auto &item) { item = gallery_types::advice_struct {
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

    void update_mosaic(name _self, symbol_code commun_code, uint64_t tracery) {
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto& mosaic = mosaics_table.get(tracery, "mosaic doesn't exist");
        require_auth(mosaic.creator);
        eosio::check(mosaic.status == gallery_types::mosaic_struct::ACTIVE, "mosaic is inactive");
        mosaics_table.modify(mosaic, eosio::same_payer, [&](auto& m) {
            m.lock_date = time_point();
        });
    }
    
    void set_lock_status(name _self, symbol_code commun_code, uint64_t tracery, bool lock) {
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto& mosaic = mosaics_table.get(tracery, "mosaic doesn't exist");
        mosaics_table.modify(mosaic, eosio::same_payer, [&](auto& m) {
            if (lock) { 
                m.lock();
            }
            else {
                auto& community = commun_list::get_community(commun_code);
                m.unlock(community.moderation_period);
                send_mosaic_event(_self, community.commun_symbol, m);
            }
        });
    }

    void ban_mosaic(name _self, symbol_code commun_code, uint64_t tracery) {
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto mosaic = mosaics_table.find(tracery);
        eosio::check(mosaic != mosaics_table.end(), "mosaic doesn't exist");
        eosio::check(!mosaic->banned(), "mosaic is already banned");
        eosio::check(mosaic->status != gallery_types::mosaic_struct::ARCHIVED, "mosaic is archived");
        mosaics_table.modify(mosaic, name(), [&](auto& item) {
            item.status = item.hidden() ? gallery_types::mosaic_struct::BANNED_AND_HIDDEN : gallery_types::mosaic_struct::BANNED;
        });
    }
    
    void hide_mosaic(name _self, symbol_code commun_code, uint64_t tracery) {
        gallery_types::mosaics mosaics_table(_self, commun_code.raw());
        auto& mosaic = mosaics_table.get(tracery, "mosaic doesn't exist");
        require_auth(mosaic.creator);
        eosio::check(!mosaic.hidden(), "mosaic is already hidden");
        mosaics_table.modify(mosaic, name(), [&](auto& item) {
            item.status = item.banned() ? gallery_types::mosaic_struct::BANNED_AND_HIDDEN : gallery_types::mosaic_struct::HIDDEN;
        });
    }
};

class
/// @cond
[[eosio::contract("commun.gallery")]]
/// @endcond
gallery : public gallery_base<gallery>, public contract {
public:
    using contract::contract;
    static void deactivate(name self, symbol_code commun_code, const gallery_types::mosaic_struct& mosaic) {};

    [[eosio::action]] void init(symbol_code commun_code) {
        require_auth(_self);
        init_gallery(_self, commun_code);
    }

    [[eosio::action]] void emit(symbol_code commun_code) {
        require_auth(_self);
        emit_for_gallery(_self, commun_code);
    }

    [[eosio::action]] void createmosaic(name creator, uint64_t tracery, name opus, asset quantity, uint16_t royalty, gallery_types::providers_t providers) {
        require_auth(creator);
        create_mosaic(_self, creator, tracery, opus, quantity, royalty, providers);
    }

    [[eosio::action]] void addtomosaic(uint64_t tracery, asset quantity, bool damn, name gem_creator, gallery_types::providers_t providers) {
        require_auth(gem_creator);
        add_to_mosaic(_self, tracery, quantity, damn, gem_creator, providers);
    }
    
    // [[eosio::action]] // TODO: removed from MVP
    void hold(uint64_t tracery, symbol_code commun_code, name gem_owner, std::optional<name> gem_creator) {
        hold_gem(_self, tracery, commun_code, gem_owner, gem_creator.value_or(gem_owner));
    }
    
    // [[eosio::action]] // TODO: removed from MVP
    void transfer(uint64_t tracery, symbol_code commun_code, name gem_owner, std::optional<name> gem_creator, name recipient) {
        transfer_gem(_self, tracery, commun_code, gem_owner, gem_creator.value_or(gem_owner), recipient);
    }
    
    [[eosio::action]] void claim(uint64_t tracery, symbol_code commun_code, name gem_owner, 
                                   std::optional<name> gem_creator, std::optional<bool> eager) {
        claim_gem(_self, tracery, commun_code, gem_owner, gem_creator.value_or(gem_owner), eager.value_or(false));
    }
    
    // [[eosio::action]] // TODO: removed from MVP
    void provide(name grantor, name recipient, asset quantity, std::optional<uint16_t> fee) {
        provide_points(_self, grantor, recipient, quantity, fee);
    }

    // [[eosio::action]] // TODO: removed from MVP
    void advise(symbol_code commun_code, name leader, std::set<uint64_t> favorites) {
        control::require_leader_auth(commun_code, leader);
        advise_mosaics(_self, commun_code, leader, favorites);
    }

    //TODO: [[eosio::action]] void checkadvice (symbol_code commun_code, name leader);

    [[eosio::action]] void update(symbol_code commun_code, uint64_t tracery) {
        update_mosaic(_self, commun_code, tracery);
    }

    [[eosio::action]] void lock(symbol_code commun_code, name leader, uint64_t tracery, string reason) {
        control::require_leader_auth(commun_code, leader);
        eosio::check(!reason.empty(), "Reason cannot be empty.");
        set_lock_status(_self, commun_code, tracery, true);
    }

    [[eosio::action]] void unlock(symbol_code commun_code, name leader, uint64_t tracery, string reason) {
        control::require_leader_auth(commun_code, leader);
        eosio::check(!reason.empty(), "Reason cannot be empty.");
        set_lock_status(_self, commun_code, tracery, false);
    }

    [[eosio::action]] void ban(symbol_code commun_code, uint64_t tracery) {
        require_auth(_self);
        ban_mosaic(_self, commun_code, tracery);
    }

    [[eosio::action]] void hide(symbol_code commun_code, uint64_t tracery) {
        hide_mosaic(_self, commun_code, tracery);
    }

    ON_TRANSFER(COMMUN_POINT) void ontransfer(name from, name to, asset quantity, std::string memo) {
        on_points_transfer(_self, from, to, quantity, memo);
    }      
};
    
} /// namespace commun
