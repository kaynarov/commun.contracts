#!/usr/bin/python3

import random
import unittest
import testnet
import community
import pymongo
import json
import eehelper as ee
from testnet import Asset

techKey    ='5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3'
clientKey  ='5JdhhMMJdb1KEyCatAynRLruxVvi7mWPywiSjpLYqKqgsT4qjsN'
client     ='c.com@c.com'

CMN = testnet.Symbol(4, 'CMN')

class DeployTests(unittest.TestCase):

    def test_createCommunity(self):
        point = community.getUnusedPointSymbol()
        owner = community.createCommunity(
            community_name = point,
            creator_auth = client,
            creator_key = clientKey,
            maximum_supply = Asset.fromstr('100000000.000 %s'%point),
            reserve_amount = Asset.fromstr('1000000.0000 CMN'))

    def test_createPost(self):
        (private, public) = testnet.createKey()
        author = testnet.createRandomAccount(public, keys=techKey)
        community.openBalance(author, 'CATS', 'tech', keys=techKey)
        community.buyCommunityPoints(author, '1000.0000 CMN', 'CATS', private, clientKey)

        (private2, public2) = testnet.createKey()
        voter = testnet.createRandomAccount(public2, keys=techKey)
        community.openBalance(voter, 'CATS', 'tech', keys=techKey)
        community.buyCommunityPoints(voter, '10.0000 CMN', 'CATS', private2, clientKey)

        permlink = testnet.randomPermlink()
        header = testnet.randomText(128)
        body = testnet.randomText(1024)
        community.createPost('CATS', author, permlink, 'cats', header, body, providebw=author+'/c@providebw', keys=[private, clientKey])

        community.upvotePost('CATS', voter, author, permlink, providebw=voter+'/c@providebw', keys=[private2, clientKey])


    def test_glsProvideBW(self):
        (private, public) = testnet.createKey()
        acc = testnet.createRandomAccount(public, keys=techKey)

appLeadersCount = 5
class CommunLeaderTests(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        leaderDb = testnet.mongoClient["_CYBERWAY_c_ctrl"]["leader"]
        leadCursor = leaderDb.find_one({"_SERVICE_.scope":""}, sort=[("total_weight",pymongo.DESCENDING)])
        min_quantity = 10*leadCursor["total_weight"].to_decimal() if leadCursor else 0
        quantity = '%.4f CMN'%(random.randint(min_quantity+10, min_quantity+100)/10000)

        (voterPrivate, voterPublic) = testnet.createKey()
        voter = testnet.createRandomAccount(voterPublic, keys=techKey)
        community.openBalance(voter, '', 'tech', keys=techKey)
        community.buyCommunityPoints(voter, quantity, '', voterPrivate, clientKey)

        leaders = {}
        for i in range(appLeadersCount):
            (private, public) = testnet.createKey()
            leader = testnet.createRandomAccount(public, keys=techKey)
            community.openBalance(leader, '', 'tech', keys=techKey)
            community.regLeader(commun_code='', leader=leader, url=testnet.randomText(16),
                    providebw=leader+'/tech', keys=[techKey, private])
            community.voteLeader(commun_code='', voter=voter, leader=leader, pct=1000,
                    providebw=voter+'/tech', keys=[voterPrivate,techKey])
            leaders[leader] = private

        self.leaders = leaders
        approvers = [ (k,v) for k,v in self.leaders.items() ]
        self.smajor_approvers = approvers[:int(appLeadersCount*3/4+1)]
        self.major_approvers = approvers[:int(appLeadersCount*1/2+1)]
        self.minor_approvers = approvers[:int(appLeadersCount*1/3+1)]

    def printTopLeaders(self):
        leaderDb = testnet.mongoClient["_CYBERWAY_c_ctrl"]["leader"]
        leadCursor = leaderDb.find({"_SERVICE_.scope":""}, sort=[("total_weight",pymongo.DESCENDING)]).limit(appLeadersCount+1)
        for leader in leadCursor:
            print("{leader} {weight}".format(leader=leader['name'], weight=leader['total_weight']))

    def test_canUseActiveAuthority(self):
        trx = testnet.Trx()
        trx.addAction('cyber.token', 'issue', 'c.issuer@active', {
                'to': 'c.issuer',
                'quantity': '0.0001 CMN',
                'memo': ''
            })

        with self.assertRaisesRegex(Exception, 'assertion failure with message: transaction authorization failed'):
            community.createAndExecProposal(
                    commun_code='',
                    permission='lead.smajor',
                    trx=trx,
                    leaders=self.minor_approvers,
                    clientKey=clientKey)

        community.createAndExecProposal(
                commun_code='',
                permission='lead.smajor',
                trx=trx,
                leaders=self.smajor_approvers,
                clientKey=clientKey)

    def test_canIssueCommunityPoints(self):
        issuer = community.getPointParam('CATS')['issuer']
        trx = testnet.Trx()
        trx.addAction('c.point', 'issue', 'c.point', {
                'to': issuer,
                'quantity': '0.001 CATS',
                'memo': 'issue by app leaders'
            })
        trx.addAction('c.point', 'transfer', issuer+'@owner', {
                'from': issuer,
                'to': 'c',
                'quantity': '0.001 CATS',
                'memo': 'transfer by app leaders'
            })

        community.createAndExecProposal(
                commun_code='',
                permission='lead.smajor',
                trx=trx,
                leaders=self.smajor_approvers,
                clientKey=clientKey,
                providebw=issuer+'/c@providebw')

    def test_setRecover(self):
        # trx which is imitating setcode (requires c.ctrl@active too)
        trx = testnet.Trx()
        trx.addAction('c.ctrl', 'setrecover', 'c.ctrl', {})

        requestedPermissions = []
        for (approver, approverKey) in self.smajor_approvers:
            requestedPermissions.append({'actor': approver, 'permission': 'active'})
        (proposer, proposerKey) = self.smajor_approvers[0]

        # trying setcode using cyber.msig without setrecover
        with self.assertRaisesRegex(Exception, 'assertion failure with message: transaction authorization failed'):
            testnet.msigPropose(proposer, 'recovery', requestedPermissions, trx, proposer+'/c@providebw', [proposerKey, clientKey])

        # setrecover adds leaders to lead.recover authority
        trx_setRecover = testnet.Trx()
        trx_setRecover.addAction('c.ctrl', 'setrecover', 'c.ctrl', {})
        community.createAndExecProposal(
                commun_code='',
                permission='lead.smajor',
                trx=trx_setRecover,
                leaders=self.smajor_approvers,
                clientKey=clientKey,
                providebw='c.ctrl/c@providebw')

        # trying setcode again
        testnet.msigPropose(proposer, 'recovery', requestedPermissions, trx, proposer+'/c@providebw', [proposerKey, clientKey])

        # trying setcode with not enough approvers
        for (approver, approverKey) in self.smajor_approvers[:-1]:
            testnet.msigApprove(approver, proposer, 'recovery', approver+'/c@providebw', [approverKey, clientKey])
        with self.assertRaisesRegex(Exception, 'assertion failure with message: transaction authorization failed'):
            testnet.msigExec(proposer, proposer, 'recovery', proposer+'/c@providebw', [proposerKey, clientKey])

        # trying setcode with enough approvers
        (approver, approverKey) = self.smajor_approvers[-1]
        testnet.msigApprove(approver, proposer, 'recovery', approver+'/c@providebw', [approverKey, clientKey])
        testnet.msigExec(proposer, proposer, 'recovery', proposer+'/c@providebw', [proposerKey, clientKey])

        # checking duplicated setrecover not fails
        community.createAndExecProposal(
                commun_code='',
                permission='lead.smajor',
                trx=trx_setRecover,
                leaders=self.smajor_approvers,
                clientKey=clientKey,
                providebw='c.ctrl/c@providebw')

class CommunityLeaderTests(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        point = community.getUnusedPointSymbol()
        owner = community.createCommunity(
            community_name = point,
            creator_auth = client,
            creator_key = clientKey,
            maximum_supply = Asset.fromstr('100000000.000 %s'%point),
            reserve_amount = Asset.fromstr('1000000.0000 CMN'))

        (voter, voterPrivate) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community=point, buyPointsOn='%.4f CMN'%(random.randint(100, 20000)/10000))

        (appLeader, appLeaderPrivate) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community='', leaderUrl=testnet.randomText(16))

        leaders = {}
        for i in range(3):
            (leader, leaderPrivate) = community.createCommunityUser(
                    creator='tech', creatorKey=techKey, clientKey=clientKey,
                    community=point, leaderUrl=testnet.randomText(16))
            leaders[leader] = leaderPrivate

        for leader in leaders.keys():
            community.voteLeader(commun_code=point, voter=voter, leader=leader, pct = 1000,
                    providebw=voter+'/tech', keys=[voterPrivate,techKey])

        self.appLeader = appLeader
        self.point = point
        self.owner = owner
        self.leaders = leaders

        leadersCount = 3
        approvers = [ (k,v) for k,v in self.leaders.items() ]
        self.smajor_approvers = approvers[:int(leadersCount*3/4+1)]
        self.major_approvers = approvers[:int(leadersCount*1/2+1)]
        self.minor_approvers = approvers[:int(leadersCount*1/3+1)]

    def test_setParams(self):
        trx = testnet.Trx()
        trx.addAction('c.list', 'setparams', self.owner+'@active', {
                'commun_code': self.point,
                'author_percent': 5000
            })
        print(json.dumps(trx.getTrx(), indent=3))

        community.createAndExecProposal(
                commun_code=self.point,
                permission='lead.smajor',
                trx=trx,
                leaders=self.smajor_approvers,
                clientKey=clientKey)

    def test_setInfo(self):
        trx = testnet.Trx()
        trx.addAction('c.list', 'setinfo', self.owner+'@active', {
                'commun_code': self.point,
                'description': 'Community description',
                'language': 'ru',
                'rules': 'Community rules',
                'avatar_image': 'http://community/avatar.img',
                'cover_image': 'http://community/cover.img'
            })
        print(json.dumps(trx.getTrx(), indent=3))

        community.createAndExecProposal(
                commun_code=self.point,
                permission='lead.smajor',
                trx=trx,
                leaders=self.smajor_approvers,
                clientKey=clientKey)

    def test_banPostWithMinorAuthority(self):
        (private, public) = testnet.createKey()
        author = testnet.createRandomAccount(public, keys=techKey)
        community.openBalance(author, self.point, 'tech', keys=techKey)
        community.buyCommunityPoints(author, '10.0000 CMN', self.point, private, clientKey)

        permlink = testnet.randomPermlink()
        header = testnet.randomText(128)
        body = testnet.randomText(1024)
        community.createPost(self.point, author, permlink, 'cats', header, body, providebw=author+'/c@providebw', keys=[private, clientKey])

        trx = testnet.Trx()
        trx.addAction('c.gallery', 'ban', self.owner+'@lead.minor', {
                'commun_code': self.point,
                'message_id': {'author': author, 'permlink': permlink}})

        community.createAndExecProposal(
                commun_code=self.point,
                permission='lead.minor',
                trx=trx,
                leaders=self.minor_approvers,
                clientKey=clientKey)


    def test_voteAppLeader(self):
        trx = testnet.Trx()
        trx.addAction('c.ctrl', 'voteleader', self.owner+'@active', {
                'commun_code': '',
                'voter': self.owner,
                'leader': self.appLeader
            })
        print(json.dumps(trx.getTrx(), indent=3))
        community.createAndExecProposal(
                commun_code=self.point,
                permission='lead.smajor',
                trx=trx,
                leaders=self.smajor_approvers,
                clientKey=clientKey,
                providebw=self.owner+'/c@providebw')

        trx = testnet.Trx()
        trx.addAction('c.ctrl', 'unvotelead', self.owner+'@active', {
                'commun_code': '',
                'voter': self.owner,
                'leader': self.appLeader
            })
        print(json.dumps(trx.getTrx(), indent=3))
        community.createAndExecProposal(
                commun_code=self.point,
                permission='lead.smajor',
                trx=trx,
                leaders=self.smajor_approvers,
                clientKey=clientKey,
                providebw=self.owner+'/c@providebw')


class PointTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        point = community.getUnusedPointSymbol()
        owner = community.createCommunity(
            community_name = point,
            creator_auth = client,
            creator_key = clientKey,
            maximum_supply = Asset.fromstr('100000000.000 %s'%point),
            reserve_amount = Asset.fromstr('1000000.0000 CMN'))

        (user, userPrivate) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community=point, buyPointsOn='%.4f CMN'%(random.uniform(100000.0000, 200000.0000)))

        self.point = point
        self.owner = owner

    def setUp(self):
        self.eeHelper = ee.EEHelper(self)

    def tearDown(self):
        self.eeHelper.tearDown()

    # This test checks notifications when user sells community points
    def test_buyPoints(self):
        params = {}

        (alice, alicePrivate) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community=self.point, buyPointsOn='%.4f CMN'%(random.uniform(1000.0000, 2000.0000)))

        tokenQuantity = Asset.fromstr('%.4f CMN'%(random.uniform(1000.0000, 10000.0000)))
        community.issueCommunToken(alice, str(tokenQuantity), clientKey)

        # Calculate CT/CP-quantities after transaction
        pointParam = community.getPointParam(self.point)
        pointStat = community.getPointStat(self.point)
        alicePoints = community.getPointBalance(self.point, alice)

        newReserve = pointStat['reserve'] + tokenQuantity
        q = pow(1.0 + tokenQuantity.amount/pointStat['reserve'].amount, pointParam['cw']/10000)
        newSupply = pointStat['supply'] * q
        pointQuantity = newSupply - pointStat['supply']

        # Buy community points through transfer tokens to 'c.point' account
        buyArgs = {'from':alice, 'to':'c.point', 'quantity':str(tokenQuantity), 'memo':self.point}
        buyResult = testnet.pushAction('cyber.token', 'transfer', alice, buyArgs, providebw=alice+'/c@providebw', keys=[alicePrivate, clientKey])

        buyTrx = buyResult['transaction_id']
        buyBlock = buyResult['processed']['block_num']
        buyTrace = [
            {
                'receiver': 'c.point', 'code': 'cyber.token', 'action': 'transfer',
                'auth': [{'actor': alice, 'permission': 'active'}],
                'args': buyArgs,
                'events': ee.AllItems(
                    {
                        'code': 'c.point', 'event': 'balance',
                        'args': {'account': alice, 'balance': str(alicePoints+pointQuantity)}
                    }, {
                        'code': 'c.point', 'event': 'exchange',
                        'args': {'amount': str(pointQuantity)}
                    }, {
                        'code': 'c.point', 'event': 'currency',
                        'args': {'supply': str(newSupply), 'reserve': str(newReserve)}
                    })
            }, {
                'receiver': 'c.ctrl', 'code': 'c.ctrl', 'action': 'changepoints',
                'auth': [{'actor': 'c.ctrl', 'permission': 'changepoints'}],
                'args': {'who': alice, 'diff': str(pointQuantity)},
            }, {
                'receiver': 'c.ctrl', 'code': 'c.ctrl', 'action': 'changepoints',
                'auth': [{'actor': 'c.ctrl', 'permission': 'changepoints'}],
                'args': {'who': pointParam['issuer'], 'diff': '%d '%tokenQuantity.amount},
            },
        ]

        self.eeHelper.waitEvents(
            [ ({'msg_type':'ApplyTrx', 'id':buyTrx}, {'block_num':buyBlock, 'actions':buyTrace, 'except':ee.Missing()}),
            ], buyBlock)


    # This test checks notifications when user sells community points
    def test_sellPoints(self):
        params = {}
        (alice, alicePrivate) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community=self.point, buyPointsOn='%.4f CMN'%(random.uniform(1000.0000, 2000.0000)))

        tokenQuantity = Asset.fromstr('%.4f CMN'%(random.uniform(1000.0000, 10000.0000)))
        community.issueCommunToken(alice, str(tokenQuantity), clientKey)

        # Calculate CT/CP-quantities after transaction
        pointParam = community.getPointParam(self.point)
        pointStat = community.getPointStat(self.point)
        alicePoints = community.getPointBalance(self.point, alice)

        sellPoints = alicePoints * random.uniform(0.1, 0.9)
        if sellPoints == pointStat['supply']:
            tokenQuantity = pointStat['reserve']
            feeQuantity = Asset(0, CMN)
        else:
            q = 1.0 - pow(1.0 - sellPoints.amount/pointStat['supply'].amount, 10000/pointParam['cw'])
            tokenQuantity = pointStat['reserve'] * q

            if pointParam['fee'] != 0:
                totalQuantity = tokenQuantity
                tokenQuantity = totalQuantity * (10000-pointParam['fee']) // 10000
                feeQuantity = totalQuantity - tokenQuantity
            else:
                feeQuantity = Asset(0, CMN)

        # Sell community points through transfer them to 'c.point' account
        sellArgs = {'from':alice, 'to':'c.point', 'quantity':str(sellPoints), 'memo':''}
        sellResult = testnet.pushAction('c.point', 'transfer', alice, sellArgs, providebw=alice+'/c@providebw', keys=[alicePrivate, clientKey])

        sellTrx = sellResult['transaction_id']
        sellBlock = sellResult['processed']['block_num']
        sellTrace = [
            {
                'receiver': 'c.point', 'code': 'c.point', 'action': 'transfer',
                'auth': [{'actor': alice, 'permission': 'active'}],
                'args': sellArgs,
                'events': ee.AllItems(
                    {
                        'code': 'c.point', 'event': 'balance',
                        'args': {'account': alice, 'balance': str(alicePoints-sellPoints)}
                    }, {
                        'code': 'c.point', 'event': 'currency',
                        'args': {'supply': str(pointStat['supply']-sellPoints), 'reserve': str(pointStat['reserve']-tokenQuantity)}
                    }, {
                        'code': 'c.point', 'event': 'exchange',
                        'args': {'amount': str(tokenQuantity)}
                    }, {
                        'code': 'c.point', 'event': 'fee',
                        'args': {'amount': str(feeQuantity)}
                    })
            }, {
                'receiver': 'c.ctrl', 'code': 'c.ctrl', 'action': 'changepoints',
                'auth': [{'actor': 'c.ctrl', 'permission': 'changepoints'}],
                'args': {'who': alice, 'diff': str(-sellPoints)},
            }, {
                'receiver': 'cyber.token', 'code': 'cyber.token', 'action': 'transfer',
                'auth': [{'actor': 'c.point', 'permission': 'active'}],
                'args': {'from':'c.point','to':alice,'quantity':str(tokenQuantity),'memo':'%s sold'%self.point},
            }, {
                'receiver': 'c.ctrl', 'code': 'c.ctrl', 'action': 'changepoints',
                'auth': [{'actor': 'c.ctrl', 'permission': 'changepoints'}],
                'args': {'who': pointParam['issuer'], 'diff': '-%d '%tokenQuantity.amount}
            },
        ]

        self.eeHelper.waitEvents(
            [ ({'msg_type':'ApplyTrx', 'id':sellTrx}, {'block_num':sellBlock, 'actions':sellTrace, 'except':ee.Missing()}),
            ], sellBlock)


    # This test checks notifications when user transfers community points
    def test_transferPoints(self):
        params = {}

        (alice, alicePrivate) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community=self.point, buyPointsOn='%.4f CMN'%(random.uniform(1000.0000, 2000.0000)))

        (bob, bobPrivate) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community=self.point, buyPointsOn='%.4f CMN'%(random.uniform(1000.0000, 2000.0000)))

        # Calculate CT/CP-quantities after transaction
        pointParam = community.getPointParam(self.point)
        pointStat = community.getPointStat(self.point)
        alicePoints = community.getPointBalance(self.point, alice)
        bobPoints = community.getPointBalance(self.point, bob)

        transferPoints = alicePoints * random.uniform(0.1, 0.9)
        feePoints = max(transferPoints*pointParam['transfer_fee']//10000, Asset(pointParam['min_transfer_fee_points'], pointParam['max_supply'].symbol))

        # Transfer community points to other user (not issuer or `c`-account)
        transferArgs = {'from':alice, 'to':bob, 'quantity':str(transferPoints), 'memo':''}
        transferResult = testnet.pushAction('c.point', 'transfer', alice, transferArgs, providebw=alice+'/c@providebw', keys=[alicePrivate, clientKey])

        transferTrx = transferResult['transaction_id']
        transferBlock = transferResult['processed']['block_num']
        transferTrace = [
            {
                'receiver': 'c.point', 'code': 'c.point', 'action': 'transfer',
                'auth': [{'actor': alice, 'permission': 'active'}],
                'args': transferArgs,
                'events': ee.AllItems(
                    {
                        'code': 'c.point', 'event': 'fee',
                        'args': {'amount': str(feePoints)}
                    }, {
                        'code': 'c.point', 'event': 'currency',
                        'args': {'supply': str(pointStat['supply']-feePoints), 'reserve': str(pointStat['reserve'])}
                    }, {
                        'code': 'c.point', 'event': 'balance',
                        'args': {'account': alice, 'balance': str(alicePoints-transferPoints-feePoints)}
                    }, {
                        'code': 'c.point', 'event': 'balance',
                        'args': {'account': bob, 'balance': str(bobPoints+transferPoints)}
                    })
            }, {
                'receiver': 'c.ctrl', 'code': 'c.ctrl', 'action': 'changepoints',
                'auth': [{'actor': 'c.ctrl', 'permission': 'changepoints'}],
                'args': {'who': alice, 'diff': str(-transferPoints-feePoints)}
            }, {
                'receiver': 'c.ctrl', 'code': 'c.ctrl', 'action': 'changepoints',
                'auth': [{'actor': 'c.ctrl', 'permission': 'changepoints'}],
                'args': {'who': bob, 'diff': str(transferPoints)}
            },
        ]

        self.eeHelper.waitEvents(
            [ ({'msg_type':'ApplyTrx', 'id':transferTrx}, {'block_num':transferBlock, 'actions':transferTrace, 'except':ee.Missing()}),
            ], transferBlock)

if __name__ == '__main__':
    unittest.main()
