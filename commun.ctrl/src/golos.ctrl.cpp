#include "golos.ctrl/golos.ctrl.hpp"
#include "golos.ctrl/config.hpp"
#include <golos.vesting/golos.vesting.hpp>
#include <common/parameter_ops.hpp>
#include <common/dispatchers.hpp>
#include <cyber.bios/cyber.bios.hpp>
#include <eosio/transaction.hpp>
#include <eosio/event.hpp>

namespace golos {


using namespace eosio;
using std::vector;
using std::string;


namespace param {
constexpr uint16_t calc_thrs(uint16_t val, uint16_t top, uint16_t num, uint16_t denom) {
    return 0 == val ? uint32_t(top) * num / denom + 1 : val;
}
uint16_t msig_permissions::super_majority_threshold(uint16_t top) const { return calc_thrs(super_majority, top, 2, 3); };
uint16_t msig_permissions::majority_threshold(uint16_t top) const { return calc_thrs(majority, top, 1, 2); };
uint16_t msig_permissions::minority_threshold(uint16_t top) const { return calc_thrs(minority, top, 1, 3); };
}


////////////////////////////////////////////////////////////////
/// control
void control::assert_started() {
    eosio::check(_cfg.exists(), "not initialized");
}

struct ctrl_params_setter: set_params_visitor<ctrl_state> {
    using set_params_visitor::set_params_visitor;

    bool recheck_msig_perms = false;

    bool operator()(const ctrl_token_param& p) {
        return set_param(p, &ctrl_state::token);
    }
    bool operator()(const multisig_acc_param& p) {
        // TODO: if change multisig account, then must set auths
        return set_param(p, &ctrl_state::multisig);
    }
    bool operator()(const max_witnesses_param& p) {
        // TODO: if change max_witnesses, then must set auths
        bool changed = set_param(p, &ctrl_state::witnesses);
        if (changed) {
            // force re-check multisig_permissions, coz they can become invalid
            recheck_msig_perms = true;
        }
        return changed;
    }

    void check_msig_perms(const msig_perms_param& p) {
        const auto max = state.witnesses.max;
        const auto smaj = p.super_majority_threshold(max);
        const auto maj = p.majority_threshold(max);
        const auto min = p.minority_threshold(max);
        eosio::check(smaj <= max, "super_majority must not be greater than max_witnesses");
        eosio::check(maj <= max, "majority must not be greater than max_witnesses");
        eosio::check(min <= max, "minority must not be greater than max_witnesses");
        eosio::check(maj <= smaj, "majority must not be greater than super_majority");
        eosio::check(min <= smaj, "minority must not be greater than super_majority");
        eosio::check(min <= maj, "minority must not be greater than majority");
    }
    bool operator()(const msig_perms_param& p) {
        bool changed = set_param(p, &ctrl_state::msig_perms);
        if (changed) {
            // additionals checks against max_witnesses, which is not accessible in `validate()`
            check_msig_perms(p);
            recheck_msig_perms = false;     // don't re-check if parameter exists, variant order guarantees it checked after max_witnesses
        }
        return changed;
    }
    bool operator()(const witness_votes_param& p) {
        return set_param(p, &ctrl_state::witness_votes);
    }
    bool operator()(const update_auth_param& p) {
        return set_param(p, &ctrl_state::update_auth_period);
    }
};

void control::validateprms(vector<ctrl_param> params) {
    param_helper::check_params(params, _cfg.exists());
}

void control::setparams(vector<ctrl_param> params) {
    require_auth(_self);
    auto setter = param_helper::set_parameters<ctrl_params_setter>(params, _cfg, _self);
    if (setter.recheck_msig_perms) {
        setter.check_msig_perms(setter.state.msig_perms);
    }
    // TODO: auto-change auths on params change
}

void control::on_transfer(name from, name to, asset quantity, string memo) {
    if (!_cfg.exists() || quantity.symbol.code() != props().token.code)
            return; // distribute only community token

    if (to == _self && quantity.amount > 0) {
        // Don't check `from` for now, just distribute to top witnesses
        auto total = quantity.amount;
        auto top = top_witnesses();
        auto n = top.size();
        if (n == 0) {
            print("nobody in top");
            return;
        }

        auto token = quantity.symbol;
        static const auto memo = "emission";
        auto random = tapos_block_prefix();     // trx.ref_block_prefix; can generate hash from timestamp insead
        auto winner = top[random % n];          // witness, who will receive fraction after reward division
        auto reward = total / n;

        vector<eosio::token::recipient> top_recipients;
        for (const auto& w: top) {
            if (w == winner)
                continue;

            if (reward <= 0)
                continue;

            top_recipients.push_back({w, asset(reward, token), memo});
            total -= reward;
        }
        top_recipients.push_back({winner, asset(total, token), memo});

        INLINE_ACTION_SENDER(token, bulkpayment)(config::token_name, {_self, config::code_name},
                            {_self, top_recipients});
    }
}

void control::regwitness(name witness, string url) {
    assert_started();
    eosio::check(url.length() <= config::witness_max_url_size, "url too long");
    require_auth(witness);

    upsert_tbl<witness_tbl>(witness, [&](bool exists) {
        return [&,exists](witness_info& w) {
            eosio::check(!exists || w.url != url || !w.active, "already updated in the same way");
            w.name = witness;
            w.url = url;
            w.active = true;
        };
    });

    update_auths();
}

// TODO: special action to free memory?
void control::unregwitness(name witness) {
    assert_started();
    require_auth(witness);

    witness_tbl witness_table(_self, _self.value);
    auto it = witness_table.find(witness.value);
    eosio::check(!it->counter_votes, "not possible to remove witness as there are votes");
    witness_table.erase(*it);

    //TODO remove votes for witness
    update_auths();
}

void control::stopwitness(name witness) {
    assert_started();
    require_auth(witness);
    active_witness(witness, false);
}

void control::startwitness(name witness) {
    assert_started();
    require_auth(witness);
    active_witness(witness, true);
}

// Note: if not weighted, it's possible to pass all witnesses in vector like in BP actions
void control::votewitness(name voter, name witness) {
    assert_started();
    require_auth(voter);

    witness_tbl witness_table(_self, _self.value);
    auto witness_it = witness_table.find(witness.value);
    eosio::check(witness_it != witness_table.end(), "witness not found");
    eosio::check(witness_it->active, "witness not active");
    witness_table.modify(witness_it, eosio::same_payer, [&](auto& w) {
        ++w.counter_votes;
    });

    witness_vote_tbl tbl(_self, _self.value);
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
        eosio::check(w.size() < props().witness_votes.max, "all allowed votes already casted");
        tbl.modify(itr, eosio::same_payer, update);
    } else {
        tbl.emplace(voter, update);
    }
    apply_vote_weight(voter, witness, true);
}

void control::unvotewitn(name voter, name witness) {
    assert_started();
    require_auth(voter);

    witness_tbl witness_table(_self, _self.value);
    auto witness_it = witness_table.find(witness.value);
    eosio::check(witness_it != witness_table.end(), "witness not found");
    witness_table.modify(witness_it, eosio::same_payer, [&](auto& w) {
        --w.counter_votes;
    });

    witness_vote_tbl tbl(_self, _self.value);
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
    apply_vote_weight(voter, witness, false);
}

void control::changevest(name who, asset diff) {
    if (!_cfg.exists()) return;       // allow silent exit if changing vests before community created
    require_auth(token::get_issuer(config::token_name, diff.symbol.code()));
    eosio::check(diff.amount != 0, "diff is 0. something broken");          // in normal conditions sender must guarantee it
    eosio::check(diff.symbol.code() == props().token.code, "wrong symbol. something broken");  // in normal conditions sender must guarantee it
    change_voter_vests(who, diff.amount);
}


////////////////////////////////////////////////////////////////////////////////

void control::change_voter_vests(name voter, share_type diff) {
    if (!diff) return;
    witness_vote_tbl tbl(_self, _self.value);
    auto itr = tbl.find(voter.value);
    bool exists = itr != tbl.end();
    if (exists && itr->witnesses.size()) {
        update_witnesses_weights(itr->witnesses, diff);
    }
}

void control::apply_vote_weight(name voter, name witness, bool add) {
    const auto power = vesting::get_account_vesting(config::vesting_name, voter, props().token.code).amount;
    if (power > 0) {
        update_witnesses_weights({witness}, add ? power : -power);
    }
}

void control::update_witnesses_weights(vector<name> witnesses, share_type diff) {
    witness_tbl wtbl(_self, _self.value);
    for (const auto& witness : witnesses) {
        auto w = wtbl.find(witness.value);
        if (w != wtbl.end()) {
            wtbl.modify(w, eosio::same_payer, [&](auto& wi) {
                wi.total_weight += diff;            // TODO: additional checks of overflow? (not possible normally)
            });
            send_witness_event(*w);
        } else {
            // just skip unregistered witnesses (incl. non existing accs) for now
            print("apply_vote_weight: witness not found\n");
        }
    }

    update_auths();
}

void control::update_auths() {
    msig_auth_singleton tbl(_self, _self.value);
    const auto& top_auths = tbl.get_or_default();

    auto now = eosio::current_time_point();
    if (top_auths.last_update + eosio::seconds(props().update_auth_period.period) > now)
        return;

    auto set_last_update = [&]() {
        tbl.set({top_auths.witnesses, now}, _self);
    };

    auto top = top_witnesses();
    std::sort(top.begin(), top.end(), [](const auto& it1, const auto& it2) {
        return it1.value < it2.value;
    });

    const auto& old_top = top_auths.witnesses;
    if (old_top.size() > 0 && old_top.size() == top.size()) {
        bool result = std::equal(old_top.begin(), old_top.end(), top.begin(), [](const auto& prev, const auto& cur) {
            return prev.value == cur.value;
        });
        if (result) {
            set_last_update();
            return;
        }
    }

    if (top.size()) {
        tbl.set({top, now}, _self);
    } else {
        set_last_update();
    }

    auto& max_witn = props().witnesses.max;
    if (top.size() < max_witn) {           // TODO: ?restrict only just after creation and allow later
        print("Not enough witnesses to change auth\n");
        return;
    }
    cyber::authority auth;
    for (const auto& i : top) {
        auth.accounts.push_back({{i,config::active_name},1});
    }

    auto& thrs = props().msig_perms;
    vector<std::pair<name,uint16_t>> auths = {
        {config::minority_name, thrs.minority_threshold(max_witn)},
        {config::majority_name, thrs.majority_threshold(max_witn)},
        {config::super_majority_name, thrs.super_majority_threshold(max_witn)}
    };

    const auto& owner = props().multisig.name;
    for (const auto& [perm, thrs]: auths) {
        //permissions must be sorted
        std::sort(auth.accounts.begin(), auth.accounts.end(),
            [](const cyber::permission_level_weight& l, const cyber::permission_level_weight& r) {
                return std::tie(l.permission.actor, l.permission.permission) <
                    std::tie(r.permission.actor, r.permission.permission);
            }
        );

        auth.threshold = thrs;
        action(
            permission_level{owner, config::active_name},
            config::internal_name, "updateauth"_n,
            std::make_tuple(owner, perm, config::active_name, auth)
        ).send();
    }
}

void control::send_witness_event(const witness_info& wi) {
    eosio::event(_self, "witnessstate"_n, std::make_tuple(wi.name, wi.total_weight, wi.active)).send();
}

void control::active_witness(name witness, bool flag) {
    // TODO: simplify upsert to allow passing just inner lambda
    bool exists = upsert_tbl<witness_tbl>(witness, [&](bool) {
        return [&](witness_info& w) {
            eosio::check(flag != w.active, "active flag not updated");
            w.active = flag;

            send_witness_event(w);
        };
    }, false);
    eosio::check(exists, "witness not found");

    update_auths();
}

vector<witness_info> control::top_witness_info() {
    vector<witness_info> top;
    const auto l = props().witnesses.max;
    top.reserve(l);
    witness_tbl witness(_self, _self.value);
    auto idx = witness.get_index<"byweight"_n>();    // this index ordered descending
    for (auto itr = idx.begin(); itr != idx.end() && top.size() < l; ++itr) {
        if (itr->active && itr->total_weight > 0)
            top.emplace_back(*itr);
    }
    return top;
}

vector<name> control::top_witnesses() {
    const auto& wi = top_witness_info();
    vector<name> top(wi.size());
    std::transform(wi.begin(), wi.end(), top.begin(), [](auto& w) {
        return w.name;
    });
    return top;
}

} // golos

DISPATCH_WITH_TRANSFER(golos::control, on_transfer,
    (validateprms)(setparams)
    (regwitness)(unregwitness)
    (startwitness)(stopwitness)
    (votewitness)(unvotewitn)(changevest))
