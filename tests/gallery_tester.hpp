#include "golos_tester.hpp"
#include "commun.ctrl_test_api.hpp"

namespace eosio { namespace testing {

class gallery_tester : public golos_tester {
public:
    gallery_tester(name code, bool push_genesis = true)
        : golos_tester(code, push_genesis) {
    }
    enum status_t: uint8_t { ACTIVE, MODERATE, ARCHIVED, LOCKED, BANNED, HIDDEN, BANNED_AND_HIDDEN };

    struct gallery_errors : contract_error_messages {
        const string no_balance = amsg("point balance does not exist");
        const string no_reserve = amsg("no reserve");
        const string overdrawn_balance = amsg("overdrawn balance");
        const string moderation_period = amsg("moderation period isn't over yet");
        const string insufficient_quantity = amsg("insufficient quantity");
        const string no_mosaic = amsg("mosaic doesn't exist");
        const string collect_period = amsg("collection period is over");
        const string no_points_provided = amsg("no points provided");
        const string not_enough_provided = amsg("not enough provided points");
        const string refill = amsg("can't refill the gem");
        const string gallery_exists = amsg("the gallery with this symbol already exists");
        const string symbol_precision = amsg("symbol precision mismatch");
        const string wrong_royalty = amsg("incorrect royalty");
        const string too_many_providers = amsg("too many providers");
        const string cannot_freeze_zero = amsg("can't freeze zero points");
        const string points_negative = amsg("points must be positive");
        const string wrong_gem_type = amsg("gem type mismatch");
        const string nothing_to_claim = amsg("nothing to claim");
        const string no_authority = amsg("lack of necessary authority");
        const string already_done = amsg("already done");
        const string mosaic_is_inactive = amsg("mosaic is inactive");
        const string mosaic_already_hidden = amsg("mosaic is already hidden");
        const string mosaic_banned = amsg("mosaic banned");
        const string mosaic_already_banned = amsg("mosaic is already banned");
        const string mosaic_archived = amsg("mosaic is archived");
        const string mosaic_not_locked = amsg("mosaic not locked");
        const string not_enough_for_pledge = amsg("points are not enough for a pledge");
        const string not_enough_for_mosaic = amsg("points are not enough for mosaic inclusion");
        const string not_enough_for_gem    = amsg("points are not enough for gem inclusion");
        const string advice_surfeit       = amsg("a surfeit of advice");
        const string no_changes_favorites = amsg("no changes in favorites");
        const string reason_empty         = amsg("Reason cannot be empty.");
        const string lock_without_modify  = amsg("Mosaic should be modified to lock again.");

        const string not_a_leader(account_name leader) { return amsg((leader.to_string() + " is not a leader")); }

        const string no_community = amsg("community not exists");
    } errgallery;
    
    variant get_mosaic(name code, symbol point, uint64_t tracery) {
        return get_chaindb_struct(code, point.to_symbol_code().value, N(mosaic), tracery, "mosaic");
    }

    variant get_gem(name code, symbol point, uint64_t tracery, name creator, name owner = name()) {
        if (!owner) {
            owner = creator;
        }
        variant obj = get_chaindb_lower_bound_struct(code, point.to_symbol_code(), N(gem), N(bykey),
            std::make_pair(tracery, std::make_pair(creator, owner)), "gem");
        if (!obj.is_null() && obj.get_object().size()) { 
            if (obj["tracery"] == std::to_string(tracery) && obj["creator"] == creator.to_string() && obj["owner"] == owner.to_string()) {
                return obj;
            }
        }
        return variant();
    }

    variant get_advice(name code, symbol point, name leader) {
        return get_chaindb_struct(code, point.to_symbol_code().value, N(advice), leader.value, "advice");
    }
    
    variant get_stat(name code, symbol point) {
        return get_chaindb_struct(code, point.to_symbol_code().value, N(stat), point.to_symbol_code().value, "stat");
    }
    
    int64_t calc_bancor_amount(int64_t current_reserve, int64_t current_supply, double cw, int64_t reserve_amount) {
        if (!current_reserve) { return reserve_amount; }
        double buy_prop = static_cast<double>(reserve_amount) / static_cast<double>(current_reserve);
        double new_supply = static_cast<double>(current_supply) * std::pow(1.0 + buy_prop, cw);
        return static_cast<int64_t>(new_supply) - current_supply;
    }
};

}} // eosio::testing
