from testnet import *

def createCommunity(community_name, owner_account, maximum_supply, reserve_amount, *, commun_private_key, owner_private_key, cw=10000, fee=100, annual_emission_rate=1000, leader_reward_prop=2000):
    symbol = maximum_supply.symbol
    initial_supply = Asset(str(maximum_supply))
    initial_supply.amount //= 1000

    # 1. Buy some value of COMMUN tokens
    pushAction('cyber.token', 'issue', 'cmmn', {
        'to':owner_account,
        'quantity':reserve_amount,
        'memo':"Reserve for {c}".format(c=community_name)
    }, keys=commun_private_key)

    # 2. Create community points (need cmmn permission)
    pushAction('cmmn.point', 'create', 'cmmn.point', {
        'issuer': owner_account,
        'maximum_supply': maximum_supply,
        'cw': cw,
        'fee': fee
    }, providebw='cmmn.point/cmmn', keys=commun_private_key)

    # 3. Restock COMMUN tokens for community points
    transfer(owner_account, 'cmmn.point', reserve_amount, 'restock: {code}'.format(code=symbol.code),
        providebw='{acc}/cmmn'.format(acc=owner_account), keys=commun_private_key)

    # 4. Initial supply of community points
    pushAction('cmmn.point', 'issue', owner_account, {
        'to': owner_account,
        'quantity': initial_supply,
        'memo': 'initial'
    }, providebw='{acc}/cmmn'.format(acc=owner_account), keys=owner_private_key)
    
    # 5. Set ctrl-params (cmmn.ctrl:setparams)
    pushAction('cmmn.ctrl', 'setparams', owner_account, {
        "point":symbol.code,
        "params":[
            ["multisig_acc",{"name":owner_account}],
            ["max_witnesses",{"max":21}],
            ["multisig_perms", {"super_majority":0,"majority":0,"minority":0}],
            ["max_witness_votes",{"max":30}]
        ]
    }, providebw='{acc}/cmmn'.format(acc=owner_account), keys=owner_private_key)

    # 6. Create emission (cmmn.emit:create)
    pushAction('cmmn.emit', 'create', 'cmmn.emit', {
        "commun_symbol": symbol,
        "annual_emission_rate": annual_emission_rate,
        "leaders_reward_prop": leader_reward_prop
    }, providebw='cmmn.emit/cmmn', keys=commun_private_key)

    # 7. Open point balance for cmmn.gallery
    pushAction('cmmn.point', 'open', 'cmmn', {
        "owner": "cmmn.gallery",
        "symbol": symbol,
        "ram_payer": "cmmn"
    }, keys=commun_private_key)

    # 8. Set publication params (cmmn.gallery:setparams) (fix to check issuer permission instead of _self)
    pushAction('cmmn.gallery', 'setparams', 'cmmn.gallery', {
        "commun_code":symbol.code,
        "params":[
            ['st_max_comment_depth', {'value': 127}],
            ['st_social_acc', {'value': 'cmmn.social'}],
        ]
    }, providebw='cmmn.gallery/cmmn', keys=commun_private_key)

    # 9. Register community (cmmn.list:create)
    pushAction('cmmn.list', 'create', 'cmmn.list', {
        "token_name": symbol.code,
        "community_name": community_name
    }, providebw='cmmn.list/cmmn', keys=commun_private_key)

def openBalance(owner, symbol, payer, *, providebw=None, keys=None):
    return pushAction('cmmn.point', 'open', payer, {
            'owner':owner,
            'symbol':symbol,
            'ram_payer':payer
        }, providebw=providebw, keys=keys)

def createPost(commun_code, author, permlink, category, header, body, *, curators_prcnt=5000, providebw=None, keys=None):
    return pushAction('cmmn.gallery', 'createmssg', author, {
            'commun_code':commun_code,
            'message_id':{'author':author, 'permlink':permlink}, 
            'parent_id':{'author':"", 'permlink':category}, 
            'headermssg':header,
            'bodymssg':body,
            'languagemssg':'ru',
            'tags':[],
            'jsonmetadata':'',
            'curators_prcnt':curators_prcnt
        }, providebw=providebw, keys=keys)

def upvotePost(commun_code, voter, author, permlink, weight, *, providebw=None, keys=None):
    return pushAction('cmmn.gallery', 'upvote', voter, {
            'commun_code':commun_code,
            'voter':voter,
            'message_id':{'author':author, 'permlink':permlink},
            'weight':weight
        }, providebw=providebw, keys=keys)

def downvotePost(commun_code, voter, author, permlink, weight, *, providebw=None, keys=None):
    return pushAction('cmmn.gallery', 'downvote', voter, {
            'commun_code':commun_code,
            'voter':voter,
            'message_id':{'author':author, 'permlink':permlink},
            'weight':weight
        }, providebw=providebw, keys=keys)

def unvotePost(commun_code, voter, author, permlink, *, providebw=None, keys=None):
    return pushAction('cmmn.gallery', 'unvote', voter, {
            'commun_code':commun_code,
            'voter':voter,
            'message_id':{'author':author, 'permlink':permlink}
        }, providebw=providebw, keys=keys)

