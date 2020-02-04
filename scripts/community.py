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
    return pushAction('cyber.token', 'transfer', owner, {
            'from':owner,
            'to':'c.point',
            'quantity':quantity,
            'memo':community
        }, providebw=owner+'/c@providebw', keys=[ownerKey, clientKey])

def createCommunityUser(community, creator, creatorKey, clientKey, *, buyPointsOn=None, leaderUrl=None):
    (private, public) = createKey()
    account = createRandomAccount(public, creator=creator, keys=creatorKey)
    openBalance(account, community, creator, keys=creatorKey)
    if buyPointsOn:
        buyCommunityPoints(account, buyPointsOn, community, private, clientKey)
    if leaderUrl:
        regLeader(commun_code=community, leader=account, url=leaderUrl,
                providebw=account+'/'+creator, keys=[private, creatorKey])
    return (account, private)

def recover(account, active_key=None, owner_key=None, provider=None, **kwargs):
    args = {'account':account}
    if active_key: args['active_key'] = active_key
    if owner_key: args['owner_key'] = owner_key
    
    providebw = ['c.recover/'+provider] if provider else None
    if active_key and provider: providebw.append(account+'/'+provider)

    return pushAction('c.recover', 'recover', 'c.recover@recover', args, providebw=providebw, **kwargs)

def applyOwner(account, **kwargs):
    return pushAction('c.recover', 'applyowner', account, {'account': account}, **kwargs)

def cancelOwner(account, **kwargs):
    return pushAction('c.recover', 'cancelowner', account, {'account': account}, **kwargs)

def getUnusedPointSymbol():
    while True:
        point = ''.join(random.choice("ABCDEFGHIJKLMNOPQRSTUVWXYZ") for i in range(6))
        if getPointParam(point) == None:
            return point

def getPointParam(point):
    param = mongoClient["_CYBERWAY_c_point"]["param"].find_one({"max_supply._sym":point})
    if param:
        param['max_supply'] = Asset.fromdb(param['max_supply'])
        param['transfer_fee'] = int(param['transfer_fee'].to_decimal())
    return param

def getPointStat(point):
    stat = mongoClient["_CYBERWAY_c_point"]["stat"].find_one({"supply._sym":point})
    if stat:
        stat['supply'] = Asset.fromdb(stat['supply'])
        stat['reserve'] = Asset.fromdb(stat['reserve'])
    return stat

def getPointBalance(point, account):
    res = mongoClient["_CYBERWAY_c_point"]["accounts"].find_one({"balance._sym":point,"_SERVICE_.scope":account})
    if res:
        res = Asset.fromdb(res['balance'])
    return res


def transferPoints(sender, recipient, amount, memo='', **kwargs):
    args = {'from':sender, 'to':recipient, 'quantity':amount, 'memo':memo}
    return pushAction('c.point', 'transfer', sender, args, providebw=sender+'/c@providebw', **kwargs)

def lockPoints(owner, period, **kwargs):
    args = {'owner':owner, 'period':period}
    return pushAction('c.point', 'globallock', owner, args, providebw=owner+'/c@providebw', **kwargs)

def enableSafe(owner, unlock, delay, trusted="", **kwargs):
    args = {'owner':owner, 'unlock':unlock, 'delay':delay, 'trusted':trusted}
    return pushAction('c.point', 'enablesafe', owner, args, providebw=owner+'/c@providebw', **kwargs)

def disableSafe(owner, modId, point, signer=None, **kwargs):
    actor = [owner, signer] if signer else owner
    args = {'owner':owner, 'mod_id':modId, 'commun_code':point}
    return pushAction('c.point', 'disablesafe', actor, args, providebw=owner+'/c@providebw', **kwargs)

def unlockSafe(owner, modId, unlock, signer=None, **kwargs):
    actor = [owner, signer] if signer else owner
    provide = [owner+'/c@providebw', signer+'/c@providebw'] if signer else owner+'/c@providebw'
    args = {'owner':owner, 'mod_id':modId, 'unlock':unlock}
    return pushAction('c.point', 'unlocksafe', actor, args, providebw=provide, **kwargs)

def lockSafe(owner, lock, **kwargs):
    args = {'owner':owner, 'lock':lock}
    return pushAction('c.point', 'locksafe', owner, args, providebw=owner+'/c@providebw', **kwargs)

def modifySafe(owner, modId, point, delay=None, trusted=None, signer=None, **kwargs):
    actor = [owner, signer] if signer else owner
    args = {'owner':owner, 'mod_id':modId, 'commun_code':point}
    if delay is not None: args["delay"] = delay
    if trusted is not None: args["trusted"] = trusted
    return pushAction('c.point', 'modifysafe', actor, args, providebw=owner+'/c@providebw', **kwargs)

def applySafeMod(owner, modId, signer=None, **kwargs):
    actor = [owner, signer] if signer else owner
    args = {'owner':owner, 'mod_id':modId}
    return pushAction('c.point', 'applysafemod', actor, args, providebw=owner+'/c@providebw', **kwargs)

def cancelSafeMod(owner, modId, **kwargs):
    args = {'owner':owner, 'mod_id':modId}
    return pushAction('c.point', 'cancelsafemod', owner, args, providebw=owner+'/c@providebw', **kwargs)


def getPointGlobalLock(account):
    return mongoClient["_CYBERWAY_c_point"]["lock"].find_one({"_SERVICE_.scope":account})

def getPointSafe(point, account):
    res = mongoClient["_CYBERWAY_c_point"]["safe"].find_one({"unlocked._sym":point,"_SERVICE_.scope":account})
    if res:
        res["unlocked"] = Asset.fromdb(res['unlocked'])
    return res

def getPointSafeMod(point, account, modId):
    return mongoClient["_CYBERWAY_c_point"]["safemod"].find_one({"id":modId,"commun_code":point,"_SERVICE_.scope":account})


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


def createCommunity(community_name, creator_auth, creator_key, maximum_supply, reserve_amount, *, cw=3333, fee=100, owner_account=None):
    symbol = maximum_supply.symbol
    initial_supply = Asset.fromstr(str(maximum_supply))
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

    trx.addAction('cyber', 'linkauth', owner_account, {
            'account': owner_account,
            'code': 'c.list',
            'type': 'ban',
            'requirement': 'lead.minor'})

    trx.addAction('cyber', 'linkauth', owner_account, {
            'account': owner_account,
            'code': 'c.list',
            'type': 'unban',
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

def openBalance(owner, commun_code, payer, **kwargs):
    return pushAction('c.point', 'open', payer, {
            'owner':owner,
            'commun_code':commun_code,
            'ram_payer':payer
        }, **kwargs)

def regLeader(commun_code, leader, url, **kwargs):
    return pushAction('c.ctrl', 'regleader', leader, {
            'commun_code': commun_code,
            'leader': leader,
            'url': url
        }, **kwargs)

def voteLeader(commun_code, voter, leader, pct, **kwargs):
    return pushAction('c.ctrl', 'voteleader', voter, {
            'commun_code': commun_code,
            'voter': parseAuthority(voter)['actor'],
            'leader': leader,
            'pct': pct
        }, **kwargs)

def unvoteLeader(commun_code, voter, leader, **kwargs):
    return pushAction('c.ctrl', 'unvotelead', voter, {
            'commun_code': commun_code,
            'voter': voter,
            'leader': leader,
        }, **kwargs)


def createPost(commun_code, author, permlink, category, header, body, *, client=None, **kwargs):
    actors=[author,client] if client else [author]
    return pushAction('c.gallery', 'create', actors, {
            'commun_code':commun_code,
            'message_id':{'author':author, 'permlink':permlink},
            'parent_id':{'author':"", 'permlink':category},
            'header':header,
            'body':body,
            'tags':[],
            'metadata':''
        }, **kwargs)

def upvotePost(commun_code, voter, author, permlink, *, client=None, **kwargs):
    actors=[voter,client] if client else [voter]
    return pushAction('c.gallery', 'upvote', actors, {
            'commun_code':commun_code,
            'voter':voter,
            'message_id':{'author':author, 'permlink':permlink}
        }, **kwargs)

def downvotePost(commun_code, voter, author, permlink, **kwargs):
    return pushAction('c.gallery', 'downvote', voter, {
            'commun_code':commun_code,
            'voter':voter,
            'message_id':{'author':author, 'permlink':permlink}
        }, **kwargs)

def unvotePost(commun_code, voter, author, permlink, **kwargs):
    return pushAction('c.gallery', 'unvote', voter, {
            'commun_code':commun_code,
            'voter':voter,
            'message_id':{'author':author, 'permlink':permlink}
        }, **kwargs)

