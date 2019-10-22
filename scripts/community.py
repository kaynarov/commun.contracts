from testnet import *



def buyCommunityPoints(owner, quantity, community, ownerKey, clientKey):
    pushAction('cyber.token', 'issue', 'comn@issue', {
            'to':'comn',
            'quantity':quantity,
            'memo':'issue for '+owner
        }, keys=clientKey)
    pushAction('cyber.token', 'transfer', 'comn@issue', {
            'from':'comn',
            'to':owner,
            'quantity':quantity,
            'memo':'issue for '+owner
        }, keys=clientKey)
    pushAction('cyber.token', 'transfer', owner, {
            'from':owner,
            'to':'comn.point',
            'quantity':quantity,
            'memo':community
        }, providebw=owner+'/comn@providebw', keys=[ownerKey, clientKey])
    

def createAndExecProposal(commun_code, permission, trx, leaders, clientKey, *, providebw=None, keys=None):
    (proposer, proposerKey) = leaders[0]
    proposal = randomName()

    pushAction('comn.ctrl', 'propose', proposer, {
            'commun_code': commun_code,
            'proposer': proposer,
            'proposal_name': proposal,
            'permission': permission,
            'trx': trx
        }, providebw=proposer+'/comn@providebw', keys=[proposerKey, clientKey])

    for (leaderName, leaderKey) in leaders:
        pushAction('comn.ctrl', 'approve', leaderName, {
                'proposer': proposer,
                'proposal_name': proposal,
                'approver': leaderName
            }, providebw=leaderName+'/comn@providebw', keys=[leaderKey, clientKey])

    providebw = [] if providebw is None else (providebw if type(providebw)==type([]) else [providebw])
    providebw.append(proposer+'/comn@providebw')

    pushAction('comn.ctrl', 'exec', proposer, {
            'proposer': proposer,
            'proposal_name': proposal,
            'executer': proposer
        }, providebw=providebw, keys=[proposerKey, clientKey])


def createCommunity(community_name, creator_auth, creator_key, maximum_supply, reserve_amount, *, cw=10000, fee=100, owner_account=None):
    symbol = maximum_supply.symbol
    initial_supply = Asset(str(maximum_supply))
    initial_supply.amount //= 1000

    c = parseAuthority(creator_auth)
    (creator_account, creator_permission) = (c['actor'],c['permission'])
    creator_auth = '{acc}@{perm}'.format(acc=creator_account, perm=creator_permission)

    # 0. Create owner account
    if owner_account:
        createAccount(creator_auth, owner_account, 'comn@active', creator_auth,
                providebw=creator_account+'/comn@providebw', keys=creator_key)
    else:
        owner_account = createRandomAccount('comn@active', creator_auth, creator=creator_auth,
                providebw=creator_account+'/comn@providebw', keys=creator_key)

    trx = Trx()
    for auth in ('lead.smajor', 'lead.major', 'lead.minor'):
        trx.addAction('cyber', 'updateauth', owner_account, {
                'account': owner_account,
                'permission': auth,
                'parent': 'active',
                'auth': createAuthority([], ['comn.ctrl@cyber.code'])})

    trx.addAction('cyber', 'linkauth', owner_account, {
            'account': owner_account,
            'code': 'comn.gallery',
            'type': 'ban',
            'requirement': 'lead.minor'})

    trx.addAction('cyber', 'providebw', 'comn@providebw', {
            'provider': 'comn',
            'account': owner_account})

    pushTrx(trx, keys=[creator_key])


    # 1. Buy some value of COMMUN tokens (for testing purposes comn@issue)
    trx = Trx()
    trx.addAction('cyber.token', 'issue', 'comn@issue', {
        'to':'comn',
        'quantity':reserve_amount,
        'memo':"Reserve for {c}".format(c=community_name)
    })
    trx.addAction('cyber.token', 'transfer', 'comn@issue', {
        'from':'comn',
        'to':owner_account,
        'quantity':reserve_amount,
        'memo':"Reserve for {c}".format(c=community_name)
    })
    pushTrx(trx, keys=creator_key)

    # 2. Create community points
    pushAction('comn.point', 'create', 'comn.point@clients', {
        'issuer': owner_account,
        'maximum_supply': maximum_supply,
        'cw': cw,
        'fee': fee
    }, providebw='comn.point/comn@providebw', keys=creator_key)

    # 3. Restock COMMUN tokens for community points
    transfer(owner_account, 'comn.point', reserve_amount, 'restock: {code}'.format(code=symbol.code),
        providebw=owner_account+'/comn@providebw', keys=creator_key)

    # 4. Initial supply of community points
    pushAction('comn.point', 'issue', owner_account, {
        'to': owner_account,
        'quantity': initial_supply,
        'memo': 'initial'
    }, providebw=owner_account+'/comn@providebw', keys=creator_key)

    # 5. Open point balance for comn.gallery
    pushAction('comn.point', 'open', 'comn@providebw', {
        "owner": "comn.gallery",
        "commun_code": symbol.code,
        "ram_payer": "comn"
    }, keys=creator_key)

    # 6. Register community (comn.list:create)
    pushAction('comn.list', 'create', 'comn.list@clients', {
        "commun_code": symbol.code,
        "community_name": community_name
    }, providebw=['comn.list/comn@providebw', 'comn.emit/comn@providebw', 'comn.ctrl/comn@providebw'], keys=creator_key)

    # 7. Pass account to community
    updateAuth(owner_account, 'active', 'owner', [], [owner_account+'@lead.smajor'],
            providebw=owner_account+'/comn@providebw', keys=creator_key)

    return owner_account

def openBalance(owner, commun_code, payer, *, providebw=None, keys=None):
    return pushAction('comn.point', 'open', payer, {
            'owner':owner,
            'commun_code':commun_code,
            'ram_payer':payer
        }, providebw=providebw, keys=keys)

def regLeader(commun_code, leader, url, *, providebw=None, keys=None):
    return pushAction('comn.ctrl', 'regleader', leader, {
            'commun_code': commun_code,
            'leader': leader,
            'url': url
        }, providebw=providebw, keys=keys)

def voteLeader(commun_code, voter, leader, pct, *, providebw=None, keys=None):
    return pushAction('comn.ctrl', 'voteleader', voter, {
            'commun_code': commun_code,
            'voter': voter,
            'leader': leader,
            'pct': pct
        }, providebw=providebw, keys=keys)


def createPost(commun_code, author, permlink, category, header, body, *, providebw=None, keys=None):
    return pushAction('comn.gallery', 'create', author, {
            'commun_code':commun_code,
            'message_id':{'author':author, 'permlink':permlink}, 
            'parent_id':{'author':"", 'permlink':category}, 
            'header':header,
            'body':body,
            'tags':[],
            'metadata':''
        }, providebw=providebw, keys=keys)

def upvotePost(commun_code, voter, author, permlink, weight, *, providebw=None, keys=None):
    return pushAction('comn.gallery', 'upvote', voter, {
            'commun_code':commun_code,
            'voter':voter,
            'message_id':{'author':author, 'permlink':permlink},
            'weight':weight
        }, providebw=providebw, keys=keys)

def downvotePost(commun_code, voter, author, permlink, weight, *, providebw=None, keys=None):
    return pushAction('comn.gallery', 'downvote', voter, {
            'commun_code':commun_code,
            'voter':voter,
            'message_id':{'author':author, 'permlink':permlink},
            'weight':weight
        }, providebw=providebw, keys=keys)

def unvotePost(commun_code, voter, author, permlink, *, providebw=None, keys=None):
    return pushAction('comn.gallery', 'unvote', voter, {
            'commun_code':commun_code,
            'voter':voter,
            'message_id':{'author':author, 'permlink':permlink}
        }, providebw=providebw, keys=keys)

