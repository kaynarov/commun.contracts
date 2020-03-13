#include "commun.ctrl/commun.ctrl.hpp"
#include "commun.ctrl/config.hpp"
#include "commun.point/commun.point.hpp"
#include "commun.emit/commun.emit.hpp"
//#include <golos.vesting/golos.vesting.hpp>
#include <commun/parameter_ops.hpp>
//#include <cyber.system/native.hpp>
#include <eosio/transaction.hpp>
#include <eosio/event.hpp>
#include <eosio/permission.hpp>
#include <eosio/crypto.hpp>
#include <cyber.bios/cyber.bios.hpp>

namespace commun {

using namespace eosio;
using std::vector;
using std::string;

/// control
void control::check_started(symbol_code commun_code) {
    commun_list::get_control_param(commun_code);
}

void control::init(symbol_code commun_code) {
    require_auth(_self);
    commun_list::check_community_exists(commun_code); // stats is only for community leaders, not dApp

    stats stats_table(_self, commun_code.raw());
    eosio::check(stats_table.find(commun_code.raw()) == stats_table.end(), "already exists");

    stats_table.emplace(_self, [&](auto& s) { s = { .id = commun_code.raw() };});
}

void control::on_points_transfer(name from, name to, asset quantity, std::string memo) {
    (void) memo;
    if (_self != to) { return; }
    
    auto commun_code = quantity.symbol.code();
    const auto l = commun_list::get_control_param(commun_code).leaders_num;
    leader_tbl leader_table(_self, commun_code.raw());
    auto idx = leader_table.get_index<"byweight"_n>();
    
    stats stats_table(_self, commun_code.raw());
    const auto& stat = stats_table.get(commun_code.raw(), "stat does not exists");
    
    uint64_t weight_sum = 0;
    size_t i = 0;
    for (auto itr = idx.begin(); itr != idx.end() && i < l; ++itr) {
        if (itr->active && itr->total_weight > 0) {
            weight_sum += itr->total_weight;
            ++i;
        }
    }
    
    auto left_reward = quantity.amount + stat.retained;
    if (weight_sum) {
        int64_t reward_sum = safe_prop(left_reward, i, l);
        i = 0;
        for (auto itr = idx.begin(); itr != idx.end() && i < l; ++itr) {
            if (itr->active && itr->total_weight > 0) {
                auto leader_reward = safe_prop(reward_sum, itr->total_weight, weight_sum);
                idx.modify(itr, eosio::same_payer, [&](auto& w) { w.unclaimed_points += leader_reward; });
                left_reward -= leader_reward;
                ++i;
            }
        }
    }
    
    stats_table.modify(stat, name(), [&]( auto& s) { s.retained = left_reward; });
}

void control::regleader(symbol_code commun_code, name leader, string url) {
    check_started(commun_code);
    eosio::check(url.length() <= config::leader_max_url_size, "url too long");
    require_auth(leader);

    upsert_tbl<leader_tbl>(_self, commun_code.raw(), leader, leader.value, [&](bool exists) {
        return [&,exists](leader_info& w) {
            w.name = leader;
            w.active = true;
        };
    });
}

void control::clearvotes(symbol_code commun_code, name leader, std::optional<uint16_t> count) {
    check_started(commun_code);
    require_auth(leader);
    leader_tbl leader_table(_self, commun_code.raw());
    auto leader_it = leader_table.find(leader.value);
    eosio::check(leader_it != leader_table.end(), "leader not found");
    eosio::check(!leader_it->active, "leader must be deactivated");
    eosio::check(leader_it->counter_votes, "no votes");
    eosio::check(!count.has_value() || *count <= config::max_clearvotes_count, "incorrect count");
    uint16_t actual_count = count.value_or(config::max_clearvotes_count);
    
    int64_t diff_weight = 0;
    leader_vote_tbl tbl(_self, commun_code.raw());
    auto idx = tbl.get_index<"byleader"_n>();
    uint16_t i = 0;
    for (auto itr = idx.lower_bound(std::make_tuple(leader, name())); itr != idx.end() && itr->leader == leader && i < actual_count;) {
        diff_weight += get_power(commun_code, itr->voter, itr->pct);
        itr = idx.erase(itr);
        i++;
    }
    leader_table.modify(leader_it, eosio::same_payer, [&](auto& w) {
        w.counter_votes -= i;
        w.total_weight -= diff_weight;
        send_leader_event(commun_code, w);
    });
}

void control::unregleader(symbol_code commun_code, name leader) {
    check_started(commun_code);
    require_auth(leader);

    leader_tbl leader_table(_self, commun_code.raw());
    auto it = leader_table.find(leader.value);
    eosio::check(it != leader_table.end(), "leader not found");
    eosio::check(!it->counter_votes, "not possible to remove leader as there are votes");
    leader_table.erase(*it);
}

void control::stopleader(symbol_code commun_code, name leader) {
    check_started(commun_code);
    require_auth(leader);
    active_leader(commun_code, leader, false);
}

void control::startleader(symbol_code commun_code, name leader) {
    check_started(commun_code);
    require_auth(leader);
    active_leader(commun_code, leader, true);
}

void control::voteleader(symbol_code commun_code, name voter, name leader, std::optional<uint16_t> pct) {
    check_started(commun_code);
    require_auth(voter);
    eosio::check(!pct.has_value() || *pct, "pct can't be 0");
    eosio::check(!pct.has_value() || *pct <= config::_100percent, "pct can't be greater than 100%");
    eosio::check(!pct.has_value() || *pct % (10 * config::_1percent) == 0, "incorrect pct");

    leader_tbl leader_table(_self, commun_code.raw());
    auto leader_it = leader_table.find(leader.value);
    eosio::check(leader_it != leader_table.end(), "leader not found");
    eosio::check(leader_it->active, "leader not active");
    
    uint16_t prev_votes_sum = 0;
    uint8_t votes_num = 0;
    leader_vote_tbl tbl(_self, commun_code.raw());
    auto idx = tbl.get_index<"byvoter"_n>();
    for (auto itr = idx.lower_bound(std::make_tuple(voter, name())); itr != idx.end() && itr->voter == voter; itr++) {
        eosio::check(itr->leader != leader, "already voted");
        prev_votes_sum += itr->pct;
        votes_num++;
    }
    
    eosio::check(votes_num < commun_list::get_control_param(commun_code).max_votes, "all allowed votes already casted");
    eosio::check(prev_votes_sum <= config::_100percent, "SYSTEM: incorrect prev_votes_sum");
    eosio::check(prev_votes_sum < config::_100percent, "all power already casted");
    eosio::check(!pct.has_value() || prev_votes_sum + *pct <= config::_100percent, "all votes exceed 100%");
    
    uint16_t actual_pct = pct.has_value() ? *pct : config::_100percent - prev_votes_sum;
    
    tbl.emplace(voter, [&](auto& v) { v = {
        .id = tbl.available_primary_key(),
        .voter = voter,
        .leader = leader,
        .pct = actual_pct
    };});
    
    leader_table.modify(leader_it, eosio::same_payer, [&](auto& w) {
        ++w.counter_votes;
        w.total_weight += get_power(commun_code, voter, actual_pct);
        send_leader_event(commun_code, w);
    });
    
    if (commun_code) {
        emit::maybe_issue_reward(commun_code, _self);
    }
}

void control::unvotelead(symbol_code commun_code, name voter, name leader) {
    check_started(commun_code);
    require_auth(voter);

    leader_tbl leader_table(_self, commun_code.raw());
    auto leader_it = leader_table.find(leader.value);
    eosio::check(leader_it != leader_table.end(), "leader not found");

    leader_vote_tbl tbl(_self, commun_code.raw());
    auto idx = tbl.get_index<"byvoter"_n>();
    auto itr = idx.find(std::make_tuple(voter, leader));
    eosio::check(itr != idx.end(), "there is no vote for this leader");
    
    leader_table.modify(leader_it, eosio::same_payer, [&](auto& w) {
        --w.counter_votes;
        w.total_weight -= get_power(commun_code, voter, itr->pct);
        send_leader_event(commun_code, w);
    });
    
    idx.erase(itr);
    
    if (commun_code) {
        emit::maybe_issue_reward(commun_code, _self);
    }
}

void control::claim(symbol_code commun_code, name leader) {
    check_started(commun_code);
    require_auth(leader);
    
    leader_tbl leader_table(_self, commun_code.raw());
    auto leader_it = leader_table.find(leader.value);
    eosio::check(leader_it != leader_table.end(), "leader not found");
    eosio::check(leader_it->unclaimed_points >= 0, "SYSTEM: incorrect unclaimed_points");
    eosio::check(leader_it->unclaimed_points, "nothing to claim");
    
    INLINE_ACTION_SENDER(point, transfer)(config::point_name, {_self, config::transfer_permission},
        {_self, leader, asset(leader_it->unclaimed_points, point::get_supply(commun_code).symbol), "claimed points"});
    
    leader_table.modify(leader_it, eosio::same_payer, [&](auto& w) {
        w.unclaimed_points = 0;
    });
}

void control::emit(symbol_code commun_code) {
    check_started(commun_code);
    require_auth(_self);

    emit::issue_reward(commun_code, _self);
}

int64_t control::get_power(symbol_code commun_code, name voter, uint16_t pct = config::_100percent) {
    return safe_pct(pct, commun_code ? 
        point::get_balance(voter, commun_code).amount :
        point::get_assigned_reserve_amount(voter));
}

void control::changepoints(name who, asset diff) {
    symbol_code commun_code = diff.symbol.code();
    require_auth(_self);
    eosio::check(diff.amount != 0, "diff is 0. something broken");          // in normal conditions sender must guarantee it
    leader_tbl leader_table(_self, commun_code.raw());
    auto total_power = get_power(commun_code, who);
    
    leader_vote_tbl tbl(_self, commun_code.raw());
    auto idx = tbl.get_index<"byvoter"_n>();
    for (auto itr = idx.lower_bound(std::make_tuple(who, name())); itr != idx.end() && itr->voter == who; itr++) {
        auto diff_weight = safe_pct(itr->pct, total_power) - safe_pct(itr->pct, total_power - diff.amount);
        auto leader_it = leader_table.find(itr->leader.value);
        eosio::check(leader_it != leader_table.end(), "SYSTEM: leader not found: " + itr->leader.to_string());
        leader_table.modify(leader_it, eosio::same_payer, [&](auto& w) {
            w.total_weight += diff_weight;
            send_leader_event(commun_code, w);
        });
    }
}

void control::send_leader_event(symbol_code commun_code, const leader_info& wi) {
    leaderstate_event data{commun_code, wi.name, wi.total_weight, wi.active};
    eosio::event(_self, "leaderstate"_n, data).send();
}

void control::active_leader(symbol_code commun_code, name leader, bool flag) {
    // TODO: simplify upsert to allow passing just inner lambda
    bool exists = upsert_tbl<leader_tbl>(_self, commun_code.raw(), _self, leader.value, [&](bool) {
        return [&](leader_info& w) {
            eosio::check(flag != w.active, "active flag not updated");
            w.active = flag;

            send_leader_event(commun_code, w);
        };
    }, false);
    eosio::check(exists, "leader not found");
}

vector<leader_info> control::top_leader_info(symbol_code commun_code) {
    vector<leader_info> top;
    const auto l = commun_list::get_control_param(commun_code).leaders_num;
    top.reserve(l);
    leader_tbl leader(_self, commun_code.raw());
    auto idx = leader.get_index<"byweight"_n>();    // this index ordered descending
    for (auto itr = idx.begin(); itr != idx.end() && top.size() < l; ++itr) {
        if (itr->active && itr->total_weight > 0)
            top.emplace_back(*itr);
    }
    return top;
}

vector<name> control::top_leaders(symbol_code commun_code) {
    const auto& wi = top_leader_info(commun_code);
    vector<name> top(wi.size());
    std::transform(wi.begin(), wi.end(), top.begin(), [](auto& w) {
        return w.name;
    });
    return top;
}

//multisig:

constexpr uint8_t calc_req(uint8_t top, uint8_t num, uint8_t denom) {
    return static_cast<uint16_t>(top) * num / denom + 1;
}

uint8_t control::get_required(symbol_code commun_code, name permission) {
    eosio::check(!commun_code || permission != config::active_name, "permission not available");
    
    auto control_param = commun_list::get_control_param(commun_code);
    auto custom_thr = std::find_if(control_param.custom_thresholds.begin(), control_param.custom_thresholds.end(), 
        [&](const structures::threshold& t) { return t.permission == permission; });
    if (custom_thr != control_param.custom_thresholds.end()) {
        return custom_thr->required;
    }
    
    uint8_t req = 0;
    if (permission == config::super_majority_name) { req = calc_req(control_param.leaders_num, 2, 3); }
    else if (permission == config::majority_name)  { req = calc_req(control_param.leaders_num, 1, 2); }
    else if (permission == config::minority_name)  { req = calc_req(control_param.leaders_num, 1, 3); }
    else { eosio::check(false, "unknown permission"); }
    
    return req;
}

void control::propose(ignore<symbol_code> commun_code,
                      ignore<name> proposer,
                      ignore<name> proposal_name,
                      ignore<name> permission,
                      ignore<transaction> trx) {

    symbol_code _commun_code;
    name _proposer;
    name _proposal_name;
    name _permission;
    transaction_header _trx_header;

    _ds >> _commun_code >> _proposer >> _proposal_name >> _permission;

    const char* trx_pos = _ds.pos();
    size_t size    = _ds.remaining();
    _ds >> _trx_header;
    
    require_auth(_proposer);
    eosio::check(in_the_top(_commun_code, _proposer), _proposer.to_string() + " is not a leader");
    
    auto governance = _commun_code ? point::get_issuer(_commun_code) : config::dapp_name;
    
    eosio::check(_trx_header.expiration >= eosio::current_time_point(), "transaction expired");

    proposals proptable(_self, _proposer.value);
    eosio::check(proptable.find( _proposal_name.value ) == proptable.end(), "proposal with the same name exists");

    auto packed_requested = pack(std::vector<permission_level>{{governance, _permission}}); 

    auto res = eosio::check_transaction_authorization(trx_pos, size,
                                                 (const char*)0, 0,
                                                 packed_requested.data(), packed_requested.size());
 
    eosio::check(res > 0, "transaction authorization failed");
    
    std::vector<char> pkd_trans;
    pkd_trans.resize(size);
    memcpy((char*)pkd_trans.data(), trx_pos, size);
    proptable.emplace(_proposer, [&]( auto& prop) {
        prop.proposal_name       = _proposal_name;
        prop.commun_code         = _commun_code;
        prop.permission          = _permission;
        prop.packed_transaction  = pkd_trans;
    });

    approvals apptable(_self, _proposer.value);
    apptable.emplace( _proposer, [&]( auto& a ) {
        a.proposal_name = _proposal_name;
    });
}

void control::approve(name proposer, name proposal_name, name approver, const eosio::binary_extension<eosio::checksum256>& proposal_hash) {
    require_auth(approver);
    proposals proptable(_self, proposer.value);
    auto& prop = proptable.get(proposal_name.value, "proposal not found");
    
    eosio::check(in_the_top(prop.commun_code, approver), approver.to_string() + " is not a leader");

    if(proposal_hash) {
        assert_sha256(prop.packed_transaction.data(), prop.packed_transaction.size(), *proposal_hash);
    }

    approvals apptable(_self, proposer.value);
    auto& apps = apptable.get(proposal_name.value, "proposal not found");
    
    eosio::check(std::find_if(apps.provided_approvals.begin(), apps.provided_approvals.end(), [&](const approval& a) { return a.approver == approver; })
        == apps.provided_approvals.end(), "already approved");

    apptable.modify(apps, approver, [&]( auto& a ) { a.provided_approvals.push_back(approval{approver, current_time_point()}); });
}

void control::unapprove(name proposer, name proposal_name, name approver) {
    require_auth(approver);

    approvals apptable( _self, proposer.value);
    auto& apps = apptable.get(proposal_name.value, "proposal not found");
    auto itr = std::find_if( apps.provided_approvals.begin(), apps.provided_approvals.end(), [&](const approval& a) { return a.approver == approver; } );
    eosio::check(itr != apps.provided_approvals.end(), "no approval previously granted");
    
    apptable.modify(apps, approver, [&]( auto& a ) { a.provided_approvals.erase(itr); });
}

void control::cancel(name proposer, name proposal_name, name canceler) {
    require_auth(canceler);

    proposals proptable(_self, proposer.value);
    auto& prop = proptable.get( proposal_name.value, "proposal not found" );

    if(canceler != proposer) {
        eosio::check(unpack<transaction_header>( prop.packed_transaction ).expiration < current_time_point(), "cannot cancel until expiration");
    }
    proptable.erase(prop);

    approvals apptable( _self, proposer.value);
    auto& apps = apptable.get(proposal_name.value, "SYSTEM: proposal not found");
    apptable.erase(apps);
}


void control::exec(name proposer, name proposal_name, name executer) {
    require_auth(executer);

    proposals proptable(_self, proposer.value);
    auto& prop = proptable.get(proposal_name.value, "proposal not found");
    transaction trx;
    datastream<const char*> ds(prop.packed_transaction.data(), prop.packed_transaction.size());
    ds >> trx;
    eosio::check(trx.expiration >= current_time_point(), "transaction expired");
    
    auto governance = prop.commun_code ? point::get_issuer(prop.commun_code) : config::dapp_name;
    
    auto packed_requested = pack(std::vector<permission_level>{{governance, prop.permission}});
    auto res = eosio::check_transaction_authorization(prop.packed_transaction.data(), prop.packed_transaction.size(),
                                             (const char*)0, 0,
                                             packed_requested.data(), packed_requested.size());
    eosio::check(res > 0, "transaction authorization is no longer satisfied");

    auto required = get_required(prop.commun_code, prop.permission);

    auto top = top_leaders(prop.commun_code);
    std::sort(top.begin(), top.end());
    
    uint16_t approvals_num = 0;

    approvals apptable( _self, proposer.value);
    auto& apps = apptable.get(proposal_name.value, "SYSTEM: proposal not found");
    invalidations inv_table(_self, _self.value);
    
    for (auto& p : apps.provided_approvals) {
        if (std::binary_search(top.begin(), top.end(), p.approver)) {
            auto it = inv_table.find(p.approver.value);
            if (it == inv_table.end() || it->last_invalidation_time < p.time) {
                ++approvals_num;
                if (approvals_num >= required) {
                    break;
                }
            }
        }
    }
    eosio::check(approvals_num >= required, "transaction authorization failed");
    apptable.erase(apps);
    
    for (const auto& a : trx.context_free_actions) {
        a.send_context_free();
    }
    for (const auto& a : trx.actions) {
        a.send();
    }

    proptable.erase(prop);
}

void control::invalidate(name account) {
    require_auth(account);
    invalidations inv_table(_self, _self.value);
    auto it = inv_table.find(account.value);
    if (it == inv_table.end()) {
        inv_table.emplace(account, [&](auto& i) {
            i.account = account;
            i.last_invalidation_time = eosio::current_time_point();
        });
    }
    else {
        inv_table.modify(it, account, [&](auto& i) {
            i.last_invalidation_time = eosio::current_time_point();
        });
    }
}

void control::setrecover() {
    require_auth(_self);

    cyber::authority auth;

    auto top = top_leaders(symbol_code());
    std::sort(top.begin(), top.end());

    for (const auto& i : top) {
        auth.accounts.push_back({{i,config::active_name},1});
    }

    auth.threshold = get_required(symbol_code(), config::super_majority_name);
    action(
        permission_level{config::dapp_name, config::active_name},
        config::internal_name, "updateauth"_n,
        std::make_tuple(config::dapp_name, config::recovery_name, config::active_name, auth)
    ).send();
}

} // commun
