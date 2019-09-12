#include "golos_tester.hpp"

namespace eosio { namespace testing {

class gallery_tester : public golos_tester {
public:
    gallery_tester(name code, bool push_genesis = true)
    	: golos_tester(code, push_genesis) {
    }

    struct gallery_errors : contract_error_messages {
        const string no_balance = amsg("point balance does not exist");
        const string no_reserve = amsg("no reserve");
        const string overdrawn_balance = amsg("overdrawn balance");
        const string eval_period = amsg("evaluation period isn't over yet");
        const string insufficient_quantity = amsg("insufficient quantity");
        const string no_mosaic = amsg("mosaic doesn't exist");
        const string collect_period = amsg("collection period is over");
        const string no_points_provided = amsg("no points provided");
        const string not_enough_provided = amsg("not enough provided points");
        const string gallery_exists = amsg("the gallery with this symbol already exists");
        const string no_param_no_gallery = amsg("param does not exists");
        const string wrong_royalty = amsg("incorrect royalty");
        const string too_many_providers = amsg("too many providers");
        const string cannot_freeze_zero = amsg("can't freeze zero points");
        const string points_negative = amsg("points must be positive");
        const string wrong_gem_type = amsg("gem type mismatch");
        const string nothing_to_claim = amsg("nothing to claim");
    } errgallery;

    variant get_mosaic(name code, symbol point, name creator, uint64_t tracery) {
        variant obj = get_chaindb_lower_bound_struct(code, point.to_symbol_code(), N(mosaic), N(bykey),
            std::make_pair(creator, tracery), "mosaic");
        if (!obj.is_null() && obj.get_object().size()) { 
            if (obj["creator"] == creator.to_string() && obj["tracery"] == std::to_string(tracery)) {
                return obj;
            }
        }
        return variant();
    }

    variant get_gem(name code, symbol point, uint64_t mosaic_id, name creator, name owner = name()) {
        if (!owner) {
            owner = creator;
        }
        variant obj = get_chaindb_lower_bound_struct(code, point.to_symbol_code(), N(gem), N(bykey),
            std::make_pair(mosaic_id, std::make_pair(creator, owner)), "gem");
        if (!obj.is_null() && obj.get_object().size()) { 
            if (obj["mosaic_id"] == std::to_string(mosaic_id) && obj["creator"] == creator.to_string() && obj["owner"] == owner.to_string()) {
                return obj;
            }
        }
        return variant();
    }
};

}} // eosio::testing
