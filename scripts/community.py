from testnet import *


def issueCommunToken(owner, quantity, clientKey):
    pushAction('cyber.token', 'issue', 'c.issuer@issue', {
            'to':'c.issuer',
            'quantity':quantity,
            'memo':'issue for '+owner
        }, providebw='c.issuer/c@providebw', keys=clientKey)
    pushAction('cyber.token', 'transfer', 'c.issuer@issue', {
            'from':'c.issuer',
            'to':owner,
            'quantity':quantity,
            'memo':'issue for '+owner
        }, providebw='c.issuer/c@providebw', keys=clientKey)

def buyCommunityPoints(owner, quantity, community, ownerKey, clientKey):
    issueCommunToken(owner, quantity, clientKey)
    pushAction('cyber.token', 'transfer', owner, {
            'from':owner,
            'to':'c.point',
            'quantity':quantity,
            'memo':community
        }, providebw=owner+'/c@providebw', keys=[ownerKey, clientKey])
    

def createAndExecProposal(commun_code, permission, trx, leaders, clientKey, *, providebw=None, keys=None):
    (proposer, proposerKey) = leaders[0]
    proposal = randomName()

    pushAction('c.ctrl', 'propose', proposer, {
            'commun_code': commun_code,
            'proposer': proposer,
            'proposal_name': proposal,
            'permission': permission,
            'trx': trx
        }, providebw=proposer+'/c@providebw', keys=[proposerKey, clientKey])

    for (leaderName, leaderKey) in leaders:
        pushAction('c.ctrl', 'approve', leaderName, {
                'proposer': proposer,
                'proposal_name': proposal,
                'approver': leaderName
            }, providebw=leaderName+'/c@providebw', keys=[leaderKey, clientKey])

    providebw = [] if providebw is None else (providebw if type(providebw)==type([]) else [providebw])
    providebw.append(proposer+'/c@providebw')

    pushAction('c.ctrl', 'exec', proposer, {
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
        createAccount(creator_auth, owner_account, 'c@active', creator_auth,
                providebw=creator_account+'/c@providebw', keys=creator_key)
    else:
        owner_account = createRandomAccount('c@active', creator_auth, creator=creator_auth,
                providebw=creator_account+'/c@providebw', keys=creator_key)

    trx = Trx()
    for auth in ('lead.smajor', 'lead.major', 'lead.minor'):
        trx.addAction('cyber', 'updateauth', owner_account, {
                'account': owner_account,
                'permission': auth,
                'parent': 'active',
                'auth': createAuthority([], ['c.ctrl@cyber.code'])})

    trx.addAction('cyber', 'linkauth', owner_account, {
            'account': owner_account,
            'code': 'c.gallery',
            'type': 'ban',
            'requirement': 'lead.minor'})

    trx.addAction('cyber', 'updateauth', owner_account, {
            'account': owner_account,
            'permission': 'transferperm',
            'parent': 'active',
            'auth': createAuthority([], ['c.emit@cyber.code'])})

    trx.addAction('cyber', 'linkauth', owner_account, {
            'account': owner_account,
            'code': 'c.point',
            'type': 'transfer',
            'requirement': 'transferperm'})

    trx.addAction('cyber', 'providebw', 'c@providebw', {
            'provider': 'c',
            'account': owner_account})

    pushTrx(trx, keys=[creator_key])


    # 1. Buy some value of CMN tokens (for testing purposes c.issuer@issue)
    trx = Trx()
    trx.addAction('cyber.token', 'issue', 'c.issuer@issue', {
        'to':'c.issuer',
        'quantity':reserve_amount,
        'memo':"Reserve for {c}".format(c=community_name)
    })
    trx.addAction('cyber.token', 'transfer', 'c.issuer@issue', {
        'from':'c.issuer',
        'to':owner_account,
        'quantity':reserve_amount,
        'memo':"Reserve for {c}".format(c=community_name)
    })
    trx.addAction('cyber', 'providebw', 'c@providebw', {
            'provider': 'c',
            'account': 'c.issuer'})
    pushTrx(trx, keys=creator_key)

    # 2. Create community points
    pushAction('c.point', 'create', 'c.point@clients', {
        'issuer': owner_account,
        'initial_supply': initial_supply,
        'maximum_supply': maximum_supply,
        'cw': cw,
        'fee': fee
    }, providebw='c.point/c@providebw', keys=creator_key)

    # 3. Restock CMN tokens for community points
    transfer(owner_account, 'c.point', reserve_amount, 'restock: {code}'.format(code=symbol.code),
        providebw=owner_account+'/c@providebw', keys=creator_key)

    # 4. Open point balance for c.gallery & c.ctrl
    for acc in ('c.gallery', 'c.ctrl'):
        pushAction('c.point', 'open', 'c@providebw', {
            "owner": acc,
            "commun_code": symbol.code,
            "ram_payer": "c"
        }, keys=creator_key)

    # 5. Register community (c.list:create)
    pushAction('c.list', 'create', 'c.list@clients', {
        "commun_code": symbol.code,
        "community_name": community_name
    }, providebw=['c.list/c@providebw', 'c.emit/c@providebw', 'c.ctrl/c@providebw', 'c.gallery/c@providebw'], keys=creator_key)

    # 6. Pass account to community
    updateAuth(owner_account, 'active', 'owner', [], [owner_account+'@lead.smajor'],
            providebw=owner_account+'/c@providebw', keys=creator_key)

    return owner_account

def openBalance(owner, commun_code, payer, *, providebw=None, keys=None):
    return pushAction('c.point', 'open', payer, {
            'owner':owner,
            'commun_code':commun_code,
            'ram_payer':payer
        }, providebw=providebw, keys=keys)

def regLeader(commun_code, leader, url, *, providebw=None, keys=None):
    return pushAction('c.ctrl', 'regleader', leader, {
            'commun_code': commun_code,
            'leader': leader,
            'url': url
        }, providebw=providebw, keys=keys)

def voteLeader(commun_code, voter, leader, pct, *, providebw=None, keys=None):
    return pushAction('c.ctrl', 'voteleader', voter, {
            'commun_code': commun_code,
            'voter': voter,
            'leader': leader,
            'pct': pct
        }, providebw=providebw, keys=keys)


def createPost(commun_code, author, permlink, category, header, body, *, providebw=None, keys=None):
    return pushAction('c.gallery', 'create', author, {
            'commun_code':commun_code,
            'message_id':{'author':author, 'permlink':permlink}, 
            'parent_id':{'author':"", 'permlink':category}, 
            'header':header,
            'body':body,
            'tags':[],
            'metadata':''
        }, providebw=providebw, keys=keys)

def upvotePost(commun_code, voter, author, permlink, *, providebw=None, keys=None):
    return pushAction('c.gallery', 'upvote', voter, {
            'commun_code':commun_code,
            'voter':voter,
            'message_id':{'author':author, 'permlink':permlink}
        }, providebw=providebw, keys=keys)

def downvotePost(commun_code, voter, author, permlink, *, providebw=None, keys=None):
    return pushAction('c.gallery', 'downvote', voter, {
            'commun_code':commun_code,
            'voter':voter,
            'message_id':{'author':author, 'permlink':permlink}
        }, providebw=providebw, keys=keys)

def unvotePost(commun_code, voter, author, permlink, *, providebw=None, keys=None):
    return pushAction('c.gallery', 'unvote', voter, {
            'commun_code':commun_code,
            'voter':voter,
            'message_id':{'author':author, 'permlink':permlink}
        }, providebw=providebw, keys=keys)

