#include "gallery_tester.hpp"
#include "commun.posting_test_api.hpp"
#include "commun.point_test_api.hpp"
#include "commun.emit_test_api.hpp"
#include "commun.list_test_api.hpp"
#include "cyber.token_test_api.hpp"
#include "../commun.publication/include/commun.publication/config.hpp"
#include "../commun.point/include/commun.point/config.hpp"
#include "../commun.emit/include/commun.emit/config.hpp"
#include "../commun.gallery/include/commun.gallery/config.hpp"
#include "../commun.list/include/commun.list/config.hpp"
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
const account_name _client = N(communcom);

class commun_publication_tester : public gallery_tester {
protected:
    cyber_token_api token;
    commun_point_api point;
    commun_ctrl_api ctrl;
    commun_emit_api emit;
    commun_list_api community;
    commun_posting_api post;

    std::vector<account_name> _users;

public:
    commun_publication_tester()
        : gallery_tester(cfg::publish_name)
        , token({this, cfg::token_name, cfg::reserve_token})
        , point({this, cfg::point_name, _point})
        , ctrl({this, cfg::control_name, _point.to_symbol_code(), _golos})
        , emit({this, cfg::emit_name})
        , community({this, cfg::list_name})
        , post({this, cfg::publish_name, symbol(0, point_code_str).to_symbol_code()})
        , _users{N(jackiechan), N(brucelee), N(chucknorris), N(alice)} {
        create_accounts(_users);
        create_accounts({_code, _commun, _golos, cfg::token_name, cfg::point_name, cfg::list_name, 
            cfg::emit_name, cfg::control_name, _client});
        produce_block();
        install_contract(cfg::control_name, contracts::ctrl_wasm(), contracts::ctrl_abi());
        install_contract(cfg::point_name, contracts::point_wasm(), contracts::point_abi());
        install_contract(cfg::emit_name, contracts::emit_wasm(), contracts::emit_abi());
        install_contract(cfg::token_name, contracts::token_wasm(), contracts::token_abi());
        install_contract(cfg::list_name, contracts::list_wasm(), contracts::list_abi());
        install_contract(cfg::publish_name, contracts::publication_wasm(), contracts::publication_abi());

        set_authority(cfg::emit_name, cfg::reward_perm_name, create_code_authority({_code}), "active");
        link_authority(cfg::emit_name, cfg::emit_name, cfg::reward_perm_name, N(issuereward));
        
        set_authority(cfg::emit_name, N(init), create_code_authority({cfg::list_name}), "active");
        link_authority(cfg::emit_name, cfg::emit_name, N(init), N(init));
        
        set_authority(cfg::control_name, N(init), create_code_authority({cfg::list_name}), "active");
        link_authority(cfg::control_name, cfg::control_name, N(init), N(init));

        set_authority(cfg::publish_name, N(init), create_code_authority({cfg::list_name}), "active");
        link_authority(cfg::publish_name, cfg::publish_name, N(init), N(init));

        std::vector<account_name> transfer_perm_accs{_code, cfg::emit_name};
        std::sort(transfer_perm_accs.begin(), transfer_perm_accs.end());
        set_authority(cfg::point_name, cfg::issue_permission, create_code_authority({cfg::emit_name}), "active");
        set_authority(cfg::gallery_name, cfg::transfer_permission, create_code_authority(transfer_perm_accs), "active");
        set_authority(cfg::control_name, N(changepoints), create_code_authority({cfg::point_name}), "active");

        link_authority(cfg::point_name, cfg::point_name, cfg::issue_permission, N(issue));
        link_authority(cfg::gallery_name, cfg::point_name, cfg::transfer_permission, N(transfer));
        link_authority(cfg::control_name, cfg::control_name, N(changepoints), N(changepoints));
        
        set_authority(cfg::publish_name, cfg::client_permission_name,
            authority (1, {}, {{.permission = {_client, cfg::active_name}, .weight = 1}}), "owner");
        link_authority(cfg::publish_name, cfg::publish_name, cfg::client_permission_name, N(upvote));
        link_authority(cfg::publish_name, cfg::publish_name, cfg::client_permission_name, N(downvote));
        link_authority(cfg::publish_name, cfg::publish_name, cfg::client_permission_name, N(unvote));
        link_authority(cfg::publish_name, cfg::publish_name, cfg::client_permission_name, N(create));
        link_authority(cfg::publish_name, cfg::publish_name, cfg::client_permission_name, N(update));
        link_authority(cfg::publish_name, cfg::publish_name, cfg::client_permission_name, N(settags));
        link_authority(cfg::publish_name, cfg::publish_name, cfg::client_permission_name, N(remove));
        link_authority(cfg::publish_name, cfg::publish_name, cfg::client_permission_name, N(report));
        link_authority(cfg::publish_name, cfg::publish_name, cfg::client_permission_name, N(reblog));
        link_authority(cfg::publish_name, cfg::publish_name, cfg::client_permission_name, N(erasereblog));
    }

    void init() {
        supply  = 5000000000000;
        reserve = 50000000000;
        uint16_t royalty = 2500;

        uint16_t annual_emission_rate = 1000;
        uint16_t leaders_reward_prop = 1000;

        BOOST_CHECK_EQUAL(success(), token.create(_commun, asset(reserve, token._symbol)));
        BOOST_CHECK_EQUAL(success(), token.issue(_commun, _golos, asset(reserve, token._symbol), ""));

        set_authority(_golos, cfg::transfer_permission, create_code_authority({cfg::emit_name}), cfg::active_name);
        link_authority(_golos, cfg::point_name, cfg::transfer_permission, N(transfer));

        BOOST_CHECK_EQUAL(success(), point.create(_golos, asset(0, point._symbol), asset(supply * 2, point._symbol), 10000, 1));
        BOOST_CHECK_EQUAL(success(), point.setfreezer(commun::config::gallery_name));

        BOOST_CHECK_EQUAL(success(), community.create(cfg::list_name, point_code, "community 1"));

        BOOST_CHECK_EQUAL(success(), community.setparams(_golos, point_code, community.args()
            ("emission_rate", annual_emission_rate)
            ("leaders_percent", leaders_reward_prop)));
        
        BOOST_CHECK_EQUAL(success(), token.transfer(_golos, cfg::point_name, asset(reserve, token._symbol), cfg::restock_prefix + point_code_str));
        BOOST_CHECK_EQUAL(success(), point.issue(_golos, _golos, asset(supply, point._symbol), std::string(point_code_str) + " issue"));
        BOOST_CHECK_EQUAL(success(), point.open(_code, point_code, _code));

        produce_block();
        for (auto& u : _users) {
            BOOST_CHECK_EQUAL(success(), point.open(u, point_code, u));
            BOOST_CHECK_EQUAL(success(), point.transfer(_golos, u, asset(supply / _users.size(), point._symbol)));
        }
    }
    
    transaction get_ban_mosaic_trx(const vector<permission_level>& auths, mssgid message_id) {
        fc::variants v;
        for (auto& level : auths) {
            v.push_back(fc::mutable_variant_object()("actor", level.actor)("permission", level.permission));
        }

        variant pretty_trx = fc::mutable_variant_object()
            ("expiration", "2021-01-01T00:30")
            ("ref_block_num", 2)
            ("ref_block_prefix", 3)
            ("max_net_usage_words", 0)
            ("max_cpu_usage_ms", 0)
            ("max_ram_kbytes", 0)
            ("max_storage_kbytes", 0)
            ("delay_sec", 0)
            ("actions", fc::variants({
            fc::mutable_variant_object()
                ("account", cfg::publish_name)
                ("name", "ban")
                ("authorization", v)
                ("data", fc::mutable_variant_object() 
                    ("commun_code", point_code)
                    ("message_id", message_id)
                )}));
       transaction trx;
       abi_serializer::from_variant(pretty_trx, trx, get_resolver(), abi_serializer_max_time);
       return trx;
    }
    
    int64_t supply;
    int64_t reserve;

    struct errors: contract_error_messages {
        const string auth_self                  = "Missing authority of _self. ";
        const string no_param              = amsg("param does not exists");
        const string remove_children       = amsg("comment with child comments can't be removed during the active period");
        const string no_permlink           = amsg("Permlink doesn't exist.");
        const string max_comment_depth     = amsg("publication::create: level > MAX_COMMENT_DEPTH");
        const string max_cmmnt_dpth_less_0 = amsg("Max comment depth must be greater than 0.");
        const string msg_exists            = amsg("This message already exists.");
        const string parent_no_message     = amsg(auth_self + "Parent message doesn't exist");
        const string no_message            = amsg(auth_self + "Message does not exist.");
        const string inactive              = amsg(auth_self + "Mosaic is inactive.");
        const string not_enough_for_gem    = amsg(auth_self + "points are not enough for gem inclusion");
        const string no_vote               = amsg(auth_self + "vote doesn't exist");

        const string wrong_prmlnk_length   = amsg("Permlink length is empty or more than 256.");
        const string wrong_prmlnk          = amsg("Permlink contains wrong symbol.");
        const string wrong_title_length    = amsg("Title length is more than 256.");
        const string wrong_body_length     = amsg("Body is empty.");
        const string tags_are_same         = amsg("No changes in tags.");
        const string reason_empty          = amsg("Reason cannot be empty.");

        const string simple_no_message     = amsg("Message does not exist.");
        const string simple_inactive       = amsg("Message is inactive.");
        const string already_locked        = amsg("Message has already been locked");
        
        const string already_removed       = amsg("Message already removed.");
        const string parent_removed        = amsg("Parent message removed.");
        
        const string vote_weight_0         = amsg(auth_self + "weight can't be 0.");
        const string vote_weight_gt100     = amsg("weight can't be more than 100%.");

        const string own_reblog               = amsg("You cannot reblog your own content.");
        const string wrong_reblog_body_length = amsg("Body must be set if title is set.");
        const string own_reblog_erase         = amsg("You cannot erase reblog your own content.");
        const string gem_type_mismatch        = amsg("gem type mismatch");
        const string author_cannot_unvote     = amsg("author can't unvote");
        const string authorization_failed     = amsg("transaction authorization failed");
    } err;
};

BOOST_AUTO_TEST_SUITE(commun_publication_tests)

BOOST_FIXTURE_TEST_CASE(create_message, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Create message testing.");
    init();
    std::string str256(256, 'a');

    BOOST_TEST_MESSAGE("--- checking permlink length.");
    BOOST_CHECK_EQUAL(err.wrong_prmlnk_length, post.create({N(brucelee), ""}));
    BOOST_CHECK_EQUAL(err.wrong_prmlnk_length, post.create({N(brucelee), str256}));

    BOOST_TEST_MESSAGE("--- checking permlink naming convension.");
    BOOST_CHECK_EQUAL(err.wrong_prmlnk, post.create({N(brucelee), "ABC"}));
    BOOST_CHECK_EQUAL(err.wrong_prmlnk, post.create({N(brucelee), "АБЦ"}));
    BOOST_CHECK_EQUAL(success(), post.create({N(brucelee), "abcdefghijklmnopqrstuvwxyz0123456789-"}));

    BOOST_TEST_MESSAGE("--- checking with too long title and empty body.");
    BOOST_CHECK_EQUAL(err.wrong_title_length, post.create({N(brucelee), "test-title"},
        {N(), "parentprmlnk"}, str256));
    BOOST_CHECK_EQUAL(err.wrong_body_length, post.create({N(brucelee), "test-body"},
        {N(), "parentprmlnk"}, "header", ""));

    BOOST_TEST_MESSAGE("--- creating post.");
    BOOST_CHECK_EQUAL(success(), post.create({N(brucelee), "permlink"}));
    produce_block();
    BOOST_TEST_MESSAGE("--- checking its vertex.");
    CHECK_MATCHING_OBJECT(post.get_vertex({N(brucelee), "permlink"}), mvo()
        ("parent_tracery", 0)
        ("level", 0)
        ("childcount", 0)
    );
    BOOST_TEST_MESSAGE("--- checking its mosaic.");
    auto mos = get_mosaic(_code, _point, mssgid{N(brucelee), "permlink"}.tracery());
    BOOST_CHECK(!mos.is_null());

    BOOST_TEST_MESSAGE("--- testing hierarchy.");
    BOOST_CHECK_EQUAL(err.msg_exists, post.create({N(brucelee), "permlink"}));

    BOOST_CHECK_EQUAL(err.parent_no_message, post.create({N(jackiechan), "child"}, {N(notexist), "parent"}));
    BOOST_CHECK_EQUAL(success(), 
        post.create({N(jackiechan), "child1"}, {N(notexist), "parent"}, "h", "b", {"t"}, "m", std::optional<uint16_t>(), _client));
    CHECK_MATCHING_OBJECT(post.get_vertex({N(jackiechan), "child1"}), mvo()
        ("parent_tracery", 0)
        ("level", 0)
        ("childcount", 0)
    );
    BOOST_CHECK_EQUAL(success(), post.create({N(jackiechan), "child"}, {N(brucelee), "permlink"}));
    BOOST_CHECK(!get_mosaic(_code, _point, mssgid{N(jackiechan), "child"}.tracery()).is_null());
    CHECK_MATCHING_OBJECT(post.get_vertex({N(jackiechan), "child"}), mvo()
        ("parent_tracery", mssgid{N(brucelee), "permlink"}.tracery())
        ("level", 1)
        ("childcount", 0)
    );
    CHECK_MATCHING_OBJECT(post.get_vertex({N(brucelee), "permlink"}), mvo()
        ("parent_tracery", 0)
        ("level", 0)
        ("childcount", 1)
    );
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(nesting_level_test, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("nesting level test.");
    init();
    BOOST_CHECK_EQUAL(success(), post.create({N(brucelee), "permlink0"}));
    size_t i = 0;
    for (; i < cfg::max_comment_depth; i++) {
        BOOST_CHECK_EQUAL(success(), post.create(
            {N(brucelee), "permlink" + std::to_string(i+1)},
            {N(brucelee), "permlink" + std::to_string(i)}));
    }
    BOOST_CHECK_EQUAL(err.max_comment_depth, post.create(
        {N(brucelee), "permlink" + std::to_string(i+1)},
        {N(brucelee), "permlink" + std::to_string(i)}));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(update_message, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Update message testing.");
    init();
    BOOST_CHECK_EQUAL(success(), post.create({N(brucelee), "permlink"}));

    BOOST_CHECK_EQUAL(success(), post.update({N(brucelee), "permlink"},
        "headernew", "bodynew", {{"tagnew"}}, "metadatanew"));
    BOOST_CHECK_EQUAL(err.no_message, post.update({N(brucelee), "notexist"},
        "headernew", "bodynew", {{"tagnew"}}, "metadatanew"));
    BOOST_CHECK_EQUAL(success(), post.update({N(brucelee), "notexist"},
        "headernew", "bodynew", {{"tagnew"}}, "metadatanew", _client));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(settags_to_message, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Set message tags by leader testing.");
    init();
    ctrl.prepare({N(jackiechan)}, N(brucelee));
    mssgid msg = {N(brucelee), "permlink"};
    BOOST_CHECK_EQUAL(success(), post.create(msg));

    BOOST_CHECK_EQUAL(errgallery.not_a_leader(N(brucelee)), post.settags(N(brucelee), {N(brucelee), "permlink"}, {"newtag"}, {"oldtag"}, "the reason"));

    BOOST_CHECK_EQUAL(err.no_message, post.settags(N(jackiechan), {N(brucelee), "notexist"}, {"newtag"}, {"oldtag"}, "the reason"));
    BOOST_CHECK_EQUAL(success(), post.settags(N(jackiechan), {N(brucelee), "notexist"}, {"newtag"}, {"oldtag"}, "the reason", _client));

    BOOST_CHECK_EQUAL(success(), post.settags(N(jackiechan), msg, {"newtag"}, {"oldtag"}, "the reason"));
    BOOST_CHECK_EQUAL(success(), post.settags(N(jackiechan), msg, {"newtag"}, {"oldtag"}, ""));
    BOOST_CHECK_EQUAL(success(), post.settags(N(jackiechan), msg, {}, {"oldtag"}, "the reason"));
    BOOST_CHECK_EQUAL(success(), post.settags(N(jackiechan), msg, {"newtag"}, {}, "the reason"));
    BOOST_CHECK_EQUAL(err.tags_are_same, post.settags(N(jackiechan), msg, {}, {}, "the reason"));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(remove_message, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Remove message testing.");
    init();
    BOOST_CHECK_EQUAL(success(), post.create({N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(success(), post.create({N(jackiechan), "child"}, {N(brucelee), "permlink"}));

    BOOST_TEST_MESSAGE("--- fail then remove non-existing post and post with child");
    BOOST_CHECK_EQUAL(err.no_message, post.remove({N(jackiechan), "permlink1"}));
    BOOST_CHECK_EQUAL(success(), post.remove({N(jackiechan), "permlink1"}, _client));
    BOOST_CHECK_EQUAL(success(), post.remove({N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(uint8_t(HIDDEN), get_mosaic(_code, _point, mssgid{N(brucelee), "permlink"}.tracery())["status"].as<uint8_t>());
    BOOST_CHECK(!post.get_vertex({N(brucelee), "permlink"}).is_null());
    BOOST_CHECK_EQUAL(err.parent_removed, post.create({N(jackiechan), "new-child"}, {N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(err.inactive, post.upvote(N(chucknorris), {N(brucelee), "permlink"}));

    BOOST_CHECK_EQUAL(success(), post.remove({N(jackiechan), "child"}));
    BOOST_CHECK(get_mosaic(_code, _point, mssgid{N(jackiechan), "child"}.tracery()).is_null());
    BOOST_CHECK(post.get_vertex({N(jackiechan), "child"}).is_null());
    CHECK_MATCHING_OBJECT(post.get_vertex({N(brucelee), "permlink"}), mvo()
       ("childcount", 0)
    );
    produce_block();
    BOOST_CHECK_EQUAL(err.already_removed, post.remove({N(brucelee), "permlink"}));
    BOOST_CHECK(!post.get_vertex({N(brucelee), "permlink"}).is_null());
    //to completely remove the mosaic without children and votes, claim can be used
    BOOST_CHECK_EQUAL(success(), post.claim({N(brucelee), "permlink"}, N(brucelee), account_name(), true));
    BOOST_CHECK(post.get_vertex({N(brucelee), "permlink"}).is_null());

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(report_message, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Report message testing.");
    init();
    ctrl.prepare({N(jackiechan)}, N(brucelee));

    mssgid msg = {N(brucelee), "permlink"};
    BOOST_CHECK_EQUAL(success(), post.create(msg));

    BOOST_CHECK_EQUAL(err.missing_auth(_code), post.report(N(chucknorris), {N(brucelee), "notexist"}, "the reason"));
    BOOST_CHECK_EQUAL(err.simple_no_message, post.report(N(chucknorris), {N(brucelee), "notexist"}, "the reason", _client));

    BOOST_CHECK_EQUAL(success(), post.report(N(chucknorris), msg, "the reason", _client));
    BOOST_CHECK_EQUAL(err.reason_empty, post.report(N(chucknorris), msg, "", _client));

    BOOST_CHECK_EQUAL(success(), post.lock(N(jackiechan), msg, "the reason"));
    BOOST_CHECK_EQUAL(err.simple_inactive, post.report(N(chucknorris), msg, "the reason", _client));
    BOOST_CHECK_EQUAL(success(), post.unlock(N(jackiechan), msg, "the reason"));
    BOOST_CHECK_EQUAL(err.already_locked, post.report(N(chucknorris), msg, "the reason", _client));

    BOOST_CHECK_EQUAL(success(), post.update(msg, "headernew", "bodynew", {{"tagnew"}}, "metadatanew"));
    BOOST_CHECK_EQUAL(success(), post.report(N(chucknorris), msg, "the reason", _client));

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(reblog_message, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Reblog message testing.");

    init();
    std::string str256(256, 'a');

    BOOST_CHECK_EQUAL(success(), post.create({N(brucelee), "permlink"}));

    BOOST_CHECK_EQUAL(err.missing_auth(_code), post.reblog(N(chucknorris), account_name(), {N(brucelee), "permlink"},
        "header",
        "body"));

    BOOST_CHECK_EQUAL(err.own_reblog, post.reblog(N(brucelee), _client, {N(brucelee), "permlink"},
        "header",
        "body"));
    BOOST_CHECK_EQUAL(err.wrong_title_length, post.reblog(N(chucknorris), _client, {N(brucelee), "permlink"},
        str256,
        "body"));
    BOOST_CHECK_EQUAL(err.wrong_reblog_body_length, post.reblog(N(chucknorris), _client, {N(brucelee), "permlink"},
        "header",
        ""));
    BOOST_CHECK_EQUAL(success(), post.reblog(N(chucknorris), _client, {N(brucelee), "test"},
        "header",
        "body"));
    BOOST_CHECK_EQUAL(success(), post.reblog(N(chucknorris), _client, {N(brucelee), "test"},
        "header",
        "body"));
    BOOST_CHECK_EQUAL(success(), post.reblog(N(chucknorris), _client, {N(brucelee), "permlink"},
        "header",
        "body"));
    BOOST_CHECK_EQUAL(success(), post.reblog(N(jackiechan), _client, {N(brucelee), "permlink"},
        "",
        ""));
    BOOST_CHECK_EQUAL(success(), post.reblog(_golos, _client, {N(brucelee), "permlink"},
        "",
        "body"));

    BOOST_CHECK_EQUAL(err.missing_auth(_code), post.erase_reblog(N(chucknorris), account_name(),
        {N(brucelee), "notexist"}));
    BOOST_CHECK_EQUAL(err.own_reblog_erase, post.erase_reblog(N(brucelee), _client,
        {N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(success(), post.erase_reblog(N(chucknorris), _client,
        {N(brucelee), "notexist"}));
    BOOST_CHECK_EQUAL(success(), post.erase_reblog(N(chucknorris), _client,
        {N(brucelee), "notexist"}));
    BOOST_CHECK_EQUAL(success(), post.erase_reblog(N(chucknorris), _client,
        {N(brucelee), "permlink"}));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(upvote, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Upvote testing.");
    auto permlink = "permlink";
    auto vote_brucelee = [&](auto weight){ return post.upvote(N(brucelee), {N(brucelee), permlink}, weight); };
    BOOST_CHECK_EQUAL(errgallery.no_community, vote_brucelee(1));
    init();
    BOOST_CHECK_EQUAL(success(), community.setsysparams( point_code, community.sysparams()("refill_gem_enabled", true)));
    BOOST_CHECK_EQUAL(err.no_message, vote_brucelee(1));
    BOOST_CHECK_EQUAL(success(), post.create({N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(err.vote_weight_gt100, vote_brucelee(cfg::_100percent+1));
    BOOST_CHECK_EQUAL(success(), vote_brucelee(cfg::_100percent));
    auto gem = get_gem(_code, _point, mssgid{N(brucelee), "permlink"}.tracery(), N(brucelee));
    BOOST_CHECK(!gem.is_null());
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(downvote, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Downvote testing.");
    auto permlink = "permlink";
    auto vote_brucelee = [&](auto weight){ return post.downvote(N(brucelee), {N(brucelee), permlink}, weight); };
    BOOST_CHECK_EQUAL(errgallery.no_community, vote_brucelee(1));
    init();
    BOOST_CHECK_EQUAL(success(), community.setsysparams( point_code, community.sysparams()("refill_gem_enabled", true)));
    BOOST_CHECK_EQUAL(err.no_message, vote_brucelee(1));
    BOOST_CHECK_EQUAL(success(), post.create({N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(err.vote_weight_gt100, vote_brucelee(cfg::_100percent+1));
    BOOST_CHECK_EQUAL(err.gem_type_mismatch, vote_brucelee(cfg::_100percent)); //brucelee cannot downvote for his own post
    BOOST_CHECK_EQUAL(success(), post.downvote(N(chucknorris), {N(brucelee), permlink}, cfg::_100percent));
    auto gem = get_gem(_code, _point, mssgid{N(brucelee), "permlink"}.tracery(), N(chucknorris));
    BOOST_CHECK(!gem.is_null());
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(second_vote, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Second vote testing.");
    init();
    BOOST_CHECK_EQUAL(success(), post.create({N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(success(), post.upvote(N(jackiechan), {N(brucelee), "permlink"}));
    produce_block();
    BOOST_CHECK_EQUAL(errgallery.refill, post.upvote(N(jackiechan), {N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(success(), community.setsysparams( point_code, community.sysparams()("refill_gem_enabled", true)));
    BOOST_CHECK_EQUAL(success(), post.upvote(N(jackiechan), {N(brucelee), "permlink"}));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(unvote, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Unvote testing.");
    init();
    BOOST_CHECK_EQUAL(err.no_message, post.unvote(N(chucknorris), {N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(success(), post.unvote(N(chucknorris), {N(brucelee), "permlink"}, _client));
    BOOST_CHECK_EQUAL(success(), post.create({N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(err.author_cannot_unvote, post.unvote(N(brucelee), {N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(err.no_vote, post.unvote(N(chucknorris), {N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(success(), post.upvote(N(chucknorris), {N(brucelee), "permlink"}, 123));
    produce_block();
    BOOST_CHECK(!get_gem(_code, _point, mssgid{N(brucelee), "permlink"}.tracery(), N(chucknorris)).is_null());
    BOOST_CHECK_EQUAL(success(), post.unvote(N(chucknorris), {N(brucelee), "permlink"}));
    BOOST_CHECK(get_gem(_code, _point, mssgid{N(brucelee), "permlink"}.tracery(), N(chucknorris)).is_null());
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(upvote_downvote, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Upvote/downvote testing.");
    init();
    BOOST_CHECK_EQUAL(success(), post.create({N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(success(), post.upvote(N(jackiechan), {N(brucelee), "permlink"}, 100));
    BOOST_CHECK_EQUAL(success(), post.downvote(N(jackiechan), {N(brucelee), "permlink"}, 0, _client)); //empty
    BOOST_CHECK_EQUAL(errgallery.wrong_gem_type, post.downvote(N(jackiechan), {N(brucelee), "permlink"}, 100));
    BOOST_CHECK_EQUAL(success(), post.unvote(N(jackiechan), {N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(success(), post.downvote(N(jackiechan), {N(brucelee), "permlink"}, 100));
    
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(free_messages, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Free messages testing.");
    init();
    create_accounts({N(noob)});
    BOOST_CHECK_EQUAL(success(), point.open(N(noob), point_code));
    BOOST_CHECK_EQUAL(errgallery.overdrawn_balance, post.create({N(noob), "parent"}, {N(), ""}, "h", "b", {"t"}, "m", std::optional<uint16_t>()));
    BOOST_CHECK_EQUAL(success(), post.create({N(noob), "parent"}, {N(), ""}, "h", "b", {"t"}, "m", std::optional<uint16_t>(), _client));
    BOOST_CHECK_EQUAL(errgallery.overdrawn_balance, post.create({N(noob), "child"}, {N(noob), "parent"}, "h", "b", {"t"}, "m", std::optional<uint16_t>()));
    BOOST_CHECK_EQUAL(success(), post.create({N(noob), "child"}, {N(noob), "parent"}, "h", "b", {"t"}, "m", std::optional<uint16_t>(), _client));
    
    std::set<commun::structures::opus_info> new_opuses = {{
        commun::structures::opus_info{ .name = cfg::post_opus_name,    .mosaic_pledge = 0, .min_mosaic_inclusion = 0, .min_gem_inclusion = 1 },
        commun::structures::opus_info{ .name = cfg::comment_opus_name, .mosaic_pledge = 0, .min_mosaic_inclusion = 1, .min_gem_inclusion = 1 }
    }};
    BOOST_CHECK_EQUAL(success(), community.setsysparams( point_code, community.sysparams()("opuses", new_opuses )));
    
    BOOST_CHECK_EQUAL(errgallery.overdrawn_balance, post.create({N(noob), "parent1"}, {N(), ""}, "h", "b", {"t"}, "m", std::optional<uint16_t>()));
    BOOST_CHECK_EQUAL(success(), post.create({N(noob), "parent1"}, {N(), ""}, "h", "b", {"t"}, "m", std::optional<uint16_t>(), _client));
    BOOST_CHECK_EQUAL(errgallery.overdrawn_balance, post.create({N(noob), "child1"}, {N(noob), "parent1"}, "h", "b", {"t"}, "m", std::optional<uint16_t>()));
    BOOST_CHECK_EQUAL(errgallery.overdrawn_balance, post.create({N(noob), "child1"}, {N(noob), "parent1"}, "h", "b", {"t"}, "m", std::optional<uint16_t>(), _client));
    
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(empty_votes, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Empty votes testing.");
    init();
    
    BOOST_CHECK_EQUAL(success(), post.create({N(brucelee), "permlink"}));
    BOOST_CHECK_EQUAL(err.vote_weight_0, post.upvote(N(jackiechan), {N(brucelee), "permlink"}, 0));
    BOOST_CHECK_EQUAL(success(), post.upvote(N(jackiechan), {N(brucelee), "permlink"}, 0, _client));
    BOOST_CHECK(get_gem(_code, _point, mssgid{N(brucelee), "permlink"}.tracery(), N(jackiechan)).is_null());
    
    BOOST_CHECK_EQUAL(err.no_message, post.upvote(N(jackiechan), {N(brucelee), "permlink1"}));
    BOOST_CHECK_EQUAL(success(), post.upvote(N(jackiechan), {N(brucelee), "permlink1"}, std::optional<uint16_t>(), _client));
    
    std::set<commun::structures::opus_info> new_opuses = {{
        commun::structures::opus_info{ cfg::post_opus_name, 100, 100, 100 },
        commun::structures::opus_info{ cfg::comment_opus_name }
    }};
    
    BOOST_CHECK_EQUAL(success(), community.setsysparams( point_code, community.sysparams()("opuses", new_opuses )));
    BOOST_CHECK_EQUAL(success(), post.create({N(brucelee), "permlink1"}));
    BOOST_CHECK_EQUAL(success(), post.upvote(N(jackiechan), {N(brucelee), "permlink1"}));
    BOOST_CHECK(!get_gem(_code, _point, mssgid{N(brucelee), "permlink1"}.tracery(), N(jackiechan)).is_null());
    
    create_accounts({N(noob)});
    BOOST_CHECK_EQUAL(success(), point.open(N(noob), point_code));
    BOOST_CHECK_EQUAL(err.not_enough_for_gem, post.downvote(N(noob), {N(brucelee), "permlink1"}));
    BOOST_CHECK_EQUAL(success(), post.downvote(N(noob), {N(brucelee), "permlink1"}, std::optional<uint16_t>(), _client));
    BOOST_CHECK(get_gem(_code, _point, mssgid{N(brucelee), "permlink1"}.tracery(), N(noob)).is_null());
    BOOST_CHECK_EQUAL(err.no_vote, post.unvote(N(noob), {N(brucelee), "permlink1"}));
    BOOST_CHECK_EQUAL(success(), post.unvote(N(noob), {N(brucelee), "permlink1"}, _client));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(setproviders, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("setproviders testing.");
    BOOST_CHECK_EQUAL(errgallery.no_community, post.setproviders(N(brucelee), {N(chucknorris)}));
    init();
    BOOST_CHECK_EQUAL(success(), post.setproviders(N(brucelee), {N(chucknorris)}));
    BOOST_CHECK(!post.get_accparam(N(brucelee)).is_null());
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(set_gem_holders, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Set gem holders testing.");
    init();
    BOOST_CHECK_EQUAL(success(), post.create({N(alice), "facelift"}));
    BOOST_CHECK_EQUAL(success(), post.create({N(alice), "dirt"}));
    BOOST_CHECK_EQUAL(success(), post.create({N(alice), "alice-in-blockchains"}));

    //_golos has no tokens to freeze
    BOOST_CHECK_EQUAL(errgallery.overdrawn_balance, post.transfer({N(alice), "dirt"}, N(alice), N(alice), _golos));

    BOOST_CHECK_EQUAL(success(), post.transfer({N(alice), "dirt"}, N(alice), N(alice), N(jackiechan)));
    BOOST_CHECK_EQUAL(success(), post.hold({N(alice), "alice-in-blockchains"}, N(alice)));

    produce_block();
    auto archive_date = cfg::def_collection_period + cfg::def_moderation_period + cfg::def_extra_reward_period;
    produce_block(fc::seconds(archive_date - cfg::block_interval_ms / 1000));

    //a third party can claim it because the active period has expired
    BOOST_CHECK_EQUAL(success(), post.claim({N(alice), "facelift"}, N(alice), N(alice), false, N(chucknorris)));

    BOOST_CHECK_EQUAL(errgallery.nothing_to_claim, post.claim({N(alice), "dirt"}, N(alice), N(alice), false, N(chucknorris)));
    BOOST_CHECK_EQUAL(errgallery.no_authority, post.claim({N(alice), "dirt"}, N(jackiechan), N(alice), false, N(alice)));
    BOOST_CHECK_EQUAL(success(), post.claim({N(alice), "dirt"}, N(jackiechan), N(alice), false, N(jackiechan)));

    BOOST_CHECK_EQUAL(errgallery.no_authority, post.claim({N(alice), "alice-in-blockchains"}, N(alice), N(alice), false, N(chucknorris)));
    BOOST_CHECK_EQUAL(success(), post.claim({N(alice), "alice-in-blockchains"}, N(alice), N(alice), false, N(alice)));

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(advise_message, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Advise message by leader testing.");
    BOOST_CHECK_EQUAL(errgallery.no_community, post.advise(N(jackiechan), {{N(brucelee), "notexist"}}));
    init();
    ctrl.prepare({N(jackiechan)}, N(brucelee));
    mssgid msg = {N(brucelee), "permlink"};
    BOOST_CHECK_EQUAL(success(), post.create(msg));
    std::vector<mssgid> duplicated;
    duplicated.push_back(msg);
    duplicated.push_back(msg);
    BOOST_CHECK_EQUAL(success(), post.advise(N(jackiechan), duplicated));
    BOOST_CHECK_EQUAL(get_mosaic(_code, _point, msg.tracery())["lead_rating"], cfg::advice_weight[0]);
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(lock_message, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Lock message by leader testing.");
    mssgid msg = {N(brucelee), "permlink"};
    BOOST_CHECK_EQUAL(errgallery.reason_empty, post.lock(N(jackiechan), msg, ""));
    BOOST_CHECK_EQUAL(errgallery.reason_empty, post.unlock(N(jackiechan), msg, ""));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(reward_for_downvote, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Reward for downvote testing.");
    init();
    BOOST_CHECK_EQUAL(success(), community.setsysparams( point_code, community.sysparams()("damned_gem_reward_enabled", true)));
    uint16_t weight = 100;
    ctrl.prepare({N(jackiechan), N(brucelee)}, N(chucknorris));
    
    BOOST_CHECK_EQUAL(success(), post.create({N(alice), "facelift"}, {N(), "p"}, "h", "b", {"t"}, "m", weight));
    BOOST_CHECK_EQUAL(success(), post.create({N(alice), "dirt"}, {N(), "p"}, "h", "b", {"t"}, "m", weight));
    BOOST_CHECK_EQUAL(success(), post.create({N(alice), "alice-in-blockchains"}, {N(), "p"}, "h", "b", {"t"}, "m", weight));

    BOOST_CHECK(!get_mosaic(_code, _point, mssgid{N(alice), "facelift"}.tracery())["meritorious"].as<bool>());
    BOOST_CHECK(!get_mosaic(_code, _point, mssgid{N(alice), "dirt"}.tracery())["meritorious"].as<bool>());
    BOOST_CHECK(!get_mosaic(_code, _point, mssgid{N(alice), "alice-in-blockchains"}.tracery())["meritorious"].as<bool>());
    
    set_authority(_golos, cfg::minority_name, create_code_authority({cfg::control_name}), "active");
    link_authority(_golos, cfg::publish_name, cfg::minority_name, N(ban));
    
    BOOST_CHECK_EQUAL(success(), ctrl.propose(N(brucelee), N(banfacelift), cfg::minority_name, 
        get_ban_mosaic_trx({permission_level{_golos, cfg::minority_name}}, {N(alice), "facelift"})));

    BOOST_CHECK_EQUAL(success(), ctrl.propose(N(brucelee), N(bandirt), cfg::minority_name, 
        get_ban_mosaic_trx({permission_level{_golos, cfg::minority_name}}, {N(alice), "dirt"})));

    BOOST_CHECK_EQUAL(success(), ctrl.propose(N(brucelee), N(banthirdone), cfg::minority_name, 
        get_ban_mosaic_trx({permission_level{_golos, cfg::minority_name}}, {N(alice), "alice-in-blockchains"})));
    
    BOOST_CHECK_EQUAL(success(), ctrl.approve(N(brucelee), N(banfacelift), N(brucelee)));
    BOOST_CHECK_EQUAL(success(), ctrl.approve(N(brucelee), N(bandirt), N(brucelee)));
    BOOST_CHECK_EQUAL(success(), ctrl.approve(N(brucelee), N(banthirdone), N(brucelee)));

    produce_block();
    produce_block(fc::seconds(cfg::def_reward_mosaics_period - (cfg::block_interval_ms / 1000)));

    //chucknorris will receive a reward as "facelift" will be in the top and will be banned (*1)
    BOOST_CHECK_EQUAL(success(), post.downvote(N(chucknorris), {N(alice), "facelift"}, weight - 1));

    BOOST_CHECK(get_mosaic(_code, _point, mssgid{N(alice), "facelift"}.tracery())["meritorious"].as<bool>());
    BOOST_CHECK(get_mosaic(_code, _point, mssgid{N(alice), "dirt"}.tracery())["meritorious"].as<bool>());
    BOOST_CHECK(get_mosaic(_code, _point, mssgid{N(alice), "alice-in-blockchains"}.tracery())["meritorious"].as<bool>());

    //jackiechan will not receive a reward because "dirt" will not be in the top, although it will be banned (*2)
    BOOST_CHECK_EQUAL(success(), post.downvote(N(jackiechan), {N(alice), "dirt"}, weight));

    //brucelee will not receive a reward because "alice-in-blockchains" will not be banned (*3)
    BOOST_CHECK_EQUAL(success(), post.downvote(N(brucelee), {N(alice), "alice-in-blockchains"}, weight - 1));

    produce_block();
    produce_block(fc::seconds(cfg::def_reward_mosaics_period - (cfg::block_interval_ms / 1000)));

    BOOST_CHECK_EQUAL(success(), post.create({N(brucelee), "what-are-you-waiting-for-jackie"}));
    BOOST_CHECK_EQUAL(success(), post.hold({N(brucelee), "what-are-you-waiting-for-jackie"}, N(brucelee)));
    
    BOOST_CHECK_EQUAL(success(), ctrl.approve(N(brucelee), N(banfacelift), N(jackiechan)));
    BOOST_CHECK_EQUAL(success(), ctrl.approve(N(brucelee), N(bandirt), N(jackiechan)));
    
    BOOST_CHECK_EQUAL(success(), ctrl.exec(N(brucelee), N(banfacelift), N(brucelee)));
    BOOST_CHECK_EQUAL(success(), ctrl.exec(N(brucelee), N(bandirt), N(brucelee)));
    BOOST_CHECK_EQUAL(err.authorization_failed, ctrl.exec(N(brucelee), N(banthirdone), N(brucelee)));

    BOOST_CHECK_EQUAL(uint8_t(BANNED), get_mosaic(_code, _point, mssgid{N(alice), "facelift"}.tracery())["status"].as<uint8_t>());
    BOOST_CHECK_EQUAL(uint8_t(BANNED), get_mosaic(_code, _point, mssgid{N(alice), "dirt"}.tracery())["status"].as<uint8_t>());
    BOOST_CHECK_EQUAL(uint8_t(ACTIVE), get_mosaic(_code, _point, mssgid{N(alice), "alice-in-blockchains"}.tracery())["status"].as<uint8_t>());

    BOOST_CHECK_EQUAL(errgallery.mosaic_already_banned, post.ban(_golos, {N(alice), "facelift"}));
    BOOST_CHECK_EQUAL(err.inactive, post.downvote(N(chucknorris), {N(alice), "dirt"}, weight));

    produce_block();
    produce_block(fc::seconds((cfg::def_collection_period + cfg::def_moderation_period) - cfg::def_reward_mosaics_period));

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

    //at the end of this story, let's verify that we cannot ban the archive mosaic
    produce_block();
    produce_block(fc::seconds(cfg::def_extra_reward_period + cfg::def_reward_mosaics_period - (cfg::block_interval_ms / 1000)));
    //curious case: first, the existence of the parent permlink is checked, 
    //then the parent mosaic is archived and the parent permlink is destroyed
    BOOST_CHECK_EQUAL(success(), post.create({N(jackiechan), "what"}, {N(brucelee), "what-are-you-waiting-for-jackie"}));
    //therefore, jackie will not be able to create a second comment
    BOOST_CHECK_EQUAL(err.parent_no_message, post.create({N(jackiechan), "hm"}, {N(brucelee), "what-are-you-waiting-for-jackie"}));

    BOOST_CHECK_EQUAL(errgallery.mosaic_archived, post.ban(_golos, {N(brucelee), "what-are-you-waiting-for-jackie"}));

    BOOST_CHECK_EQUAL(success(), post.claim({N(brucelee), "what-are-you-waiting-for-jackie"}, N(brucelee)));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(ban_post_with_comment, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Ban post with comment testing.");
    init();
    ctrl.prepare({N(jackiechan), N(brucelee)}, N(chucknorris));
    set_authority(_golos, cfg::minority_name, create_code_authority({cfg::control_name}), "active");
    link_authority(_golos, cfg::publish_name, cfg::minority_name, N(ban));
    
    BOOST_CHECK_EQUAL(success(), post.create({N(alice), "alice-in-blockchains"}, {N(), "p"}));
    BOOST_CHECK_EQUAL(success(), post.create({N(brucelee), "i-do-not-like-it"}, {N(alice), "alice-in-blockchains"}));

    BOOST_CHECK_EQUAL(success(), ctrl.propose(N(brucelee), N(banpost), cfg::minority_name, 
        get_ban_mosaic_trx({permission_level{_golos, cfg::minority_name}}, {N(alice), "alice-in-blockchains"})));
    
    BOOST_CHECK_EQUAL(success(), ctrl.approve(N(brucelee), N(banpost), N(brucelee)));
    BOOST_CHECK_EQUAL(success(), ctrl.approve(N(brucelee), N(banpost), N(jackiechan))); 
    BOOST_CHECK_EQUAL(success(), ctrl.exec(N(brucelee), N(banpost), N(brucelee)));
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(gem_num_limit, commun_publication_tester) try {
    BOOST_TEST_MESSAGE("Gem num limit testing");
    init();
    static std::set<commun::structures::opus_info> new_opuses = {{
        commun::structures::opus_info{ cfg::post_opus_name, 1, 1, 1 },
        commun::structures::opus_info{ cfg::comment_opus_name }
    }};
    BOOST_CHECK_EQUAL(success(), community.setsysparams( point_code, community.sysparams()("opuses", new_opuses )));
    size_t posts_num = 1000;
    for (size_t i = 0; i < posts_num; i++) {
        if (i % 100 == 0) { BOOST_TEST_MESSAGE("--- i = " << i); }
        mssgid id{N(alice), "alice-in-blockchains-" + std::to_string(i)};
        BOOST_CHECK_EQUAL(success(), post.create(id, {N(), "p"}, "h", "b", {"t"}, "m", 1));
        BOOST_CHECK_EQUAL(success(), post.upvote(N(chucknorris), id));
        produce_block();
        produce_block(fc::seconds(60 * 60 * 24 / cfg::def_gems_per_day));
    }
    
    int64_t seconds_per_day = 24 * 60 * 60;
    int64_t mosaic_active_period = cfg::def_collection_period + cfg::def_moderation_period + cfg::def_extra_reward_period;
    int gems_per_period = cfg::def_gems_per_day * mosaic_active_period / seconds_per_day;
    
    for (size_t i = 0; i <= gems_per_period; i++) {
        mssgid id{N(alice), "alice-in-blockchains--" + std::to_string(i)};
        BOOST_CHECK_EQUAL(success(), post.create(id, {N(), "p"}, "h", "b", {"t"}, "m", 1));
        BOOST_CHECK_EQUAL(i == gems_per_period ? err.not_enough_for_gem : success(), post.upvote(N(jackiechan), id));
        produce_block();
        produce_block(fc::seconds(60 * 60 * 24 / (cfg::def_gems_per_day * 2)));
    }
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
