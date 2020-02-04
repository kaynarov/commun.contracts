#!/usr/bin/python3

import random
import unittest
import testnet
import community
import pymongo
import json
import eehelper as ee
import time
from testcase import TestCase
from copy import deepcopy
from datetime import datetime, timedelta
from testnet import Asset
from jsonprinter import Item, Items, JsonPrinter
from console import Console, ColoredConsole

def from_time_point_sec(s):
    return datetime.strptime(s, '%Y-%m-%dT%H:%M:%S.000')

recoverKey='5J24gjEWSEQFgtngPJNeSKvFsnmAG8qJVJRgQwnmXoAhxcchxee'
recoverPublic='GLS71iAcPXAqzruvh1EFu28S89Cy8GoYNQXKSQ6UuaBYFuB7usyCB'

techKey    ='5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3'
clientKey  ='5JdhhMMJdb1KEyCatAynRLruxVvi7mWPywiSjpLYqKqgsT4qjsN'
client     ='c.com@c.com'

CMN = testnet.Symbol(4, 'CMN')

class DataItems(Items):
    def __init__(self, *serviceFields, **kwargs):
        super().__init__()
        serviceItems = Items().add(*serviceFields).others(hide=True)
        self.add('_SERVICE_', items=serviceItems)
        self.add('_id', hide=True)
        self.others(**kwargs)

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
        community.createPost('CATS', author, permlink, 'cats', header, body,
                providebw=[author+'/c@providebw','c.gallery/c@providebw'], client='c.gallery@clients', keys=[private, clientKey])

        community.upvotePost('CATS', voter, author, permlink,
                providebw=[voter+'/c@providebw','c.gallery/c@providebw'], client='c.gallery@clients', keys=[private2, clientKey])


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
            testnet.msigPropose(proposer, 'recovery', requestedPermissions, trx, providebw=proposer+'/c@providebw', keys=[proposerKey, clientKey])

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
        testnet.msigPropose(proposer, 'recovery', requestedPermissions, trx, providebw=proposer+'/c@providebw', keys=[proposerKey, clientKey])

        # trying setcode with not enough approvers
        for (approver, approverKey) in self.smajor_approvers[:-1]:
            testnet.msigApprove(approver, proposer, 'recovery', providebw=approver+'/c@providebw', keys=[approverKey, clientKey])
        with self.assertRaisesRegex(Exception, 'assertion failure with message: transaction authorization failed'):
            testnet.msigExec(proposer, proposer, 'recovery', providebw=proposer+'/c@providebw', keys=[proposerKey, clientKey])

        # trying setcode with enough approvers
        (approver, approverKey) = self.smajor_approvers[-1]
        testnet.msigApprove(approver, proposer, 'recovery', providebw=approver+'/c@providebw', keys=[approverKey, clientKey])
        testnet.msigExec(proposer, proposer, 'recovery', providebw=proposer+'/c@providebw', keys=[proposerKey, clientKey])

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

    def test_banPostUsingMinorAuthority(self):
        (private, public) = testnet.createKey()
        author = testnet.createRandomAccount(public, keys=techKey)
        community.openBalance(author, self.point, 'tech', keys=techKey)
        community.buyCommunityPoints(author, '10.0000 CMN', self.point, private, clientKey)

        permlink = testnet.randomPermlink()
        header = testnet.randomText(128)
        body = testnet.randomText(1024)
        community.createPost(self.point, author, permlink, 'cats', header, body,
                providebw=[author+'/c@providebw','c.gallery/c@providebw'], client='c.gallery@clients', keys=[private, clientKey])

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


    def test_banUserUsingMinorAuthority(self):
        (user, userPrivate) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community=self.point)

        # the leader cannot individually block the user
        with self.assertRaisesRegex(Exception, 'Missing required authority'):
            (leader,leaderKey) = next(iter(self.leaders.items()))
            print('leader:', leader)
            testnet.pushAction('c.list', 'ban', leader, 
                    {'commun_code': self.point, 'account': user, 'reason': 'Spammer'},
                    providebw=leader+'/tech', keys=[leaderKey, techKey])

        # ...but leaders can block the user using `lead.minor` consensus
        trx = testnet.Trx()
        trx.addAction('c.list', 'ban', self.owner+'@lead.minor',
                {'commun_code': self.point, 'account': user, 'reason': 'Bad user'})

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



class PointTestCase(TestCase):
    @classmethod
    def setUpClass(self):
        super().setUpClass()

        point = community.getUnusedPointSymbol()
        owner = community.createCommunity(
            community_name = point,
            creator_auth = client,
            creator_key = clientKey,
            maximum_supply = Asset.fromstr('100000000.000 %s'%point),
            reserve_amount = Asset.fromstr('1000000.0000 CMN'))

        buyPointsOn = '%.4f CMN'%(random.uniform(100000.0000, 200000.0000))
        (user, userPrivate) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community=point, buyPointsOn=buyPointsOn)

        self.point = point
        self.owner = owner

        self.accounts[self.owner] = 'point_issuer'

    # This test checks notifications when user sells community points
    def test_buyPoints(self):
        buyPointsOn = '%.4f CMN'%(random.uniform(1000.0000, 2000.0000))
        (alice, alicePrivate) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community=self.point, buyPointsOn=buyPointsOn)

        self.accounts[alice] = 'Alice'

        tokenQuantity = Asset.fromstr('%.4f CMN'%(random.uniform(1000.0000, 10000.0000)))
        community.issueCommunToken(alice, str(tokenQuantity), clientKey)

        # Calculate CT/CP-quantities after transaction
        pointParam = community.getPointParam(self.point)
        pointStat = community.getPointStat(self.point)
        alicePoints = community.getPointBalance(self.point, alice)

        if pointParam['fee'] != 0:
            totalQuantity = tokenQuantity
            tokenQuantity = totalQuantity * (10000-pointParam['fee']) // 10000
            feeQuantity = totalQuantity - tokenQuantity
        else:
            feeQuantity = Asset(0, CMN)

        newReserve = pointStat['reserve'] + tokenQuantity
        q = pow(1.0 + tokenQuantity.amount/pointStat['reserve'].amount, pointParam['cw']/10000)
        newSupply = pointStat['supply'] * q
        pointQuantity = newSupply - pointStat['supply']

        # Buy community points through transfer tokens to 'c.point' account
        buyArgs = {'from':alice, 'to':'c.point', 'quantity':str(tokenQuantity+feeQuantity), 'memo':self.point}
        #testnet.jsonPrinter.printer.console.textColor(20)
        print("<Alice> ({alice}) buy some {point} points through transfer {quantity} to 'c.point' account".
                format(alice=alice, point=self.point, quantity=buyArgs['quantity']))
        #testnet.jsonPrinter.printer.console.textColor()
        buyResult = testnet.pushAction('cyber.token', 'transfer', alice, buyArgs, providebw=alice+'/c@providebw', keys=[alicePrivate, clientKey], output=True)

        buyTrx = buyResult['transaction_id']
        buyBlock = buyResult['processed']['block_num']
        buyTrace = [
            {
                'receiver': 'c.point', 'code': 'cyber.token', 'action': 'transfer',
                'auth': [{'actor': alice, 'permission': 'active'}],
                'args': buyArgs,
                'events': ee.AllItems(
                    {
                        'code': 'c.point', 'event': 'fee',
                        'args': {'amount': str(feeQuantity)}
                    }, {
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
            [ ({'msg_type':'ApplyTrx', 'id':buyTrx, 'block_num':buyBlock}, {'actions':buyTrace, 'except':ee.Missing()}),
            ], buyBlock)


    # This test checks notifications when user sells community points
    def test_sellPoints(self):
        (alice, alicePrivate) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community=self.point, buyPointsOn='%.4f CMN'%(random.uniform(1000.0000, 2000.0000)))

#        tokenQuantity = Asset.fromstr('%.4f CMN'%(random.uniform(1000.0000, 10000.0000)))
#        community.issueCommunToken(alice, str(tokenQuantity), clientKey)

        self.accounts[alice] = 'Alice'

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
        sellResult = testnet.pushAction('c.point', 'transfer', alice, sellArgs, providebw=alice+'/c@providebw', keys=[alicePrivate, clientKey], output=True)

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
                        'args': {'supply': str(pointStat['supply']-sellPoints), 'reserve': str(pointStat['reserve']-(tokenQuantity+feeQuantity))}
                    }, {
                        'code': 'c.point', 'event': 'fee',
                        'args': {'amount': str(feeQuantity)}
                    }, {
                        'code': 'c.point', 'event': 'exchange',
                        'args': {'amount': str(tokenQuantity)}
                    })
            }, {
                'receiver': 'c.ctrl', 'code': 'c.ctrl', 'action': 'changepoints',
                'auth': [{'actor': 'c.ctrl', 'permission': 'changepoints'}],
                'args': {'who': alice, 'diff': str(-sellPoints)},
            }, {
                'receiver': 'cyber.token', 'code': 'cyber.token', 'action': 'transfer',
                'auth': [{'actor': 'c.point', 'permission': 'active'}],
                'args': {'from':'c.point','to':'cyber.null','quantity':str(feeQuantity),'memo':'selling %s fee'%self.point},
            }, {
                'receiver': 'cyber.token', 'code': 'cyber.token', 'action': 'transfer',
                'auth': [{'actor': 'c.point', 'permission': 'active'}],
                'args': {'from':'c.point','to':alice,'quantity':str(tokenQuantity),'memo':'%s sold'%self.point},
            }, {
                'receiver': 'c.ctrl', 'code': 'c.ctrl', 'action': 'changepoints',
                'auth': [{'actor': 'c.ctrl', 'permission': 'changepoints'}],
                'args': {'who': pointParam['issuer'], 'diff': '-%d '%(tokenQuantity.amount+feeQuantity.amount)}
            },
        ]

        self.eeHelper.waitEvents(
            [ ({'msg_type':'ApplyTrx', 'id':sellTrx, 'block_num':sellBlock}, {'actions':sellTrace, 'except':ee.Missing()}),
            ], sellBlock)


    # This test checks notifications when user transfers community points
    def test_transferPoints(self):
        (alice, alicePrivate) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community=self.point, buyPointsOn='%.4f CMN'%(random.uniform(1000.0000, 2000.0000)))

        (bob, bobPrivate) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community=self.point, buyPointsOn='%.4f CMN'%(random.uniform(1000.0000, 2000.0000)))

        self.accounts[alice] = 'Alice'
        self.accounts[bob] = 'Bob'

        # Calculate CT/CP-quantities after transaction
        pointParam = community.getPointParam(self.point)
        pointStat = community.getPointStat(self.point)
        alicePoints = community.getPointBalance(self.point, alice)
        bobPoints = community.getPointBalance(self.point, bob)

        transferPoints = alicePoints * random.uniform(0.1, 0.9)
        feePoints = max(transferPoints*pointParam['transfer_fee']//10000, Asset(pointParam['min_transfer_fee_points'], pointParam['max_supply'].symbol))

        # Transfer community points to other user (not issuer or `c`-account)
        transferArgs = {'from':alice, 'to':bob, 'quantity':str(transferPoints), 'memo':''}
        transferResult = testnet.pushAction('c.point', 'transfer', alice, transferArgs, providebw=alice+'/c@providebw', keys=[alicePrivate, clientKey], output=True)

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
            [ ({'msg_type':'ApplyTrx', 'id':transferTrx, 'block_num':transferBlock}, {'actions':transferTrace, 'except':ee.Missing()}),
            ], transferBlock)


    def test_globalPointsLock(self):
        (alice, alicePrivate) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community=self.point, buyPointsOn='%.4f CMN'%(random.uniform(1000.0000, 2000.0000)))

        (bob, bobPrivate) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community=self.point, buyPointsOn='%.4f CMN'%(random.uniform(1000.0000, 2000.0000)))

        self.accounts[alice] = 'Alice'
        self.accounts[bob] = 'Bob'

        alicePoints = community.getPointBalance(self.point, alice)
        bobPoints = community.getPointBalance(self.point, bob)
        transferPointsA = alicePoints * random.uniform(0.1, 0.9)
        transferPointsB = bobPoints * random.uniform(0.1, 0.9)

        # Alice locks points (for example, after recovery)
        period = 10
        community.lockPoints(alice, period//2, keys=[alicePrivate, clientKey], output=True)
        # Period can be increased but not decreased
        lockResult = community.lockPoints(alice, period, keys=[alicePrivate, clientKey], output=True)
        with self.assertRaisesRegex(Exception, 'new unlock time must be greater than current'):
            community.lockPoints(alice, period - 3 - 1, keys=[alicePrivate, clientKey])

        # Alice can't transfer, retire or withdraw points when locked
        with self.assertRaisesRegex(Exception, 'balance locked in safe'):
            community.transferPoints(alice, bob, transferPointsA, keys=[alicePrivate, clientKey], output=True)
        community.transferPoints(bob, alice, transferPointsB, keys=[bobPrivate, clientKey], output=True)

        # Unlock time is visible in db
        globalLock = community.getPointGlobalLock(alice)

        items = DataItems('scope', 'rev').add('unlocks').others(hide=True)
        print('Alices lock:', self.jsonPrinter.format(items, globalLock))

        # When current time >= unlock time then Alice allowed to transfer, etc…
        lockBlock = lockResult['processed']['block_num']
        targetBlock = lockBlock + (period + 2) // 3
        self.eeHelper.waitEvents([({'msg_type':'AcceptBlock','block_num':targetBlock}, {'block_num':ee.Any(), 'block_time':ee.Any()})], targetBlock)

        community.transferPoints(alice, bob, transferPointsA, keys=[alicePrivate, clientKey], output=True)


class PointSafeTestCase(unittest.TestCase):
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
        (self.alice, self.alicePrivate) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community=self.point, buyPointsOn='%.4f CMN'%(random.uniform(1000.0000, 2000.0000)))

        (self.bob, self.bobPrivate) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community=self.point, buyPointsOn='%.4f CMN'%(random.uniform(1000.0000, 2000.0000)))

        self.alicePoints = community.getPointBalance(self.point, self.alice)
        self.bobPoints = community.getPointBalance(self.point, self.bob)

        pointParam = community.getPointParam(self.point)
        self.feePct = pointParam['transfer_fee']
        self.minFee = Asset(pointParam['min_transfer_fee_points'], pointParam['max_supply'].symbol)

    def tearDown(self):
        self.eeHelper.tearDown()

    def calcFee(self, toTransfer):
        return max(toTransfer * self.feePct // 10000, self.minFee)

    def waitBlock(self, targetBlock):
        self.eeHelper.waitEvents([({'msg_type':'AcceptBlock','block_num':targetBlock}, {})], targetBlock + 1)

    def test_FullStory(self):
        (bob, bobPrivate, bobPoints) = (self.bob, self.bobPrivate, self.bobPoints)
        (alice, alicePrivate, alicePoints) = (self.alice, self.alicePrivate, self.alicePoints)
        keys = [bobPrivate, clientKey]

        # Bob enables safe with initial unlocked value (can be 0) and delay (each point can have individual)
        delay = 30
        unlock = bobPoints // 5
        community.enableSafe(bob, unlock=unlock, delay=delay, keys=keys)
        # Now his transfers (out) limited with "unlocked" value (retire/withdraw also limited)
        toTransfer = unlock // 2
        community.transferPoints(bob, alice, toTransfer, keys=keys)
        unlock -= toTransfer + self.calcFee(toTransfer)

        toTransfer = unlock * 2 // 3
        community.transferPoints(bob, alice, toTransfer, keys=keys)
        unlock -= toTransfer + self.calcFee(toTransfer)

        one = Asset(1, bobPoints.symbol)
        with self.assertRaisesRegex(Exception, 'overdrawn safe unlocked balance'):
            community.transferPoints(bob, alice, unlock + one, keys=keys)
        fee = self.calcFee(unlock)
        toTransfer = unlock - fee # it's not precise, but will be enough
        community.transferPoints(bob, alice, toTransfer, keys=keys)
        community.transferPoints(alice, bob, alicePoints // 5, keys=[alicePrivate, clientKey])
        # Safe state is visible in db
        safe = community.getPointSafe(self.point, bob)
        print('Bobs safe: %s' % safe)

        # Bob can unlock balance, but he should wait "delay" seconds before apply safe mod
        # unlocked value can be > than current balance
        # unique (per account) mod_id should be specified (used later to apply/cancel)
        unlockRes = community.unlockSafe(bob, modId="unlock123", unlock=bobPoints*2, keys=keys)
        reqUnlockBlock = unlockRes['processed']['block_num']

        # Bob also can change safe settings or disable safe, this also requires to wait for "delay" seconds
        community.modifySafe(bob, point=self.point, modId="modify543", delay=delay//2, keys=keys)
        community.modifySafe(bob, point=self.point, modId="modify1", delay=1, keys=keys)
        testnet.wait(5)
        disableRes = community.disableSafe(bob, point=self.point, modId="disable24", keys=keys)

        # Active safe mods are visible in db
        mod1 = community.getPointSafeMod(self.point, bob, "unlock123")
        mod2 = community.getPointSafeMod(self.point, bob, "modify543")
        mod3 = community.getPointSafeMod(self.point, bob, "disable24")
        print('Bobs safe mods:\n  %s\n  %s\n  %s' % (mod1, mod2, mod3))

        # Delayed mods can be cancelled anytime
        community.cancelSafeMod(bob, "modify1", keys=keys)
        # Mod can be applied after delay
        targetBlock = reqUnlockBlock + (delay + 2) // 3
        self.waitBlock(targetBlock)
        community.applySafeMod(bob, "unlock123", keys=keys)
        community.transferPoints(bob, alice, bobPoints // 2, keys=keys)

        # Bob can instantly lock unlocked points if he decides it's too much (0 means lock all unlocked)
        community.lockSafe(bob, bobPoints, keys=keys)
        zero = Asset(0, bobPoints.symbol)
        community.lockSafe(bob, zero, keys=keys)
        with self.assertRaisesRegex(Exception, 'overdrawn safe unlocked balance'):
            community.transferPoints(bob, alice, one, keys=keys)

        # Bob disables safe to remove all restrictions
        disableBlock = disableRes['processed']['block_num'] + (delay + 2) // 3
        if disableBlock > targetBlock:
            self.waitBlock(disableBlock)

        community.applySafeMod(bob, "disable24", keys=keys)
        community.transferPoints(bob, alice, one, keys=keys)

        # He can't re-enable safe if there mods
        with self.assertRaisesRegex(Exception, "Can't enable safe with existing delayed mods"):
            community.enableSafe(bob, unlock=unlock, delay=delay*2, keys=keys)

        # Bob cancels mod and re-enables his safe
        community.cancelSafeMod(bob, "modify543", keys=keys)
        community.enableSafe(bob, unlock=zero, delay=delay*2, trusted=alice, keys=keys)
        testnet.wait(2*3)

        # If safe have trusted account set, then his signature added to action allows to instantly
        # unlock, modify, disable or apply mod. mod id must be empty in this case (except for apply mod)
        with self.assertRaisesRegex(Exception, 'overdrawn safe unlocked balance'):
            community.transferPoints(bob, alice, one, keys=keys)
        toUnlock = one + self.minFee
        community.unlockSafe(bob, modId="", unlock=toUnlock, keys=[bobPrivate, alicePrivate, clientKey], signer=alice)
        community.transferPoints(bob, alice, one, keys=keys)

    def test_enableDisableSafe(self):
        (bob, bobPrivate, bobPoints) = (self.bob, self.bobPrivate, self.bobPoints)
        (alice, alicePrivate, alicePoints) = (self.alice, self.alicePrivate, self.alicePoints)
        keys = [bobPrivate, clientKey]

        # enable safe with 0-unlocked (all funds locked, no outgoing transfers allowed)
        delay = 30
        unlock = Asset(0, bobPoints.symbol)
        community.enableSafe(bob, unlock=unlock, delay=delay, keys=keys)
        one = Asset(1, bobPoints.symbol)
        with self.assertRaisesRegex(Exception, 'overdrawn safe unlocked balance'):
            community.transferPoints(bob, alice, one, keys=keys)
        # incoming transfers are accepted
        community.transferPoints(alice, bob, one, keys=[alicePrivate, clientKey])
        # Safe state is visible in db
        safe = community.getPointSafe(self.point, bob)
        print('Bobs safe: %s' % safe)

        # disable safe using delayed mod
        disableRes = community.disableSafe(bob, point=self.point, modId="disable24", keys=keys)
        # apply mod after delay
        disableBlock = disableRes['processed']['block_num'] + (delay + 2) // 3
        self.waitBlock(disableBlock)
        community.applySafeMod(bob, "disable24", keys=keys)
        community.transferPoints(bob, alice, one, keys=keys)

    def test_unlockSafe(self):
        (bob, bobPrivate, bobPoints) = (self.bob, self.bobPrivate, self.bobPoints)
        (alice, alicePrivate, alicePoints) = (self.alice, self.alicePrivate, self.alicePoints)
        keys = [bobPrivate, clientKey]
        zero = Asset(0, bobPoints.symbol)
        one = Asset(1, bobPoints.symbol)

        # enable safe with all funds locked
        delay = 30
        community.enableSafe(bob, unlock=zero, delay=delay, keys=keys)
        with self.assertRaisesRegex(Exception, 'overdrawn safe unlocked balance'):
            community.transferPoints(bob, alice, one, keys=keys)
        # can unlock some value using delayed mod
        # !! take care about the a fee if want to transfer all unlocked
        toTransfer = bobPoints // 4
        unlock = toTransfer + self.calcFee(toTransfer)
        unlockRes = community.unlockSafe(bob, modId="unlock1", unlock=unlock, keys=keys)
        # apply mod after delay
        unlockBlock = unlockRes['processed']['block_num'] + (delay + 2) // 3
        self.waitBlock(unlockBlock)
        community.applySafeMod(bob, "unlock1", keys=keys)
        community.transferPoints(bob, alice, toTransfer, keys=keys)

        # can unlock more than have
        community.unlockSafe(bob, modId="unlock.x5", unlock=bobPoints*5, keys=keys)
        # several mods can co-exist
        testnet.wait(5)
        community.unlockSafe(bob, modId="unlock.oth", unlock=one, keys=keys)

    def test_lockSafe(self):
        (bob, bobPrivate, bobPoints) = (self.bob, self.bobPrivate, self.bobPoints)
        (alice, alicePrivate, alicePoints) = (self.alice, self.alicePrivate, self.alicePoints)
        keys = [bobPrivate, clientKey]

        # enable safe with some funds unlocked
        delay = 30
        unlock = bobPoints // 2
        community.enableSafe(bob, unlock=unlock, delay=delay, keys=keys)
        # can transfer while have unlocked
        community.transferPoints(bob, alice, unlock // 2, keys=keys)
        # can instantly lock to reduce unlocked
        community.lockSafe(bob, unlock // 3, keys=keys)
        # can pass zero-value asset to lock all unlocked
        zero = Asset(0, bobPoints.symbol)
        community.lockSafe(bob, zero, keys=keys)
        with self.assertRaisesRegex(Exception, 'overdrawn safe unlocked balance'):
            one = Asset(1, bobPoints.symbol)
            community.transferPoints(bob, alice, one, keys=keys)

    def test_modifySafe(self):
        (bob, bobPrivate, bobPoints) = (self.bob, self.bobPrivate, self.bobPoints)
        (alice, alicePrivate, alicePoints) = (self.alice, self.alicePrivate, self.alicePoints)
        keys = [bobPrivate, clientKey]

        # enable safe
        delay = 30
        unlock = bobPoints // 2
        community.enableSafe(bob, unlock=unlock, delay=delay, trusted=alice, keys=keys)

        # can change delay and/or trusted account using delayed mod
        newDelay = delay // 2
        modifyRes = community.modifySafe(bob, point=self.point, modId="set.delay", delay=newDelay, keys=keys)
        community.modifySafe(bob, point=self.point, modId="set.trusted", trusted=self.owner, keys=keys)
        community.modifySafe(bob, point=self.point, modId="del.trusted", trusted="", keys=keys)

        # wait and apply
        modifyBlock = modifyRes['processed']['block_num'] + (delay + 2) // 3
        self.waitBlock(modifyBlock)
        community.applySafeMod(bob, "set.delay", keys=keys)

        # delay changed, mods cerated starting from this time will have new delay
        modifyRes = community.modifySafe(bob, point=self.point, modId="modify.both", delay=delay*2, trusted="", keys=keys)
        modifyBlock = modifyRes['processed']['block_num'] + (newDelay + 2) // 3
        self.waitBlock(modifyBlock)
        community.applySafeMod(bob, "modify.both", keys=keys)

        # !!! existing mods can still be applied if not cancelled (and waited theirs delay)
        # but only if there is actual change
        with self.assertRaisesRegex(Exception, "Change has no effect and can be cancelled"):
            community.applySafeMod(bob, "del.trusted", keys=keys)
        community.applySafeMod(bob, "set.trusted", keys=keys)

    def test_cancelSafeMod(self):
        (bob, bobPrivate, bobPoints) = (self.bob, self.bobPrivate, self.bobPoints)
        (alice, alicePrivate, alicePoints) = (self.alice, self.alicePrivate, self.alicePoints)
        keys = [bobPrivate, clientKey]

        # enable safe
        delay = 30
        unlock = bobPoints // 2
        community.enableSafe(bob, unlock=unlock, delay=delay, keys=keys)

        # schedule some mods
        community.unlockSafe(bob, modId="unlock123", unlock=bobPoints*2, keys=keys)
        testnet.wait(5)
        disableRes = community.disableSafe(bob, point=self.point, modId="disable24", keys=keys)
        testnet.wait(5)
        community.modifySafe(bob, point=self.point, modId="modify543", delay=delay//2, keys=keys)

        # Delayed mods can be cancelled anytime
        community.cancelSafeMod(bob, "modify543", keys=keys)

        # Active safe mods are visible in db
        mod1 = community.getPointSafeMod(self.point, bob, "unlock123")
        mod2 = community.getPointSafeMod(self.point, bob, "disable24")
        mod3 = community.getPointSafeMod(self.point, bob, "modify543")
        print('Bobs safe mods:\n  %s\n  %s\n  %s' % (mod1, mod2, mod3))

        # disable safe applying mod
        disableBlock = disableRes['processed']['block_num'] + (delay + 2) // 3
        self.waitBlock(disableBlock)
        community.applySafeMod(bob, "disable24", keys=keys)

        # Must cancel all active mods (for given point) before re-enable safe
        with self.assertRaisesRegex(Exception, "Can't enable safe with existing delayed mods"):
            community.enableSafe(bob, unlock=unlock, delay=delay, keys=keys)
        community.cancelSafeMod(bob, "unlock123", keys=keys)
        community.enableSafe(bob, unlock=unlock, delay=delay, keys=keys)

    def test_trustedSafe(self):
        (bob, bobPrivate, bobPoints) = (self.bob, self.bobPrivate, self.bobPoints)
        (alice, alicePrivate, alicePoints) = (self.alice, self.alicePrivate, self.alicePoints)
        keys = [bobPrivate, clientKey]

        # enable safe with all funds locked
        delay = 100500
        community.enableSafe(bob, unlock=Asset(0, bobPoints.symbol), delay=delay, trusted=alice, keys=keys)
        toTransfer = bobPoints // 2
        with self.assertRaisesRegex(Exception, 'overdrawn safe unlocked balance'):
            community.transferPoints(bob, alice, toTransfer, keys=keys)

        # If safe have trusted account set, then his signature added to action allows to instantly (without delay)
        # unlock, modify, disable or apply mod.
        # !!! mod id must be empty in this case (except for apply mod)
        unlock = toTransfer + self.calcFee(toTransfer)
        community.unlockSafe(bob, modId="", unlock=unlock, keys=[bobPrivate, alicePrivate, clientKey], signer=alice)
        community.transferPoints(bob, alice, toTransfer, keys=keys)


class CtrlTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        point = community.getUnusedPointSymbol()
        owner = community.createCommunity(
            community_name = point,
            creator_auth = client,
            creator_key = clientKey,
            maximum_supply = Asset.fromstr('100000000.000 %s'%point),
            reserve_amount = Asset.fromstr('1000000.0000 CMN'))

        (self.appLeader1, appLeader1Key) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community='', buyPointsOn='%.4f CMN'%(random.uniform(100000.0000, 200000.0000)), leaderUrl=testnet.randomText(16))

        (self.appLeader2, appLeader2Key) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community='', buyPointsOn='%.4f CMN'%(random.uniform(100000.0000, 200000.0000)), leaderUrl=testnet.randomText(16))

        community.voteLeader('', owner+'@owner', self.appLeader2, 4000, providebw=owner+'/tech', keys=[techKey])

        (self.leader1, leader1Key) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community=point, buyPointsOn='%.4f CMN'%(random.uniform(100000.0000, 200000.0000)), leaderUrl=testnet.randomText(16))

        (self.leader2, leader2Key) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community=point, buyPointsOn='%.4f CMN'%(random.uniform(100000.0000, 200000.0000)), leaderUrl=testnet.randomText(16))

        self.point = point
        self.owner = owner

    def setUp(self):
        self.eeHelper = ee.EEHelper(self)

    def tearDown(self):
        self.eeHelper.tearDown()

    def getLeadersWeights(self, commun_code):
        leaders = {}
        cursor = testnet.mongoClient["_CYBERWAY_c_ctrl"]["leader"].find({"_SERVICE_.scope":commun_code})
        for item in cursor:
            leaders[item['name']] = int(item['total_weight'].to_decimal())
        return leaders

    def test_voteLeader(self):
        cases = (('', self.appLeader1), (self.point, self.leader1))
        for (point, leader) in cases:
          with self.subTest(point):
            (alice, aliceKey) = community.createCommunityUser(
                    creator='tech', creatorKey=techKey, clientKey=clientKey,
                    community=point, buyPointsOn='%.4f CMN'%(random.uniform(100000.0000, 200000.0000)))

            alicePoints = community.getPointBalance(point, alice)
            leaders = self.getLeadersWeights(point)
            leaders[leader] += (30*alicePoints.amount)//100

            trxResult = community.voteLeader(point, alice, leader, 3000, providebw=alice+'/tech', keys=[aliceKey,techKey])
            trxId = trxResult['transaction_id']
            trxBlock = trxResult['processed']['block_num']
            trxTrace = [
                {
                    'receiver': 'c.ctrl', 'code': 'c.ctrl', 'action': 'voteleader',
                    'auth': [{'actor': alice, 'permission': 'active'}],
                    'args': {'commun_code': point, 'voter': alice, 'leader': leader, 'pct': 3000},
                    'events': ee.AllItems(
                        {
                            'code': 'c.ctrl', 'event': 'leaderstate',
                            'args': {'commun_code': point, 'leader': leader, 'weight': leaders[leader]}
                        })
                }
            ]
            self.eeHelper.waitEvents(
                [ ({'msg_type':'ApplyTrx', 'id':trxId}, {'block_num':trxBlock, 'actions':trxTrace, 'except':ee.Missing()}),
                ], trxBlock)

            self.assertEqual(leaders, self.getLeadersWeights(point))


    def test_unvoteLeader(self):
        cases = (('', self.appLeader1), (self.point, self.leader1))
        for (point, leader) in cases:
          with self.subTest(point):
            (alice, aliceKey) = community.createCommunityUser(
                    creator='tech', creatorKey=techKey, clientKey=clientKey,
                    community=point, buyPointsOn='%.4f CMN'%(random.uniform(100000.0000, 200000.0000)))
            community.voteLeader(point, alice, leader, 3000, providebw=alice+'/tech', keys=[aliceKey,techKey])

            alicePoints = community.getPointBalance(point, alice)
            leaders = self.getLeadersWeights(point)
            leaders[leader] -= (30*alicePoints.amount)//100

            trxResult = community.unvoteLeader(point, alice, leader, providebw=alice+'/tech', keys=[aliceKey,techKey])
            trxId = trxResult['transaction_id']
            trxBlock = trxResult['processed']['block_num']
            trxTrace = [
                {
                    'receiver': 'c.ctrl', 'code': 'c.ctrl', 'action': 'unvotelead',
                    'auth': [{'actor': alice, 'permission': 'active'}],
                    'args': {'commun_code': point, 'voter': alice, 'leader': leader},
                    'events': ee.AllItems(
                        {
                            'code': 'c.ctrl', 'event': 'leaderstate',
                            'args': {'commun_code': point, 'leader': leader, 'weight': leaders[leader]}
                        })
                }
            ]
            self.eeHelper.waitEvents(
                [ ({'msg_type':'ApplyTrx', 'id':trxId}, {'block_num':trxBlock, 'actions':trxTrace, 'except':ee.Missing()}),
                ], trxBlock)

            self.assertEqual(leaders, self.getLeadersWeights(point))


    def test_voterTransferPoints(self):
        pointParam = community.getPointParam(self.point)
        (point, leader1, leader2) = (self.point, self.leader1, self.leader2)

        (alice, aliceKey) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community=point, buyPointsOn='%.4f CMN'%(random.uniform(100000.0000, 200000.0000)))
        community.voteLeader(point, alice, leader1, 3000, providebw=alice+'/tech', keys=[aliceKey,techKey])
        community.voteLeader(point, alice, leader2, 2000, providebw=alice+'/tech', keys=[aliceKey,techKey])

        (bob, bobKey) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community=point, buyPointsOn='%.4f CMN'%(random.uniform(100000.0000, 200000.0000)))
        community.voteLeader(point, bob, leader2, 7000, providebw=bob+'/tech', keys=[bobKey,techKey])

        alicePoints = community.getPointBalance(point, alice)
        bobPoints = community.getPointBalance(point, bob)

        transferPoints = alicePoints * random.uniform(0.1, 0.9)
        feePoints = max(transferPoints*pointParam['transfer_fee']//10000, Asset(pointParam['min_transfer_fee_points'], pointParam['max_supply'].symbol))
        alicePointsAfter = alicePoints - transferPoints - feePoints
        bobPointsAfter = bobPoints + transferPoints
        self.assertGreaterEqual(alicePointsAfter.amount, 0)

        leaders = self.getLeadersWeights(point)
        leaders[leader1] += 3*alicePointsAfter.amount//10 - 3*alicePoints.amount//10
        leaders[leader2] += 2*alicePointsAfter.amount//10 - 2*alicePoints.amount//10
        leader2AfterAlice = leaders[leader2]
        leaders[leader2] += 7*bobPointsAfter.amount//10 - 7*bobPoints.amount//10

        args = {'from':alice, 'to':bob, 'quantity':str(transferPoints), 'memo':''}
        trxResult = testnet.pushAction('c.point', 'transfer', alice, args, providebw=alice+'/tech', keys=[aliceKey,techKey])
        trxId = trxResult['transaction_id']
        trxBlock = trxResult['processed']['block_num']
        trxTrace = [
            {
                'receiver': 'c.point', 'code': 'c.point', 'action': 'transfer',
                'auth': [{'actor': alice, 'permission': 'active'}],
                'args': args,
            },{
                'receiver': 'c.ctrl', 'code': 'c.ctrl', 'action': 'changepoints',
                'auth': [{'actor': 'c.ctrl', 'permission': 'changepoints'}],
                'args': {'who': alice, 'diff': str(-transferPoints-feePoints)},
                'events': ee.AllItems(
                    {
                        'code': 'c.ctrl', 'event': 'leaderstate',
                        'args': {'commun_code': point, 'leader': leader1, 'weight': leaders[leader1]},
                    },{
                        'code': 'c.ctrl', 'event': 'leaderstate',
                        'args': {'commun_code': point, 'leader': leader2, 'weight': leader2AfterAlice},
                    }
                ),
            },{
                'receiver': 'c.ctrl', 'code': 'c.ctrl', 'action': 'changepoints',
                'args': {'who': bob, 'diff': str(transferPoints)},
                'auth': [{'actor': 'c.ctrl', 'permission': 'changepoints'}],
                'events': ee.AllItems(
                    {
                        'code': 'c.ctrl', 'event': 'leaderstate',
                        'args': {'commun_code': point, 'leader': leader2, 'weight': leaders[leader2]},
                    }
                ),
            },
        ]
        self.eeHelper.waitEvents(
            [ ({'msg_type':'ApplyTrx', 'id':trxId}, {'block_num':trxBlock, 'actions':trxTrace, 'except':ee.Missing()}),
            ], trxBlock)

        self.assertEqual(leaders, self.getLeadersWeights(point))


    def test_voterBuyPoints(self):
        (point, leader) = (self.point, self.leader1)
        (alice, aliceKey) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community=point, buyPointsOn='%.4f CMN'%(random.uniform(100000.0000, 200000.0000)))
        community.voteLeader(point, alice, leader, 3000, providebw=alice+'/tech', keys=[aliceKey,techKey])

        pointParam = community.getPointParam(self.point)
        pointStat = community.getPointStat(self.point)
        alicePoints = community.getPointBalance(self.point, alice)

        tokenQuantity = Asset.fromstr('%.4f CMN'%(random.uniform(100000.0000, 200000.0000)))

        if pointParam['fee'] != 0:
            totalQuantity = tokenQuantity
            tokenQuantity = totalQuantity * (10000-pointParam['fee']) // 10000
            feeQuantity = totalQuantity - tokenQuantity
        else:
            feeQuantity = Asset(0, CMN)

        newReserve = pointStat['reserve'] + tokenQuantity
        q = pow(1.0 + tokenQuantity.amount/pointStat['reserve'].amount, pointParam['cw']/10000)
        newSupply = pointStat['supply'] * q
        pointQuantity = newSupply - pointStat['supply']

        leaders = self.getLeadersWeights(point)
        leaders[leader] += 3*(alicePoints+pointQuantity).amount//10 - 3*alicePoints.amount//10
        appLeaders = self.getLeadersWeights('')
        appLeaders[self.appLeader2] += 4*(pointStat['reserve']+tokenQuantity).amount//10 - 4*pointStat['reserve'].amount//10

        trxResult = community.buyCommunityPoints(alice, tokenQuantity+feeQuantity, point, aliceKey, clientKey)
        trxId = trxResult['transaction_id']
        trxBlock = trxResult['processed']['block_num']

        args = {'from': alice, 'to': 'c.point', 'quantity': str(tokenQuantity+feeQuantity), 'memo': point}
        trxTrace = [
            {
                'receiver': 'cyber.token', 'code': 'cyber.token', 'action': 'transfer',
                'auth': [{'actor': alice, 'permission': 'active'}],
                'args': args,
            },{
                'receiver': 'c.point', 'code': 'cyber.token', 'action': 'transfer',
                'auth': [{'actor': alice,'permission': 'active'}],
                'args': args,
            },{
                'receiver': 'c.ctrl', 'action': 'changepoints', 'code': 'c.ctrl',
                'auth': [{'actor': 'c.ctrl', 'permission': 'changepoints'}],
                'args': {'who': alice, 'diff': str(pointQuantity)},
                'events': ee.AllItems(
                    {
                        'code': 'c.ctrl','event': 'leaderstate',
                        'args': {'commun_code': point, 'leader': leader, 'weight': leaders[leader]},
                    }
                ),
            },{
                'receiver': 'c.ctrl', 'action': 'changepoints', 'code': 'c.ctrl',
                'auth': [{'actor': 'c.ctrl','permission': 'changepoints'}],
                'args': {'who': self.owner, 'diff': '%d '%tokenQuantity.amount},
                'events': ee.AllItems(
                    {
                        'code': 'c.ctrl','event': 'leaderstate',
                        'args': {'commun_code': '', 'leader': self.appLeader2, 'weight': str(appLeaders[self.appLeader2])},
                    }
                ),
            },
        ]
        self.eeHelper.waitEvents(
            [ ({'msg_type':'ApplyTrx', 'id':trxId}, {'block_num':trxBlock, 'actions':trxTrace, 'except':ee.Missing()}),
            ], trxBlock)

        self.assertEqual(leaders, self.getLeadersWeights(point))
        self.assertEqual(appLeaders, self.getLeadersWeights(''))


    def test_voterSellPoints(self):
        (point, leader) = (self.point, self.leader1)
        (alice, aliceKey) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community=self.point, buyPointsOn='%.4f CMN'%(random.uniform(1000.0000, 2000.0000)))
        community.voteLeader(point, alice, leader, 3000, providebw=alice+'/tech', keys=[aliceKey,techKey])

        # Calculate CT/CP-quantities after transaction
        pointParam = community.getPointParam(self.point)
        pointStat = community.getPointStat(self.point)
        alicePoints = community.getPointBalance(self.point, alice)

        sellPoints = alicePoints * random.uniform(0.1, 0.9)
        q = 1.0 - pow(1.0 - sellPoints.amount/pointStat['supply'].amount, 10000/pointParam['cw'])
        totalQuantity = pointStat['reserve'] * q
        tokenQuantity = totalQuantity * (10000-pointParam['fee']) // 10000
        feeQuantity = totalQuantity - tokenQuantity

        leaders = self.getLeadersWeights(point)
        leaders[self.leader1] += 3*(alicePoints-sellPoints).amount//10 - 3*alicePoints.amount//10
        appLeaders = self.getLeadersWeights('')
        appLeaders[self.appLeader2] += 4*(pointStat['reserve']-totalQuantity).amount//10 - 4*pointStat['reserve'].amount//10

        # Sell community points through transfer them to 'c.point' account
        trxArgs = {'from':alice, 'to':'c.point', 'quantity':str(sellPoints), 'memo':''}
        trxResult = testnet.pushAction('c.point', 'transfer', alice, trxArgs, providebw=alice+'/c@providebw', keys=[aliceKey, clientKey])

        trxTrx = trxResult['transaction_id']
        trxBlock = trxResult['processed']['block_num']
        trxTrace = [
            {
                'receiver': 'c.point', 'code': 'c.point', 'action': 'transfer',
                'auth': [{'actor': alice, 'permission': 'active'}],
                'args': trxArgs,
            }, {
                'receiver': 'c.ctrl', 'code': 'c.ctrl', 'action': 'changepoints',
                'auth': [{'actor': 'c.ctrl', 'permission': 'changepoints'}],
                'args': {'who': alice, 'diff': str(-sellPoints)},
                'events': ee.AllItems(
                    {
                        'code': 'c.ctrl','event': 'leaderstate',
                        'args': {'commun_code': point, 'leader': leader, 'weight': leaders[leader]},
                    }
                )
            }, {
                'receiver': 'c.ctrl', 'code': 'c.ctrl', 'action': 'changepoints',
                'auth': [{'actor': 'c.ctrl', 'permission': 'changepoints'}],
                'args': {'who': pointParam['issuer'], 'diff': '-%d '%(tokenQuantity.amount+feeQuantity.amount)},
                'events': ee.AllItems(
                    {
                        'code': 'c.ctrl','event': 'leaderstate',
                        'args': {'commun_code': '', 'leader': self.appLeader2, 'weight': str(appLeaders[self.appLeader2])},
                    }
                ),
            },
        ]

        self.eeHelper.waitEvents(
            [ ({'msg_type':'ApplyTrx', 'id':trxTrx}, {'block_num':trxBlock, 'actions':trxTrace, 'except':ee.Missing()}),
            ], trxBlock)


    def test_voterStakeTokens(self):
        appLeader = self.appLeader1
        (alice, aliceKey) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community='', buyPointsOn='%.4f CMN'%(random.uniform(100000.0000, 200000.0000)))
        community.voteLeader('', alice, appLeader, 3000, providebw=alice+'/tech', keys=[aliceKey,techKey])

        aliceTokens = community.getPointBalance('', alice)
        tokenQuantity = Asset.fromstr('%.4f CMN'%(random.uniform(1000.0000, 10000.0000)))

        print("Alice: %s, %s"%(aliceTokens,tokenQuantity))

        appLeaders = self.getLeadersWeights('')
        appLeaders[appLeader] += 3*(aliceTokens.amount + tokenQuantity.amount)//10 - 3*aliceTokens.amount//10

        trxResult = community.buyCommunityPoints(alice, tokenQuantity, '', aliceKey, clientKey)
        trxId = trxResult['transaction_id']
        trxBlock = trxResult['processed']['block_num']

        args = {'from': alice, 'to': 'c.point', 'quantity': str(tokenQuantity), 'memo': ''}
        trxTrace = [
            {
                'receiver': 'cyber.token', 'code': 'cyber.token', 'action': 'transfer',
                'auth': [{'actor': alice, 'permission': 'active'}],
                'args': args,
            },{
                'receiver': 'c.point', 'code': 'cyber.token', 'action': 'transfer',
                'auth': [{'actor': alice,'permission': 'active'}],
                'args': args,
                'events': ee.AllItems(
                    {
                        'code': 'c.point', 'event': 'balance',
                        'args': {'account': alice, 'balance': '%d '%(aliceTokens.amount + tokenQuantity.amount)}
                    }
                )
            },{
                'receiver': 'c.ctrl', 'action': 'changepoints', 'code': 'c.ctrl',
                'auth': [{'actor': 'c.ctrl', 'permission': 'changepoints'}],
                'args': {'who': alice, 'diff': '%d '%tokenQuantity.amount},
                'events': ee.AllItems(
                    {
                        'code': 'c.ctrl','event': 'leaderstate',
                        'args': {'commun_code': '', 'leader': appLeader, 'weight': appLeaders[appLeader]},
                    }
                ),
            },
        ]
        self.eeHelper.waitEvents(
            [ ({'msg_type':'ApplyTrx', 'id':trxId}, {'block_num':trxBlock, 'actions':trxTrace, 'except':ee.Missing()}),
            ], trxBlock)

        self.assertEqual(appLeaders, self.getLeadersWeights(''))


    def test_voterWithdrawTokens(self):
        appLeader = self.appLeader1
        (alice, aliceKey) = community.createCommunityUser(
                creator='tech', creatorKey=techKey, clientKey=clientKey,
                community='', buyPointsOn='%.4f CMN'%(random.uniform(100000.0000, 200000.0000)))
        community.voteLeader('', alice, appLeader, 3000, providebw=alice+'/tech', keys=[aliceKey,techKey])

        aliceTokens = community.getPointBalance('', alice)
        tokenQuantity = Asset.fromstr('%.4f CMN'%(aliceTokens.amount/10000*random.uniform(0.3, 0.7)))

        print("Alice: %s, %s"%(aliceTokens,tokenQuantity))

        appLeaders = self.getLeadersWeights('')
        appLeaders[appLeader] += 3*(aliceTokens.amount - tokenQuantity.amount)//10 - 3*aliceTokens.amount//10

        args = {'owner': alice, 'quantity': str(tokenQuantity)}
        trxResult = testnet.pushAction('c.point', 'withdraw', alice, args,
               providebw=alice+'/tech', keys=[aliceKey,techKey])
        trxId = trxResult['transaction_id']
        trxBlock = trxResult['processed']['block_num']

        trxTrace = [
            {
                'receiver': 'c.point', 'code': 'c.point', 'action': 'withdraw',
                'auth': [{'actor': alice,'permission': 'active'}],
                'args': args,
                'events': ee.AllItems(
                    {
                        'code': 'c.point', 'event': 'balance',
                        'args': {'account': alice, 'balance': '%d '%(aliceTokens.amount - tokenQuantity.amount)}
                    }
                )
            },{
                'receiver': 'c.ctrl', 'action': 'changepoints', 'code': 'c.ctrl',
                'auth': [{'actor': 'c.ctrl', 'permission': 'changepoints'}],
                'args': {'who': alice, 'diff': '-%d '%tokenQuantity.amount},
                'events': ee.AllItems(
                    {
                        'code': 'c.ctrl','event': 'leaderstate',
                        'args': {'commun_code': '', 'leader': appLeader, 'weight': appLeaders[appLeader]},
                    }
                ),
            },
        ]
        self.eeHelper.waitEvents(
            [ ({'msg_type':'ApplyTrx', 'id':trxId}, {'block_num':trxBlock, 'actions':trxTrace, 'except':ee.Missing()}),
            ], trxBlock)

        self.assertEqual(appLeaders, self.getLeadersWeights(''))



class RecoverTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.recover_delay = 3
        result = json.loads(testnet.cleos('get table c.recover c.recover params'))
        if len(result['rows']) == 0 or result['rows'][0]['recover_delay'] != self.recover_delay:
            testnet.pushAction('c.recover', 'setparams', 'c.recover', {
                    'recover_delay': self.recover_delay
            }, providebw='c.recover/tech', keys=[techKey])
        testnet.updateAuth('c.recover', 'recover', 'active', [recoverPublic], [],
                providebw='c.recover/tech', keys=[techKey])

        self.point = community.getUnusedPointSymbol()
        self.owner = community.createCommunity(
            community_name = self.point,
            creator_auth = client,
            creator_key = clientKey,
            maximum_supply = Asset.fromstr('100000000.000 %s'%self.point),
            reserve_amount = Asset.fromstr('1000000.0000 CMN'))

    def setUp(self):
        (private, public) = testnet.createKey()
        (self.alice, self.aliceKey) = community.createCommunityUser(
            creator='tech', creatorKey=techKey, clientKey=clientKey,
            community=self.point, buyPointsOn='%.4f CMN'%(random.uniform(10000.0000, 200000.0000)))

        testnet.updateAuth(self.alice, 'owner', '', [public], ['c.recover@cyber.code'],
                providebw=self.alice+'/tech', keys=[techKey, self.aliceKey])
        self.aliceOwner = private


    def test_unavailableRecover(self):
        (bobKey, bobPublic) = testnet.createKey()
        bob = testnet.createRandomAccount(bobPublic, keys=[techKey])

        (private2, public2) = testnet.createKey()
        with self.assertRaisesRegex(Exception, 'Key recovery for this account is not available'):
            community.recover(bob, active_key=public2, provider='tech', keys=[recoverKey,techKey])


    def test_recoverActiveAuthority(self):
        (alice, aliceKey) = (self.alice, self.aliceKey)
        self.assertEqual(None, community.getPointGlobalLock(alice))

        (private2, public2) = testnet.createKey()
        result = community.recover(alice, active_key=public2, provider='tech', keys=[recoverKey,techKey])
        recover_time = from_time_point_sec(result['processed']['block_time'])

        # Check new active key with `checkwin` action
        testnet.pushAction('cyber', 'checkwin', alice, {},
                providebw=alice+'/tech', keys=[techKey, private2])

        # Check global lock time
        globalLockTo = from_time_point_sec(community.getPointGlobalLock(alice)['unlocks'])
        self.assertEqual(globalLockTo, recover_time + timedelta(seconds=self.recover_delay))


    def test_applyOwnerRecover(self):
        (alice, aliceKey) = (self.alice, self.aliceKey)

        (private2, public2) = testnet.createKey()
        community.recover(alice, owner_key=public2, provider='tech', keys=[recoverKey,techKey])

        time.sleep(4)
        community.applyOwner(alice, providebw=alice+'/tech', keys=[aliceKey,techKey])

        # Check new owner key with `checkwin` action
        testnet.pushAction('cyber', 'checkwin', alice+'@owner', {},
                providebw=alice+'/tech', keys=[techKey, private2])


    def test_cancelOwnerRecover(self):
        (alice, aliceKey) = (self.alice, self.aliceKey)

        (private2, public2) = testnet.createKey()
        community.recover(alice, owner_key=public2, provider='tech', keys=[recoverKey,techKey])

        community.cancelOwner(alice, providebw=alice+'/tech', keys=[aliceKey,techKey])

        with self.assertRaisesRegex(Exception, "Request for change owner key doesn't exists"):
            community.applyOwner(alice, providebw=alice+'/tech', keys=[aliceKey,techKey])


if __name__ == '__main__':
    unittest.main()
