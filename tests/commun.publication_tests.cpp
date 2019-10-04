#include "gallery_tester.hpp"
#include "commun.posting_test_api.hpp"
#include "commun.point_test_api.hpp"
#include "commun.emit_test_api.hpp"
#include "commun.list_test_api.hpp"
#include "cyber.token_test_api.hpp"
#include "../commun.point/include/commun.point/config.hpp"
#include "../commun.emit/include/commun.emit/config.hpp"
#include "../commun.gallery/include/commun.gallery/config.hpp"
#include "contracts.hpp"
#include <commun/config.hpp>

namespace cfg = commun::config;
using namespace eosio::testing;
using namespace eosio::chain;
using namespace fc;
static const auto point_code_str = "GLS";
static const auto _point = symbol(3, point_code_str);
static const auto point_code = _point.to_symbol_code();

const account_name _commun = cfg::dapp_name;
const account_name _golos = N(golos);

class commun_publication_tester : public gallery_tester {
protected:
    cyber_token_api token;
    commun_point_api point;
    commun_ctrl_api ctrl;
    commun_emit_api emit;
    commun_list_api list;
    commun_posting_api post;

    std::vector<account_name> _users;

public:
    commun_publication_tester()
        : gallery_tester(cfg::publish_name)
        , token({this, cfg::token_name, cfg::reserve_token})
        , point({this, cfg::point_name, _point})
        , ctrl({this, cfg::control_name, _point.to_symbol_code(), _golos})
        , emit({this, cfg::emit_name})
        , list({this, cfg::list_name})
        , post({this, cfg::publish_name, symbol(0, point_code_str).to_symbol_code()})
        , _users{N(jackiechan), N(brucelee), N(chucknorris), N(alice)} {
        create_accounts(_users);
        create_accounts({_code, _commun, _golos, cfg::token_name, cfg::point_name, cfg::list_name, cfg::emit_name, cfg::control_name});
        produce_block();
        install_contract(cfg::control_name, contracts::ctrl_wasm(), contracts::ctrl_abi());
        install_contract(cfg::point_name, contracts::point_wasm(), contracts::point_abi());
        install_contract(cfg::emit_name, contracts::emit_wasm(), contracts::emit_abi());
        install_contract(cfg::token_name, contracts::token_wasm(), contracts::token_abi());
        install_contract(cfg::list_name, contracts::list_wasm(), contracts::list_abi());
        install_contract(cfg::publish_name, contracts::publication_wasm(), contracts::publication_abi());

        set_authority(cfg::emit_name, cfg::reward_perm_name, create_code_authority({_code}), "active");
        link_authority(cfg::emit_name, cfg::emit_name, cfg::reward_perm_name, N(issuereward));

        std::vector<account_name> transfer_perm_accs{_code, cfg::emit_name};
        std::sort(transfer_perm_accs.begin(), transfer_perm_accs.end());
        set_authority(cfg::point_name, cfg::issue_permission, create_code_authority({cfg::emit_name}), "active");
        set_authority(cfg::point_name, cfg::transfer_permission, create_code_authority(transfer_perm_accs), "active");
        set_authority(cfg::control_name, N(changepoints), create_code_authority({cfg::point_name}), "active");

        link_authority(cfg::point_name, cfg::point_name, cfg::issue_permission, N(issue));
        link_authority(cfg::point_name, cfg::point_name, cfg::transfer_permission, N(transfer));
        link_authority(cfg::control_name, cfg::control_name, N(changepoints), N(changepoints));
    }

    void init() {
        supply  = 5000000000000;
        reserve = 50000000000;
        uint16_t royalty = 2500;

        uint16_t annual_emission_rate = 1000;
        uint16_t leaders_reward_prop = 1000;

        BOOST_CHECK_EQUAL(success(), token.create(_commun, asset(reserve, token._symbol)));
        BOOST_CHECK_EQUAL(success(), token.issue(_commun, _golos, asset(reserve, token._symbol), ""));

        BOOST_CHECK_EQUAL(success(), point.create(_golos, asset(supply * 2, point._symbol), 10000, 1));
        BOOST_CHECK_EQUAL(success(), point.setfreezer(commun::config::gallery_name));
        BOOST_CHECK_EQUAL(success(), list.create_record(cfg::list_name, point_code, "community 1"));

        BOOST_CHECK_EQUAL(success(), emit.create(point._symbol.to_symbol_code(), annual_emission_rate, leaders_reward_prop));

        BOOST_CHECK_EQUAL(success(), token.transfer(_golos, cfg::point_name, asset(reserve, token._symbol), cfg::restock_prefix + point_code_str));
        BOOST_CHECK_EQUAL(success(), point.issue(_golos, _golos, asset(supply, point._symbol), std::string(point_code_str) + " issue"));
        BOOST_CHECK_EQUAL(success(), point.open(_code, point_code, _code));

        BOOST_CHECK_EQUAL(success(), post.init_default_params());

        produce_block();
        for (auto& u : _users) {
            BOOST_CHECK_EQUAL(success(), point.open(u, point_code, u));
            BOOST_CHECK_EQUAL(success(), point.transfer(_golos, u, asset(supply / _users.size(), point._symbol)));
        }
    }
    int64_t supply;
    int64_t reserve;

    struct errors: contract_error_messages {
        const string no_param              = amsg("param does not exists");
        const string delete_children       = amsg("comment with child comments can't be deleted during the active period");
        const string no_permlink           = amsg("Permlink doesn't exist.");
        const string no_mosaic             = amsg("mosaic doesn't exist");
        const string max_comment_depth     = amsg("publication::createmssg: level > MAX_COMMENT_DEPTH");
        const string max_cmmnt_dpth_less_0 = amsg("Max comment depth must be greater than 0.");
        const string msg_exists            = amsg("This message already exists.");
        const string parent_no_message     = amsg("Parent message doesn't exist");
        const string no_message            = amsg("Message does not exist.");

        const string wrong_prmlnk_length   = amsg("Permlink length is empty or more than 256.");
        const string wrong_prmlnk          = amsg("Permlink contains wrong symbol.");
        const string wrong_title_length    = amsg("Title length is more than 256.");
        const string wrong_body_length     = amsg("Body is empty.");
        const string wrong_curators_prcnt  = amsg("curators_prcnt can't be more than 100%.");
        const string tags_are_same         = amsg("No changes in tags.");
        const string reason_empty          = amsg("Reason cannot be empty.");

        const string vote_weight_0         = amsg("weight can't be 0.");
        const string vote_weight_gt100     = amsg("weight can't be more than 100%.");

        const string own_reblog               = amsg("You cannot reblog your own content.");
        const string wrong_reblog_body_length = amsg("Body must be set if title is set.");
        const string own_reblog_erase         = amsg("You cannot erase reblog your own content.");
        const string gem_type_mismatch        = amsg("gem type mismatch");
        const string author_cannot_unvote     = amsg("author can't unvote");
    } err;
};

BOOST_AUTO_TEST_SUITE(commun_publication_tests)

BOOST_FIXTURE_TEST_CASE(set_params, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Params testing.");

    BOOST_CHECK_EQUAL(success(), point.create(_golos, asset(1000000000, point._symbol), 10000, 1));

    BOOST_CHECK_EQUAL(errgallery.no_balance, post.init_default_params());

    BOOST_CHECK_EQUAL(success(), point.open(_code, point_code, _code));
    BOOST_CHECK_EQUAL(success(), post.init_default_params());
    produce_block();
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(create_message, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Create message testing.");
    init();
    std::string str256(256, 'a');

    BOOST_TEST_MESSAGE("--- checking permlink length.");
    BOOST_CHECK_EQUAL(err.wrong_prmlnk_length, post.create_msg({N(brucelee), ""}));
    BOOST_CHECK_EQUAL(err.wrong_prmlnk_length, post.create_msg({N(brucelee), str256}));

    BOOST_TEST_MESSAGE("--- checking permlink naming convension.");
    BOOST_CHECK_EQUAL(err.wrong_prmlnk, post.create_msg({N(brucelee), "ABC"}));
    BOOST_CHECK_EQUAL(err.wrong_prmlnk, post.create_msg({N(brucelee), "АБЦ"}));
    BOOST_CHECK_EQUAL(success(), post.create_msg({N(brucelee), "abcdefghijklmnopqrstuvwxyz0123456789-"}));

    BOOST_TEST_MESSAGE("--- checking with too long title and empty body.");
    BOOST_CHECK_EQUAL(err.wrong_title_length, post.create_msg({N(brucelee), "test-title"},
        {N(), "parentprmlnk"}, str256));
    BOOST_CHECK_EQUAL(err.wrong_body_length, post.create_msg({N(brucelee), "test-body"},
        {N(), "parentprmlnk"}, "header", ""));

    BOOST_TEST_MESSAGE("--- checking wrong curators_prcnt.");
    BOOST_CHECK_EQUAL(err.wrong_curators_prcnt, post.create_msg({N(brucelee), "test-title"},
        {N(), "parentprmlnk"}, "header", "body", {""}, "", cfg::_100percent+1));

    BOOST_TEST_MESSAGE("--- creating post.");
    BOOST_CHECK_EQUAL(success(), post.create_msg({N(brucelee), "permlink"}));
    produce_block();
    BOOST_TEST_MESSAGE("--- checking its vertex.");
    CHECK_MATCHING_OBJECT(post.get_vertex({N(brucelee), "permlink"}), mvo()
        ("parent_creator", "")
        ("parent_tracery", 0)
        ("level", 0)
        ("childcount", 0)
    );
    BOOST_TEST_MESSAGE("--- checking its mosaic.");
    auto mos = get_mosaic(_code, _point, N(brucelee), post.tracery("permlink"));
    BOOST_CHECK(!mos.is_null());

    BOOST_TEST_MESSAGE("--- testing hierarchy.");
    BOOST_CHECK_EQUAL(err.msg_exists, post.create_msg({N(brucelee), "permlink"}));

    BOOST_CHECK_EQUAL(err.parent_no_message, post.create_msg({N(jackiechan), "child"}, {N(notexist), "parent"}));
    BOOST_CHECK_EQUAL(success(), post.create_msg({N(jackiechan), "child"}, {N(brucelee), "permlink"}));
    BOOST_CHECK(!get_mosaic(_code, _point, N(jackiechan), post.tracery("child")).is_null());
    CHECK_MATCHING_OBJECT(post.get_vertex({N(jackiechan), "child"}), mvo()
        ("parent_creator", "brucelee")
        ("parent_tracery", post.tracery("permlink"))
        ("level", 1)
        ("childcount", 0)
    );
    CHECK_MATCHING_OBJECT(post.get_vertex({N(brucelee), "permlink"}), mvo()
        ("parent_creator", "")
        ("parent_tracery", 0)
        ("level", 0)
        ("childcount", 1)
    );
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(nesting_level_test, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("nesting level test.");
    init();
    BOOST_CHECK_EQUAL(success(), post.create_msg({N(brucelee), "permlink0"}));
    size_t i = 0;
    for (; i < post.max_comment_depth; i++) {
        BOOST_CHECK_EQUAL(success(), post.create_msg(
            {N(brucelee), "permlink" + std::to_string(i+1)},
            {N(brucelee), "permlink" + std::to_string(i)}));
    }
    BOOST_CHECK_EQUAL(err.max_comment_depth, post.create_msg(
        {N(brucelee), "permlink" + std::to_string(i+1)},
        {N(brucelee), "permlink" + std::to_string(i)}));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(update_message, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Update message testing.");
    init();
    BOOST_CHECK_EQUAL(success(), post.create_msg({N(brucelee), "permlink"}));

    BOOST_CHECK_EQUAL(success(), post.update_msg({N(brucelee), "permlink"},
        "headernew", "bodynew", {{"tagnew"}}, "metadatanew"));
    BOOST_CHECK_EQUAL(err.no_message, post.update_msg({N(brucelee), "notexist"},
        "headernew", "bodynew", {{"tagnew"}}, "metadatanew"));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(settags_to_message, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Set message tags by leader testing.");
    init();
    ctrl.prepare({N(jackiechan)}, N(brucelee));
    mssgid msg = {N(brucelee), "permlink"};
    BOOST_CHECK_EQUAL(success(), post.create_msg(msg));

    BOOST_CHECK_EQUAL(errgallery.not_a_leader(N(brucelee)), post.settags(N(brucelee), {N(brucelee), "permlink"}, {"newtag"}, {"oldtag"}, "the reason"));

    BOOST_CHECK_EQUAL(err.no_message, post.settags(N(jackiechan), {N(brucelee), "notexist"}, {"newtag"}, {"oldtag"}, "the reason"));

    BOOST_CHECK_EQUAL(success(), post.settags(N(jackiechan), msg, {"newtag"}, {"oldtag"}, "the reason"));
    BOOST_CHECK_EQUAL(success(), post.settags(N(jackiechan), msg, {"newtag"}, {"oldtag"}, ""));
    BOOST_CHECK_EQUAL(success(), post.settags(N(jackiechan), msg, {}, {"oldtag"}, "the reason"));
    BOOST_CHECK_EQUAL(success(), post.settags(N(jackiechan), msg, {"newtag"}, {}, "the reason"));
    BOOST_CHECK_EQUAL(err.tags_are_same, post.settags(N(jackiechan), msg, {}, {}, "the reason"));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(delete_message, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Delete message testing.");
    init();
    BOOST_CHECK_EQUAL(success(), post.create_msg({N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(success(), post.create_msg({N(jackiechan), "child"}, {N(brucelee), "permlink"}));

    BOOST_TEST_MESSAGE("--- fail then delete non-existing post and post with child");
    BOOST_CHECK_EQUAL(err.no_mosaic, post.delete_msg({N(jackiechan), "permlink1"}));
    BOOST_CHECK_EQUAL(err.delete_children, post.delete_msg({N(brucelee), "permlink"}));

    BOOST_CHECK_EQUAL(success(), post.delete_msg({N(jackiechan), "child"}));
    BOOST_CHECK(get_mosaic(_code, _point, N(jackiechan), post.tracery("child")).is_null());
    BOOST_CHECK(post.get_vertex({N(jackiechan), "child"}).is_null());
    CHECK_MATCHING_OBJECT(post.get_vertex({N(brucelee), "permlink"}), mvo()
       ("childcount", 0)
    );

    BOOST_CHECK_EQUAL(success(), post.delete_msg({N(brucelee), "permlink"}));
    BOOST_CHECK(get_mosaic(_code, _point, N(brucelee), post.tracery("permlink")).is_null());
    BOOST_CHECK(post.get_vertex({N(brucelee), "permlink"}).is_null());
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(report_message, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Report message testing.");
    init();
    mssgid msg = {N(brucelee), "permlink"};
    BOOST_CHECK_EQUAL(success(), post.create_msg(msg));

    BOOST_CHECK_EQUAL(err.no_message, post.report_mssg(N(jackiechan), {N(brucelee), "notexist"}, "the reason"));

    BOOST_CHECK_EQUAL(success(), post.report_mssg(N(jackiechan), msg, "the reason"));
    BOOST_CHECK_EQUAL(err.reason_empty, post.report_mssg(N(jackiechan), msg, ""));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(reblog_message, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Reblog message testing.");

    init();
    std::string str256(256, 'a');

    BOOST_CHECK_EQUAL(success(), post.create_msg({N(brucelee), "permlink"}));

    BOOST_CHECK_EQUAL(err.own_reblog, post.reblog_msg(N(brucelee), {N(brucelee), "permlink"},
        "header",
        "body"));
    BOOST_CHECK_EQUAL(err.wrong_title_length, post.reblog_msg(N(chucknorris), {N(brucelee), "permlink"},
        str256,
        "body"));
    BOOST_CHECK_EQUAL(err.wrong_reblog_body_length, post.reblog_msg(N(chucknorris), {N(brucelee), "permlink"},
        "header",
        ""));
    BOOST_CHECK_EQUAL(err.no_message, post.reblog_msg(N(chucknorris), {N(brucelee), "test"},
        "header",
        "body"));
    BOOST_CHECK_EQUAL(success(), post.reblog_msg(N(chucknorris), {N(brucelee), "permlink"},
        "header",
        "body"));
    BOOST_CHECK_EQUAL(success(), post.reblog_msg(N(jackiechan), {N(brucelee), "permlink"},
        "",
        ""));
    BOOST_CHECK_EQUAL(success(), post.reblog_msg(_code, {N(brucelee), "permlink"},
        "",
        "body"));

    BOOST_CHECK_EQUAL(err.own_reblog_erase, post.erase_reblog_msg(N(brucelee),
        {N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(err.no_message, post.erase_reblog_msg(N(chucknorris),
        {N(brucelee), "notexist"}));
    BOOST_CHECK_EQUAL(success(), post.erase_reblog_msg(N(chucknorris),
        {N(brucelee), "permlink"}));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(upvote, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Upvote testing.");
    auto permlink = "permlink";
    auto vote_brucelee = [&](auto weight){ return post.upvote(N(brucelee), {N(brucelee), permlink}, weight); };
    BOOST_CHECK_EQUAL(err.no_param, vote_brucelee(1));
    init();
    BOOST_CHECK_EQUAL(errgallery.no_mosaic, vote_brucelee(1));
    BOOST_CHECK_EQUAL(success(), post.create_msg({N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(err.vote_weight_gt100, vote_brucelee(cfg::_100percent+1));
    BOOST_CHECK_EQUAL(success(), vote_brucelee(cfg::_100percent));
    auto gem = get_gem(_code, _point, 0, N(brucelee));
    BOOST_CHECK(!gem.is_null());
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(downvote, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Downvote testing.");
    auto permlink = "permlink";
    auto vote_brucelee = [&](auto weight){ return post.downvote(N(brucelee), {N(brucelee), permlink}, weight); };
    BOOST_CHECK_EQUAL(err.no_param, vote_brucelee(1));
    init();
    BOOST_CHECK_EQUAL(errgallery.no_mosaic, vote_brucelee(1));
    BOOST_CHECK_EQUAL(success(), post.create_msg({N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(err.vote_weight_gt100, vote_brucelee(cfg::_100percent+1));
    BOOST_CHECK_EQUAL(err.gem_type_mismatch, vote_brucelee(cfg::_100percent)); //brucelee cannot downvote for his own post
    BOOST_CHECK_EQUAL(success(), post.downvote(N(chucknorris), {N(brucelee), permlink}, cfg::_100percent));
    auto gem = get_gem(_code, _point, 0, N(chucknorris));
    BOOST_CHECK(!gem.is_null());
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(unvote, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Unvote testing.");
    init();
    BOOST_CHECK_EQUAL(errgallery.no_mosaic, post.unvote(N(chucknorris), {N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(success(), post.create_msg({N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(err.author_cannot_unvote, post.unvote(N(brucelee), {N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(errgallery.nothing_to_claim, post.unvote(N(chucknorris), {N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(success(), post.upvote(N(chucknorris), {N(brucelee), "permlink"}, 123));
    produce_block();
    BOOST_CHECK(!get_gem(_code, _point, 0, N(chucknorris)).is_null());
    BOOST_CHECK_EQUAL(success(), post.unvote(N(chucknorris), {N(brucelee), "permlink"}));
    BOOST_CHECK(get_gem(_code, _point, 0, N(chucknorris)).is_null());
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(setproviders, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("setproviders testing.");
    BOOST_CHECK_EQUAL(err.no_param, post.setproviders(N(brucelee), {N(chucknorris)}));
    init();
    BOOST_CHECK_EQUAL(success(), post.setproviders(N(brucelee), {N(chucknorris)}));
    BOOST_CHECK(!post.get_accparam(N(brucelee)).is_null());
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(set_gem_holders, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Set gem holders testing.");
    init();
    BOOST_CHECK_EQUAL(success(), post.create_msg({N(alice), "facelift"}));
    BOOST_CHECK_EQUAL(success(), post.create_msg({N(alice), "dirt"}));
    BOOST_CHECK_EQUAL(success(), post.create_msg({N(alice), "alice-in-blockchains"}));

    //_golos has no tokens to freeze
    BOOST_CHECK_EQUAL(errgallery.overdrawn_balance, post.transfer({N(alice), "dirt"}, N(alice), N(alice), _golos));

    BOOST_CHECK_EQUAL(success(), post.transfer({N(alice), "dirt"}, N(alice), N(alice), N(jackiechan)));
    BOOST_CHECK_EQUAL(success(), post.hold({N(alice), "alice-in-blockchains"}, N(alice)));

    produce_block();
    produce_block(fc::seconds(cfg::default_mosaic_active_period - cfg::block_interval_ms / 1000));

    //a third party can claim it because the active period has expired
    BOOST_CHECK_EQUAL(success(), post.claim({N(alice), "facelift"}, N(alice), N(alice), false, N(chucknorris)));

    BOOST_CHECK_EQUAL(errgallery.nothing_to_claim, post.claim({N(alice), "dirt"}, N(alice), N(alice), false, N(chucknorris)));
    BOOST_CHECK_EQUAL(errgallery.no_authority, post.claim({N(alice), "dirt"}, N(jackiechan), N(alice), false, N(alice)));
    BOOST_CHECK_EQUAL(success(), post.claim({N(alice), "dirt"}, N(jackiechan), N(alice), false, N(jackiechan)));

    BOOST_CHECK_EQUAL(errgallery.no_authority, post.claim({N(alice), "alice-in-blockchains"}, N(alice), N(alice), false, N(chucknorris)));
    BOOST_CHECK_EQUAL(success(), post.claim({N(alice), "alice-in-blockchains"}, N(alice), N(alice), false, N(alice)));

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(reward_for_downvote, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Reward for downvote testing.");
    init();
    uint16_t weight = 100;
    ctrl.prepare({N(jackiechan), N(brucelee)}, N(chucknorris));
    
    BOOST_CHECK_EQUAL(success(), post.create_msg({N(alice), "facelift"}, {N(), "p"}, "h", "b", {"t"}, "m", 5000, weight));
    BOOST_CHECK_EQUAL(success(), post.create_msg({N(alice), "dirt"}, {N(), "p"}, "h", "b", {"t"}, "m", 5000, weight));
    BOOST_CHECK_EQUAL(success(), post.create_msg({N(alice), "alice-in-blockchains"}, {N(), "p"}, "h", "b", {"t"}, "m", 5000, weight));

    BOOST_CHECK(!get_mosaic(_code, _point, N(alice), post.tracery("facelift"))["meritorious"].as<bool>());
    BOOST_CHECK(!get_mosaic(_code, _point, N(alice), post.tracery("dirt"))["meritorious"].as<bool>());
    BOOST_CHECK(!get_mosaic(_code, _point, N(alice), post.tracery("alice-in-blockchains"))["meritorious"].as<bool>());

    BOOST_CHECK_EQUAL(success(), post.slap(N(brucelee), {N(alice), "facelift"}));
    BOOST_CHECK_EQUAL(success(), post.slap(N(brucelee), {N(alice), "dirt"}));
    BOOST_CHECK_EQUAL(success(), post.slap(N(brucelee), {N(alice), "alice-in-blockchains"}));

    produce_block();
    produce_block(fc::seconds(cfg::reward_mosaics_period - (cfg::block_interval_ms / 1000)));

    //chucknorris will receive a reward as "facelift" will be in the top and will be banned (*1)
    BOOST_CHECK_EQUAL(success(), post.downvote(N(chucknorris), {N(alice), "facelift"}, weight - 1));

    BOOST_CHECK(get_mosaic(_code, _point, N(alice), post.tracery("facelift"))["meritorious"].as<bool>());
    BOOST_CHECK(get_mosaic(_code, _point, N(alice), post.tracery("dirt"))["meritorious"].as<bool>());
    BOOST_CHECK(get_mosaic(_code, _point, N(alice), post.tracery("alice-in-blockchains"))["meritorious"].as<bool>());

    //jackiechan will not receive a reward because "dirt" will not be in the top, although it will be banned (*2)
    BOOST_CHECK_EQUAL(success(), post.downvote(N(jackiechan), {N(alice), "dirt"}, weight));

    //brucelee will not receive a reward because "alice-in-blockchains" will not be banned (*3)
    BOOST_CHECK_EQUAL(success(), post.downvote(N(brucelee), {N(alice), "alice-in-blockchains"}, weight - 1));

    produce_block();
    produce_block(fc::seconds(cfg::reward_mosaics_period - (cfg::block_interval_ms / 1000)));

    BOOST_CHECK_EQUAL(success(), post.create_msg({N(brucelee), "what-are-you-waiting-for-jackie"}));
    BOOST_CHECK_EQUAL(success(), post.hold({N(brucelee), "what-are-you-waiting-for-jackie"}, N(brucelee)));

    BOOST_CHECK_EQUAL(errgallery.already_done, post.slap(N(brucelee), {N(alice), "facelift"}));

    BOOST_CHECK_EQUAL(success(), post.slap(N(jackiechan), {N(alice), "facelift"}));
    BOOST_CHECK_EQUAL(success(), post.slap(N(jackiechan), {N(alice), "dirt"}));

    BOOST_CHECK(get_mosaic(_code, _point, N(alice), post.tracery("facelift"))["banned"].as<bool>());
    BOOST_CHECK(get_mosaic(_code, _point, N(alice), post.tracery("dirt"))["banned"].as<bool>());
    BOOST_CHECK(!get_mosaic(_code, _point, N(alice), post.tracery("alice-in-blockchains"))["banned"].as<bool>());

    BOOST_CHECK(!get_mosaic(_code, _point, N(alice), post.tracery("facelift"))["meritorious"].as<bool>());
    BOOST_CHECK(!get_mosaic(_code, _point, N(alice), post.tracery("dirt"))["meritorious"].as<bool>());
    BOOST_CHECK(get_mosaic(_code, _point, N(alice), post.tracery("alice-in-blockchains"))["meritorious"].as<bool>());

    BOOST_CHECK_EQUAL(errgallery.mosaic_is_inactive, post.slap(N(brucelee), {N(alice), "facelift"}));
    BOOST_CHECK_EQUAL(errgallery.mosaic_banned, post.downvote(N(chucknorris), {N(alice), "dirt"}, weight));

    produce_block();
    produce_block(fc::seconds(cfg::default_evaluation_period - cfg::reward_mosaics_period));

    auto amount_alice0 = point.get_amount(N(alice));
    BOOST_CHECK_EQUAL(success(), post.claim({N(alice), "facelift"}, N(alice), N(alice), false, N(alice)));
    auto amount_alice1 = point.get_amount(N(alice));
    BOOST_CHECK_EQUAL(success(), post.claim({N(alice), "dirt"}, N(alice), N(alice), false, N(alice)));
    auto amount_alice2 = point.get_amount(N(alice));
    BOOST_CHECK_EQUAL(success(), post.claim({N(alice), "alice-in-blockchains"}, N(alice), N(alice), false, N(alice)));
    auto amount_alice3 = point.get_amount(N(alice));

    BOOST_CHECK(amount_alice0 == amount_alice1 && amount_alice1 == amount_alice2 && amount_alice2 < amount_alice3);

    auto amount_chuck0 = point.get_amount(N(chucknorris));
    BOOST_CHECK_EQUAL(success(), post.claim({N(alice), "facelift"}, N(chucknorris), N(chucknorris), false, N(chucknorris)));
    auto amount_chuck1 = point.get_amount(N(chucknorris));
    BOOST_CHECK(amount_chuck0 < amount_chuck1); // (1)

    auto amount_jackie0 = point.get_amount(N(jackiechan));
    BOOST_CHECK_EQUAL(success(), post.claim({N(alice), "dirt"}, N(jackiechan), N(jackiechan), false, N(jackiechan)));
    auto amount_jackie1 = point.get_amount(N(jackiechan));
    BOOST_CHECK(amount_jackie0 == amount_jackie1); // (2)

    auto amount_bruce0 = point.get_amount(N(brucelee));
    BOOST_CHECK_EQUAL(success(), post.claim({N(alice), "alice-in-blockchains"}, N(brucelee), N(brucelee), false, N(brucelee)));
    auto amount_bruce1 = point.get_amount(N(brucelee));
    BOOST_CHECK(amount_bruce0 == amount_bruce1); // (3)

    //at the end of this story, let's verify that jackiechan cannot slap the archive mosaic
    produce_block();
    produce_block(fc::seconds(cfg::default_mosaic_active_period - 
                             (cfg::default_evaluation_period - cfg::reward_mosaics_period + (cfg::block_interval_ms / 1000))));
    //curious case: first, the existence of the parent permlink is checked, 
    //then the parent mosaic is archived and the parent permlink is destroyed
    BOOST_CHECK_EQUAL(success(), post.create_msg({N(jackiechan), "what"}, {N(brucelee), "what-are-you-waiting-for-jackie"}));
    //therefore, jackie will not be able to create a second comment
    BOOST_CHECK_EQUAL(err.parent_no_message, post.create_msg({N(jackiechan), "hm"}, {N(brucelee), "what-are-you-waiting-for-jackie"}));

    BOOST_CHECK_EQUAL(errgallery.mosaic_is_inactive, post.slap(N(jackiechan), {N(brucelee), "what-are-you-waiting-for-jackie"}));

    BOOST_CHECK_EQUAL(success(), post.claim({N(brucelee), "what-are-you-waiting-for-jackie"}, N(brucelee)));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(changing_leaders_and_slaps, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Changing leaders and slaps testing.");
    init();
    ctrl.prepare({N(jackiechan), N(brucelee)}, N(chucknorris));
    BOOST_CHECK_EQUAL(success(), post.create_msg({N(alice), "alice-in-blockchains"}));

    BOOST_CHECK_EQUAL(success(), post.slap(N(jackiechan), {N(alice), "alice-in-blockchains"}));
    BOOST_CHECK_EQUAL(errgallery.not_a_leader(N(chucknorris)), post.slap(N(chucknorris), {N(alice), "alice-in-blockchains"}));

    BOOST_CHECK_EQUAL(success(), ctrl.unvote_leader(N(chucknorris), N(jackiechan)));
    BOOST_CHECK_EQUAL(success(), ctrl.reg_leader(N(chucknorris), "chucknorris"));
    BOOST_CHECK_EQUAL(success(), ctrl.vote_leader(N(chucknorris), N(chucknorris)));

    BOOST_CHECK_EQUAL(1,
        get_mosaic(_code, _point, N(alice), post.tracery("alice-in-blockchains"))["slaps"].as<std::vector<account_name> >().size());

    BOOST_CHECK_EQUAL(success(), post.slap(N(chucknorris), {N(alice), "alice-in-blockchains"}));

    BOOST_CHECK_EQUAL(1,
        get_mosaic(_code, _point, N(alice), post.tracery("alice-in-blockchains"))["slaps"].as<std::vector<account_name> >().size());

    BOOST_CHECK_EQUAL(success(), post.slap(N(brucelee), {N(alice), "alice-in-blockchains"}));

    BOOST_CHECK_EQUAL(2,
        get_mosaic(_code, _point, N(alice), post.tracery("alice-in-blockchains"))["slaps"].as<std::vector<account_name> >().size());
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
