#include "golos_tester.hpp"
#include "commun.ctrl_test_api.hpp"

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
        const string no_authority = amsg("lack of necessary authority");
        const string already_done = amsg("already done");
        const string mosaic_is_inactive = amsg("mosaic is inactive");
        const string mosaic_banned = amsg("mosaic banned");
        const string not_enough_for_pledge = amsg("points are not enough for a pledge");
        const string not_enough_for_mosaic = amsg("points are not enough for mosaic inclusion");
        const string not_enough_for_gem    = amsg("points are not enough for gem inclusion");
        
        const string not_a_leader(account_name leader) { return amsg((leader.to_string() + " is not a leader")); }
        
    } errgallery;
    
    void prepare_ctrl(commun_ctrl_api& ctrl, account_name community, const std::vector<account_name>& leaders, account_name voter, 
                uint16_t max_witnesses, uint16_t max_witness_votes) {

        BOOST_CHECK_EQUAL(success(), ctrl.set_params(ctrl.default_params(community, max_witnesses, max_witness_votes)));
        produce_block();
        ctrl.prepare_multisig(community);
        produce_block();
        for (int i = 0; i < leaders.size(); i++) {
            BOOST_CHECK_EQUAL(success(), ctrl.reg_witness(leaders[i], "localhost"));
            BOOST_CHECK_EQUAL(success(), ctrl.vote_witness(voter, leaders[i]));
        }
        produce_block();
    }

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
