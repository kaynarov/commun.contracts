#include "commun.ctrl/commun.ctrl.hpp"
#include "commun.ctrl/config.hpp"
#include "commun.point/commun.point.hpp"
//#include <golos.vesting/golos.vesting.hpp>
#include <commun/parameter_ops.hpp>
#include <commun/dispatchers.hpp>
//#include <cyber.system/native.hpp>
#include <eosio/transaction.hpp>
#include <eosio/event.hpp>
#include <eosio/permission.hpp>
#include <eosio/crypto.hpp>

namespace commun {


using namespace eosio;
using std::vector;
using std::string;

void control::setparams(symbol_code commun_code, std::optional<structures::control_param> p) {
    require_auth(_self);
    //it is in 2 tables, for convenience when moving to commun_list
    if (!commun_code) {
        tables::sysparams sysparams_table(_self, _self.value);
        structures::sysparam sysparam;
        if (p.has_value()) {
            sysparam.control = *p;
        }
        else {
            sysparam.control.leaders_num = config::default_dapp_leaders_num;
            sysparam.control.thresholds = std::vector<structures::threshold>
                (config::default_dapp_thresholds.begin(), config::default_dapp_thresholds.end());
            sysparam.control.max_votes = config::default_dapp_max_votes;
        }
        sysparams_table.set(sysparam, _self);
    }
    else {
        structures::control_param control_param;
        if (p.has_value()) {
            control_param = *p;
        }
        else {
            control_param.leaders_num = config::default_comm_leaders_num;
            control_param.thresholds = std::vector<structures::threshold>
                (config::default_comm_thresholds.begin(), config::default_comm_thresholds.end());
            control_param.max_votes = config::default_comm_max_votes;
        }
        
        tables::params params_table(_self, commun_code.raw());
        auto param = params_table.find(commun_code.raw());
        
        if (param == params_table.end()) {
            params_table.emplace(_self, [&](auto& item) {
                item.commun_code = commun_code;
                item.control = control_param;
            });
        }
        else {
            params_table.modify(param, eosio::same_payer, [&](auto& item) { item.control = control_param; });
        }
    }
}

/// control
void control::check_started(symbol_code commun_code) {
    get_control_param(control_param_contract, commun_code);
}

void control::on_transfer(name from, name to, asset quantity, string memo) {
//    if (!config(quantity.symbol.code()).exists())
//        return; // distribute only community token

// Commun TODO: process income funds
//    if (to == _self && quantity.amount > 0) {
//        // Don't check `from` for now, just distribute to top witnesses
//        auto total = quantity.amount;
//        auto top = top_witnesses();
//        auto n = top.size();
//        if (n == 0) {
//            print("nobody in top");
//            return;
//        }
//
//        auto token = quantity.symbol;
//        static const auto memo = "emission";
//        auto random = tapos_block_prefix();     // trx.ref_block_prefix; can generate hash from timestamp insead
//        auto winner = top[random % n];          // witness, who will receive fraction after reward division
//        auto reward = total / n;
//
//        vector<eosio::token::recipient> top_recipients;
//        for (const auto& w: top) {
//            if (w == winner)
//                continue;
//
//            if (reward <= 0)
//                continue;
//
//            top_recipients.push_back({w, asset(reward, token), memo});
//            total -= reward;
//        }
//        top_recipients.push_back({winner, asset(total, token), memo});
//
//        INLINE_ACTION_SENDER(token, bulkpayment)(config::token_name, {_self, config::code_name},
//                            {_self, top_recipients});
//    }
}

void control::regwitness(symbol_code commun_code, name witness, string url) {
    check_started(commun_code);
    eosio::check(url.length() <= config::witness_max_url_size, "url too long");
    require_auth(witness);

    upsert_tbl<witness_tbl>(_self, commun_code.raw(), witness, witness.value, [&](bool exists) {
        return [&,exists](witness_info& w) {
            eosio::check(!exists || w.url != url || !w.active, "already updated in the same way");
            w.name = witness;
            w.url = url;
            w.active = true;
        };
    });
}

// TODO: special action to free memory?
void control::unregwitness(symbol_code commun_code, name witness) {
    check_started(commun_code);
    require_auth(witness);

    witness_tbl witness_table(_self, commun_code.raw());
    auto it = witness_table.find(witness.value);
    eosio::check(!it->counter_votes, "not possible to remove witness as there are votes");
    witness_table.erase(*it);

    //TODO remove votes for witness
}

void control::stopwitness(symbol_code commun_code, name witness) {
    check_started(commun_code);
    require_auth(witness);
    active_witness(commun_code, witness, false);
}

void control::startwitness(symbol_code commun_code, name witness) {
    check_started(commun_code);
    require_auth(witness);
    active_witness(commun_code, witness, true);
}

// Note: if not weighted, it's possible to pass all witnesses in vector like in BP actions
void control::votewitness(symbol_code commun_code, name voter, name witness) {
    check_started(commun_code);
    require_auth(voter);

    witness_tbl witness_table(_self, commun_code.raw());
    auto witness_it = witness_table.find(witness.value);
    eosio::check(witness_it != witness_table.end(), "witness not found");
    eosio::check(witness_it->active, "witness not active");
    witness_table.modify(witness_it, eosio::same_payer, [&](auto& w) {
        ++w.counter_votes;
    });

    witness_vote_tbl tbl(_self, commun_code.raw());
    auto itr = tbl.find(voter.value);
    bool exists = itr != tbl.end();

    auto update = [&](auto& v) {
        v.voter = voter;
        v.witnesses.emplace_back(witness);
    };
    if (exists) {
        auto& w = itr->witnesses;
        auto el = std::find(w.begin(), w.end(), witness);
        eosio::check(el == w.end(), "already voted");
        eosio::check(w.size() < get_control_param(control_param_contract, commun_code).max_votes, "all allowed votes already casted");
        tbl.modify(itr, eosio::same_payer, update);
    } else {
        tbl.emplace(voter, update);
    }
    apply_vote_weight(commun_code, voter, witness, true);
}

void control::unvotewitn(symbol_code commun_code, name voter, name witness) {
    check_started(commun_code);
    require_auth(voter);

    witness_tbl witness_table(_self, commun_code.raw());
    auto witness_it = witness_table.find(witness.value);
    eosio::check(witness_it != witness_table.end(), "witness not found");
    witness_table.modify(witness_it, eosio::same_payer, [&](auto& w) {
        --w.counter_votes;
    });

    witness_vote_tbl tbl(_self, commun_code.raw());
    auto itr = tbl.find(voter.value);
    bool exists = itr != tbl.end();
    eosio::check(exists, "there are no votes");

    auto w = itr->witnesses;
    auto el = std::find(w.begin(), w.end(), witness);
    eosio::check(el != w.end(), "there is no vote for this witness");
    w.erase(el);
    tbl.modify(itr, eosio::same_payer, [&](auto& v) {
        v.witnesses = w;
    });
    apply_vote_weight(commun_code, voter, witness, false);
}

void control::changepoints(name who, asset diff) {
    symbol_code commun_code = diff.symbol.code();
    if (!(commun_code ? created(control_param_contract, commun_code) : initialized(control_param_contract))) {
        return;       // allow silent exit if changing vests before community created
    }
    require_auth(_self);
    eosio::check(diff.amount != 0, "diff is 0. something broken");          // in normal conditions sender must guarantee it
    change_voter_points(commun_code, who, diff.amount);
}


////////////////////////////////////////////////////////////////////////////////

void control::change_voter_points(symbol_code commun_code, name voter, share_type diff) {
    if (!diff) return;
    witness_vote_tbl tbl(_self, commun_code.raw());
    auto itr = tbl.find(voter.value);
    bool exists = itr != tbl.end();
    if (exists && itr->witnesses.size()) {
        update_witnesses_weights(commun_code, itr->witnesses, diff);
    }
}

void control::apply_vote_weight(symbol_code commun_code, name voter, name witness, bool add) {
    const auto power = commun_code ? 
        point::get_balance(config::point_name, voter, commun_code).amount : 
        point::get_assigned_reserve_amount(config::point_name, voter);
    if (power > 0) {
        update_witnesses_weights(commun_code, {witness}, add ? power : -power);
    }
}

void control::update_witnesses_weights(symbol_code commun_code, vector<name> witnesses, share_type diff) {
    witness_tbl wtbl(_self, commun_code.raw());
    for (const auto& witness : witnesses) {
        auto w = wtbl.find(witness.value);
        if (w != wtbl.end()) {
            wtbl.modify(w, eosio::same_payer, [&](auto& wi) {
                wi.total_weight += diff;            // TODO: additional checks of overflow? (not possible normally)
            });
            send_witness_event(commun_code, *w);
        } else {
            // just skip unregistered witnesses (incl. non existing accs) for now
            print("apply_vote_weight: witness not found\n");
        }
    }
}

void control::send_witness_event(symbol_code commun_code, const witness_info& wi) {
    eosio::event(_self, "witnessstate"_n, std::make_tuple(commun_code, wi.name, wi.total_weight, wi.active)).send();
}

void control::active_witness(symbol_code commun_code, name witness, bool flag) {
    // TODO: simplify upsert to allow passing just inner lambda
    bool exists = upsert_tbl<witness_tbl>(_self, commun_code.raw(), _self, witness.value, [&](bool) {
        return [&](witness_info& w) {
            eosio::check(flag != w.active, "active flag not updated");
            w.active = flag;

            send_witness_event(commun_code, w);
        };
    }, false);
    eosio::check(exists, "witness not found");
}

vector<witness_info> control::top_witness_info(symbol_code commun_code) {
    vector<witness_info> top;
    const auto l = get_control_param(control_param_contract, commun_code).leaders_num;
    top.reserve(l);
    witness_tbl witness(_self, commun_code.raw());
    auto idx = witness.get_index<"byweight"_n>();    // this index ordered descending
    for (auto itr = idx.begin(); itr != idx.end() && top.size() < l; ++itr) {
        if (itr->active && itr->total_weight > 0)
            top.emplace_back(*itr);
    }
    return top;
}

vector<name> control::top_witnesses(symbol_code commun_code) {
    const auto& wi = top_witness_info(commun_code);
    vector<name> top(wi.size());
    std::transform(wi.begin(), wi.end(), top.begin(), [](auto& w) {
        return w.name;
    });
    return top;
}

//multisig:
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
    eosio::check(in_the_top(_self, _commun_code, _proposer), _proposer.to_string() + " is not a leader");
    
    auto governance = _commun_code ? point::get_issuer(config::point_name, _commun_code) : config::dapp_name;
    
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
    
    eosio::check(in_the_top(_self, prop.commun_code, approver), approver.to_string() + " is not a leader");

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
    
    auto governance = prop.commun_code ? point::get_issuer(config::point_name, prop.commun_code) : config::dapp_name;
    
    auto packed_requested = pack(std::vector<permission_level>{{governance, prop.permission}});
    auto res = eosio::check_transaction_authorization(prop.packed_transaction.data(), prop.packed_transaction.size(),
                                             (const char*)0, 0,
                                             packed_requested.data(), packed_requested.size());
    eosio::check(res > 0, "transaction authorization is no longer satisfied");
    
    auto thresholds = get_control_param(control_param_contract, prop.commun_code).thresholds;
    auto threshold = std::find_if(thresholds.begin(), thresholds.end(), 
        [&](const structures::threshold& t) { return t.permission == prop.permission; });
    eosio::check(threshold != thresholds.end(), "unknown permission");

    auto top = top_witnesses(prop.commun_code);
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
                if (approvals_num >= threshold->required) {
                    break;
                }
            }
        }
    }
    eosio::check(approvals_num >= threshold->required, "transaction authorization failed");
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

} // commun

DISPATCH_WITH_TRANSFER(commun::control, commun::config::token_name, on_transfer,
    (regwitness)(unregwitness)
    (startwitness)(stopwitness)
    (votewitness)(unvotewitn)(changepoints)
    (propose)(approve)(unapprove)(cancel)(exec)(invalidate)
    (setparams)
    )
