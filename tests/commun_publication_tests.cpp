#include "gallery_tester.hpp"
#include "commun_posting_test_api.hpp"
#include "commun.point_test_api.hpp"
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

class commun_publication_tester : public gallery_tester {
protected:
    cyber_token_api token;
    commun_point_api point;
    commun_posting_api post;

    std::vector<account_name> _users;

public:
    commun_publication_tester()
        : gallery_tester(cfg::publish_name)
        , token({this, cfg::token_name, cfg::reserve_token})
        , point({this, cfg::commun_point_name, _point})
        , post({this, cfg::publish_name, symbol(0, point_code_str).to_symbol_code()})
        , _users{N(jackiechan), N(brucelee), N(chucknorris), N(alice)} {
        create_accounts(_users);
        create_accounts({_code, _commun, _golos, cfg::token_name, cfg::commun_point_name, cfg::commun_emit_name});
        produce_block();
        install_contract(cfg::commun_point_name, contracts::point_wasm(), contracts::point_abi());
        install_contract(cfg::commun_emit_name, contracts::emit_wasm(), contracts::emit_abi());
        install_contract(cfg::token_name, contracts::token_wasm(), contracts::token_abi());
        install_contract(cfg::publish_name, contracts::commun_publication_wasm(), contracts::commun_publication_abi());

        set_authority(cfg::commun_emit_name, cfg::reward_perm_name, create_code_authority({_code}), "active");
        link_authority(cfg::commun_emit_name, cfg::commun_emit_name, cfg::reward_perm_name, N(issuereward));

        std::vector<account_name> transfer_perm_accs{_code, cfg::commun_emit_name};
        std::sort(transfer_perm_accs.begin(), transfer_perm_accs.end());
        set_authority(cfg::commun_point_name, cfg::issue_permission, create_code_authority({cfg::commun_emit_name}), "active");
        set_authority(cfg::commun_point_name, cfg::transfer_permission, create_code_authority(transfer_perm_accs), "active");

        link_authority(cfg::commun_point_name, cfg::commun_point_name, cfg::issue_permission, N(issue));
        link_authority(cfg::commun_point_name, cfg::commun_point_name, cfg::transfer_permission, N(transfer));
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
        BOOST_CHECK_EQUAL(success(), point.setfreezer(commun::config::commun_gallery_name));

        BOOST_CHECK_EQUAL(success(), push_action(cfg::commun_emit_name, N(create), cfg::commun_emit_name, mvo()
            ("commun_symbol", point._symbol)("annual_emission_rate", annual_emission_rate)("leaders_reward_prop", leaders_reward_prop)));

        BOOST_CHECK_EQUAL(success(), token.transfer(_golos, cfg::commun_point_name, asset(reserve, token._symbol), cfg::restock_prefix + point_code_str));
        BOOST_CHECK_EQUAL(success(), point.issue(_golos, _golos, asset(supply, point._symbol), std::string(point_code_str) + " issue"));
        BOOST_CHECK_EQUAL(success(), point.open(_code, point._symbol, _code));

        BOOST_CHECK_EQUAL(success(), post.init_default_params());

        produce_block();
        for (auto& u : _users) {
            BOOST_CHECK_EQUAL(success(), point.open(u, point._symbol, u));
            BOOST_CHECK_EQUAL(success(), point.transfer(_golos, u, asset(supply / _users.size(), point._symbol)));
        }
    }

    const account_name _commun = N(commun);
    const account_name _golos = N(golos);
    int64_t supply;
    int64_t reserve;

    struct errors: contract_error_messages {
        const string no_param              = amsg("param does not exists");
        const string delete_children       = amsg("comment with child comments can't be deleted during the active period");
        const string no_permlink           = amsg("Permlink doesn't exist.");
        const string no_mosaic             = amsg("mosaic doesn't exist");
        const string update_no_message     = amsg("You can't update this message, because this message doesn't exist.");
        const string max_comment_depth     = amsg("publication::create_message: level > MAX_COMMENT_DEPTH");
        const string max_cmmnt_dpth_less_0 = amsg("Max comment depth must be greater than 0.");
        const string msg_exists            = amsg("This message already exists.");
        const string parent_no_message     = amsg("Parent message doesn't exist");

        const string wrong_prmlnk_length   = amsg("Permlink length is empty or more than 256.");
        const string wrong_prmlnk          = amsg("Permlink contains wrong symbol.");
        const string wrong_title_length    = amsg("Title length is more than 256.");
        const string wrong_body_length     = amsg("Body is empty.");
        const string wrong_curators_prcnt  = amsg("curators_prcnt can't be more than 100%.");

        const string vote_weight_0         = amsg("weight can't be 0.");
        const string vote_weight_gt100     = amsg("weight can't be more than 100%.");

        const string no_reblog_mssg           = amsg("You can't reblog, because this message doesn't exist.");
        const string own_reblog               = amsg("You cannot reblog your own content.");
        const string wrong_reblog_body_length = amsg("Body must be set if title is set.");
        const string own_reblog_erase         = amsg("You cannot erase reblog your own content.");
        const string no_reblog_mssg_erase     = amsg("You can't erase reblog, because this message doesn't exist.");
        const string gem_type_mismatch        = amsg("gem type mismatch");
        const string author_cannot_unvote     = amsg("author can't unvote");
        
    } err;
};

BOOST_AUTO_TEST_SUITE(commun_publication_tests)

BOOST_FIXTURE_TEST_CASE(set_params, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Params testing.");

    BOOST_CHECK_EQUAL(success(), point.create(_golos, asset(1000000000, point._symbol), 10000, 1));

    BOOST_CHECK_EQUAL(errgallery.no_balance, post.init_default_params());

    BOOST_CHECK_EQUAL(success(), point.open(_code, point._symbol, _code));
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
        {N(), "parentprmlnk"}, "headermssg", ""));

    BOOST_TEST_MESSAGE("--- checking wrong curators_prcnt.");
    BOOST_CHECK_EQUAL(err.wrong_curators_prcnt, post.create_msg({N(brucelee), "test-title"},
        {N(), "parentprmlnk"}, "headermssg", "body", "", {""}, "", cfg::_100percent+1));

    BOOST_TEST_MESSAGE("--- creating post.");
    BOOST_CHECK_EQUAL(success(), post.create_msg({N(brucelee), "permlink"}));
    produce_block();
    BOOST_TEST_MESSAGE("--- checking its mosaic.");
    auto mos = get_mosaic(_code, _point, N(brucelee), post.tracery("permlink"));
    BOOST_CHECK(!mos.is_null());

    BOOST_TEST_MESSAGE("--- testing hierarchy.");
    BOOST_CHECK_EQUAL(err.msg_exists, post.create_msg({N(brucelee), "permlink"}));

    BOOST_CHECK_EQUAL(err.parent_no_message, post.create_msg({N(jackiechan), "child"}, {N(notexist), "parent"}));
    BOOST_CHECK_EQUAL(success(), post.create_msg({N(jackiechan), "child"}, {N(brucelee), "permlink"}));
    BOOST_CHECK(!get_mosaic(_code, _point, N(jackiechan), post.tracery("child")).is_null());

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
        "headermssgnew", "bodymssgnew", "languagemssgnew", {{"tagnew"}}, "jsonmetadatanew"));
    BOOST_CHECK_EQUAL(err.update_no_message, post.update_msg({N(brucelee), "notexist"},
        "headermssgnew", "bodymssgnew", "languagemssgnew", {{"tagnew"}}, "jsonmetadatanew"));
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
    BOOST_CHECK_EQUAL(success(), post.delete_msg({N(brucelee), "permlink"}));
    BOOST_CHECK(get_mosaic(_code, _point, N(brucelee), post.tracery("permlink")).is_null());
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(reblog_message, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Reblog message testing.");

    init();
    std::string str256(256, 'a');

    BOOST_CHECK_EQUAL(success(), post.create_msg({N(brucelee), "permlink"}));

    BOOST_CHECK_EQUAL(err.own_reblog, post.reblog_msg(N(brucelee), {N(brucelee), "permlink"},
        "headermssg",
        "bodymssg"));
    BOOST_CHECK_EQUAL(err.wrong_title_length, post.reblog_msg(N(chucknorris), {N(brucelee), "permlink"},
        str256,
        "bodymssg"));
    BOOST_CHECK_EQUAL(err.wrong_reblog_body_length, post.reblog_msg(N(chucknorris), {N(brucelee), "permlink"},
        "headermssg",
        ""));
    BOOST_CHECK_EQUAL(err.no_reblog_mssg, post.reblog_msg(N(chucknorris), {N(brucelee), "test"},
        "headermssg",
        "bodymssg"));
    BOOST_CHECK_EQUAL(success(), post.reblog_msg(N(chucknorris), {N(brucelee), "permlink"},
        "headermssg",
        "bodymssg"));
    BOOST_CHECK_EQUAL(success(), post.reblog_msg(N(jackiechan), {N(brucelee), "permlink"},
        "",
        ""));
    BOOST_CHECK_EQUAL(success(), post.reblog_msg(_code, {N(brucelee), "permlink"},
        "",
        "bodymssg"));

    BOOST_CHECK_EQUAL(err.own_reblog_erase, post.erase_reblog_msg(N(brucelee),
        {N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(err.no_reblog_mssg_erase, post.erase_reblog_msg(N(chucknorris),
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
    BOOST_CHECK_EQUAL(err.vote_weight_0, vote_brucelee(0));
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
    BOOST_CHECK_EQUAL(err.vote_weight_0, vote_brucelee(0));
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

BOOST_FIXTURE_TEST_CASE(setfrequency, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("setfrequency testing.");
    BOOST_CHECK_EQUAL(err.no_param, post.setfrequency(N(brucelee), 10));
    init();
    BOOST_CHECK_EQUAL(success(), post.setfrequency(N(brucelee), 10));
    BOOST_CHECK(!post.get_accparam(N(brucelee)).is_null());
    BOOST_CHECK_EQUAL(post.get_accparam(N(brucelee))["actions_per_day"].as<uint16_t>(), 10);
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
