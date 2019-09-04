#include "commun.publication.hpp"
#include <eosio/event.hpp>
#include "objects.hpp"
#include <commun.social/commun.social.hpp>


namespace commun {

extern "C" {
    void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        //publication(receiver).apply(code, action);
        auto execute_action = [&](const auto fn) {
            return eosio::execute_action(eosio::name(receiver), eosio::name(code), fn);
        };

#define NN(x) N(x).value

        if (receiver != code)
            return;

        if (NN(createmssg) == action)
            execute_action(&publication::create_message);
        if (NN(updatemssg) == action)
            execute_action(&publication::update_message);
        if (NN(deletemssg) == action)
            execute_action(&publication::delete_message);
        if (NN(upvote) == action)
            execute_action(&publication::upvote);
        if (NN(downvote) == action)
            execute_action(&publication::downvote);
        if (NN(unvote) == action)
            execute_action(&publication::unvote);
        if (NN(claim) == action)
            execute_action(&publication::claim);
        if (NN(setparams) == action)
            execute_action(&publication::set_params);
        if (NN(reblog) == action)
            execute_action(&publication::reblog);
        if (NN(erasereblog) == action)
            execute_action(&publication::erase_reblog);
        if (NN(addproviders) == action)
            execute_action(&publication::addproviders);
        if (NN(setfrequency) == action)
            execute_action(&publication::setfrequency);
    }
#undef NN
}

struct posting_params_setter: set_params_visitor<posting_state> {
    using set_params_visitor::set_params_visitor;

    bool operator()(const max_comment_depth_prm& param) {
        return set_param(param, &posting_state::max_comment_depth_param);
    }

    bool operator()(const social_acc_prm& param) {
        return set_param(param, &posting_state::social_acc_param);
    }
};

// cached to prevent unneeded db access
const posting_state& publication::params(symbol_code commun_code) {
    static const auto cfg = posting_params_singleton(_self, commun_code.raw()).get();
    return cfg;
}

void publication::create_message(
    symbol_code commun_code,
    mssgid_t message_id,
    mssgid_t parent_id,
    std::string headermssg,
    std::string bodymssg,
    std::string languagemssg,
    std::vector<std::string> tags,
    std::string jsonmetadata,
    uint16_t curators_prcnt
) {
    require_auth(message_id.author);

    eosio::check(message_id.permlink.length() && message_id.permlink.length() < config::max_length,
            "Permlink length is empty or more than 256.");
    eosio::check(validate_permlink(message_id.permlink), "Permlink contains wrong symbol.");
    eosio::check(headermssg.length() < config::max_length, "Title length is more than 256.");
    eosio::check(bodymssg.length(), "Body is empty.");
    eosio::check(curators_prcnt <= config::_100percent, "curators_prcnt can't be more than 100%.");

    const auto& max_comment_depth_param = params(commun_code).max_comment_depth_param;
    const auto& social_acc_param = params(commun_code).social_acc_param;
    
    vertices vertices_table(_self, commun_code.raw());
    auto vertices_index = vertices_table.get_index<"bykey"_n>();
    auto tracery = message_id.tracery();
    eosio::check(vertices_index.find(std::make_tuple(message_id.author, tracery)) == vertices_index.end(), "This message already exists.");
    
    uint64_t parent_pk = 0;
    uint16_t level = 0;
    uint64_t parent_tracery = 0;
    if(parent_id.author) {
        parent_tracery = parent_id.tracery();
        auto parent_vertex = vertices_index.find(std::make_tuple(parent_id.author, parent_tracery));
        eosio::check(parent_vertex != vertices_index.end(), "Parent message doesn't exist");

        vertices_index.modify(parent_vertex, eosio::same_payer, [&](auto& item) {
            ++item.childcount;
        });
        
        level = 1 + parent_vertex->level;
    }
    eosio::check(level <= max_comment_depth_param.max_comment_depth, "publication::create_message: level > MAX_COMMENT_DEPTH");
    
    vertices_table.emplace(message_id.author, [&]( auto &item) {
        item.id = vertices_table.available_primary_key();
        item.creator = message_id.author;
        item.tracery = tracery;
        item.parent_creator = parent_id.author;
        item.parent_tracery = parent_tracery;
        item.level = level;
        item.childcount = 0;
    });
    
    gallery_base::params params_table(_self, commun_code.raw());
    const auto& param = params_table.get(commun_code.raw(), "param does not exists");
    accparams accparams_table(_self, commun_code.raw());
    auto acc_param = get_acc_param(accparams_table, commun_code, message_id.author);
    
    asset quantity(
        get_amount_to_freeze(
            point::get_balance(config::commun_point_name, message_id.author, commun_code).amount, 
            get_frozen_amount(_self, message_id.author, commun_code),
            acc_param->actions_per_day, 
            param.mosaic_active_period), 
        param.commun_symbol);

    create_mosaic(_self, message_id.author, tracery, quantity, config::_100percent - curators_prcnt, get_providers(commun_code, message_id.author));    
}

void publication::update_message(symbol_code commun_code, mssgid_t message_id,
                              std::string headermssg, std::string bodymssg,
                              std::string languagemssg, std::vector<std::string> tags,
                              std::string jsonmetadata) {
                                
    require_auth(message_id.author);
    vertices vertices_table(_self, commun_code.raw());
    auto vertices_index = vertices_table.get_index<"bykey"_n>();
    eosio::check(vertices_index.find(std::make_tuple(message_id.author, message_id.tracery())) != vertices_index.end(), 
        "You can't update this message, because this message doesn't exist.");
}

void publication::delete_message(symbol_code commun_code, mssgid_t message_id) {
    require_auth(message_id.author);

    vertices vertices_table(_self, commun_code.raw());
    auto vertices_index = vertices_table.get_index<"bykey"_n>();
    auto vertex = vertices_index.find(std::make_tuple(message_id.author, message_id.tracery()));
    eosio::check(vertex != vertices_index.end(), "Permlink doesn't exist.");
    
    eosio::check(vertex->childcount == 0, "You can't delete comment with child comments.");

    if (vertex->parent_creator) {
        auto parent_vertex = vertices_index.find(std::make_tuple(vertex->parent_creator, vertex->parent_tracery));
        vertices_index.modify(parent_vertex, eosio::same_payer, [&](auto& item) {
            item.childcount--;
        });
    }
    vertices_table.erase(*vertex);
    
    auto tracery = message_id.tracery();
    
    mosaics mosaics_table(_self, commun_code.raw());
    auto mosaics_idx = mosaics_table.get_index<"bykey"_n>();
    auto mosaic = mosaics_idx.find(std::make_tuple(message_id.author, tracery));
    if (mosaic == mosaics_idx.end()) {
        return; 
    }
    gems gems_table(_self, commun_code.raw());
    auto gems_idx = gems_table.get_index<"bykey"_n>();

    for (auto gem_itr = gems_idx.lower_bound(std::make_tuple(mosaic->id, name(), name())); 
        (gem_itr != gems_idx.end()) && (gem_itr->mosaic_id == mosaic->id); ++gem_itr) {
        eosio::check(gem_itr->creator == message_id.author, "Unable to delete comment with votes.");
        eosio::check(gem_itr->owner == message_id.author, "Unable to delete comment created by providers.");
        claim_gem(_self, message_id.author, tracery, commun_code, message_id.author, message_id.author, std::optional<name>(), true);
    }
}

void publication::upvote(symbol_code commun_code, name voter, mssgid_t message_id, uint16_t weight) {
    eosio::check(weight > 0, "weight can't be 0.");
    eosio::check(weight <= config::_100percent, "weight can't be more than 100%.");
    set_vote(commun_code, voter, message_id, weight);
}

void publication::downvote(symbol_code commun_code, name voter, mssgid_t message_id, uint16_t weight) {
    eosio::check(weight > 0, "weight can't be 0.");
    eosio::check(weight <= config::_100percent, "weight can't be more than 100%.");
    set_vote(commun_code, voter, message_id, -weight);
}

void publication::unvote(symbol_code commun_code, name voter, mssgid_t message_id) {
    //unvote does not support provider gems. use claim
    claim_gem(_self, message_id.author, message_id.tracery(), commun_code, voter, voter, std::optional<name>(), true);
}

void publication::claim(name mosaic_creator, uint64_t tracery, symbol_code commun_code, name gem_owner, 
                        std::optional<name> gem_creator, std::optional<name> recipient, std::optional<bool> eager) {
    
    claim_gem(_self, mosaic_creator, tracery, commun_code, gem_owner, gem_creator, recipient, eager.value_or(false));
}

void publication::set_vote(symbol_code commun_code, name voter, const mssgid_t& message_id, int16_t weight) {
    
    gallery_base::params params_table(_self, commun_code.raw());
    const auto& param = params_table.get(commun_code.raw(), "param does not exists");
    accparams accparams_table(_self, commun_code.raw());
    auto acc_param = get_acc_param(accparams_table, commun_code, voter);
    
    uint16_t abs_weight = std::abs(weight);
    asset quantity(
        safe_pct(abs_weight, get_amount_to_freeze(
            point::get_balance(config::commun_point_name, voter, commun_code).amount, 
            get_frozen_amount(_self, voter, commun_code),
            acc_param->actions_per_day, 
            param.mosaic_active_period)), 
        param.commun_symbol);
    
    add_to_mosaic(_self, message_id.author, message_id.tracery(), quantity, weight < 0, voter, get_providers(commun_code, message_id.author, abs_weight));
}

void publication::set_params(symbol_code commun_code, std::vector<posting_params> params) {
    require_auth(_self);
    
    gallery_base::params params_table(_self, commun_code.raw());
    if (params_table.find(commun_code.raw()) == params_table.end()) {
        create_gallery(_self, point::get_supply(config::commun_point_name, commun_code).symbol);
    }
    
    posting_params_singleton cfg(_self, commun_code.raw());
    param_helper::check_params(params, cfg.exists());
    param_helper::set_parameters<posting_params_setter>(params, cfg, _self);
}

void publication::reblog(symbol_code commun_code, name rebloger, mssgid_t message_id, std::string headermssg, std::string bodymssg) {
    require_auth(rebloger);

    eosio::check(rebloger != message_id.author, "You cannot reblog your own content.");
    eosio::check(headermssg.length() < config::max_length, "Title length is more than 256.");
    eosio::check(
        !headermssg.length() || (headermssg.length() && bodymssg.length()),
        "Body must be set if title is set."
    );
    
    vertices vertices_table(_self, commun_code.raw());
    auto vertices_index = vertices_table.get_index<"bykey"_n>();
    eosio::check(vertices_index.find(std::make_tuple(message_id.author, message_id.tracery())) != vertices_index.end(), 
        "You can't reblog, because this message doesn't exist.");
}

void publication::erase_reblog(symbol_code commun_code, name rebloger, mssgid_t message_id) {
    require_auth(rebloger);
    eosio::check(rebloger != message_id.author, "You cannot erase reblog your own content.");
    
    vertices vertices_table(_self, commun_code.raw());
    auto vertices_index = vertices_table.get_index<"bykey"_n>();
    eosio::check(vertices_index.find(std::make_tuple(message_id.author, message_id.tracery())) != vertices_index.end(), 
        "You can't erase reblog, because this message doesn't exist.");
}

bool publication::validate_permlink(std::string permlink) {
    for (auto symbol : permlink) {
        if ((symbol >= '0' && symbol <= '9') ||
            (symbol >= 'a' && symbol <= 'z') ||
             symbol == '-') {
            continue;
        } else {
            return false;
        }
    }
    return true;
}

accparams::const_iterator publication::get_acc_param(accparams& accparams_table, symbol_code commun_code, name account) {
    auto ret = accparams_table.find(account.value);
    if (ret == accparams_table.end()) {
        ret = accparams_table.emplace(account, [&](auto& p) { p = {
            .account = account,
            .actions_per_day = config::default_actions_per_day
        };});
    }
    return ret;
}

int64_t publication::get_amount_to_freeze(int64_t balance, int64_t frozen, uint16_t actions_per_day, int64_t mosaic_active_period, int64_t actual_limit) {
    int64_t available = balance - frozen;
    if (available <= 0 || actual_limit <= 0) {
        return 0;
    }
    static const int64_t seconds_per_day = 24 * 60 * 60;
    int64_t actions_per_period = std::max<int64_t>(safe_prop(actions_per_day, mosaic_active_period, seconds_per_day), 1);
    int64_t points_per_action = balance / actions_per_period;
    return std::min(points_per_action ? std::min(available, points_per_action) : available, actual_limit);
}

gallery_base::opt_providers_t publication::get_providers(symbol_code commun_code, name account, uint16_t weight) {
    gallery_base::params params_table(_self, commun_code.raw());
    const auto& param = params_table.get(commun_code.raw(), "param does not exists");
    accparams accparams_table(_self, commun_code.raw());
    auto acc_param = get_acc_param(accparams_table, commun_code, account);
    provs provs_table(_self, commun_code.raw());
    std::vector<std::pair<name, int64_t> > ret;
    auto provs_index = provs_table.get_index<"bykey"_n>();
    for (size_t n = 0; n < acc_param->providers.size(); n++) {
        auto prov_name = acc_param->providers[n];
        auto prov_itr = provs_index.find(std::make_tuple(prov_name, account));
        if (prov_itr != provs_index.end() && point::balance_exists(config::commun_point_name, prov_name, commun_code)) {
            auto amount = safe_pct(weight, get_amount_to_freeze(
                prov_itr->total, 
                prov_itr->frozen,
                acc_param->actions_per_day, 
                param.mosaic_active_period,
                point::get_balance(config::commun_point_name, prov_name, commun_code).amount - get_frozen_amount(_self, prov_name, commun_code)));
        
            if (amount) {
                ret.emplace_back(std::make_pair(prov_name, amount));
            }
        }
        else {
            accparams_table.modify(acc_param, name(), [&](auto& a) { a.providers[n] = name(); });
        }
    }
    accparams_table.modify(acc_param, name(), [&](auto& a) {
        a.providers.erase(
            std::remove_if(a.providers.begin(), a.providers.end(), [](const name& p) { return p == name(); }),
            a.providers.end());
    });
    return ret.size() ? gallery_base::opt_providers_t(ret) : gallery_base::opt_providers_t();
}

void publication::addproviders(symbol_code commun_code, name recipient, std::vector<name> providers) {
    require_auth(recipient);
    gallery_base::params params_table(_self, commun_code.raw());
    const auto& param = params_table.get(commun_code.raw(), "param does not exists");
    accparams accparams_table(_self, commun_code.raw());
    auto acc_param = get_acc_param(accparams_table, commun_code, recipient);
    
    accparams_table.modify(acc_param, name(), [&](auto& a) {
        a.providers.insert(a.providers.end(), providers.begin(), providers.end());
        sort(a.providers.begin(), a.providers.end());
        a.providers.erase(unique(a.providers.begin(), a.providers.end()), a.providers.end());
    });
    
    provs provs_table(_self, commun_code.raw());
    auto provs_index = provs_table.get_index<"bykey"_n>();
    for (size_t n = 0; n < acc_param->providers.size(); n++) {
        auto prov_name = acc_param->providers[n];
        auto prov_itr = provs_index.find(std::make_tuple(prov_name, recipient));
        if (prov_itr == provs_index.end() || !point::balance_exists(config::commun_point_name, prov_name, commun_code)) {
            accparams_table.modify(acc_param, name(), [&](auto& a) { a.providers[n] = name(); });
        }
    }
    accparams_table.modify(acc_param, name(), [&](auto& a) {
        a.providers.erase(
            std::remove_if(a.providers.begin(), a.providers.end(), [](const name& p) { return p == name(); }),
            a.providers.end());
    });
    eosio::check(acc_param->providers.size() <= config::max_providers_num, "too many providers");
}

void publication::setfrequency(symbol_code commun_code, name account, uint16_t actions_per_day) {
    require_auth(account);
    accparams accparams_table(_self, commun_code.raw());
    auto acc_param = get_acc_param(accparams_table, commun_code, account);
    accparams_table.modify(acc_param, name(), [&](auto& a) { a.actions_per_day = actions_per_day; });
}

} // commun
