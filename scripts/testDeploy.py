#!/usr/bin/python3

import random
import unittest
import testnet
import community
import pymongo
import json
import eehelper as ee

techKey    ='5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3'
clientKey  ='5JdhhMMJdb1KEyCatAynRLruxVvi7mWPywiSjpLYqKqgsT4qjsN'
client     ='c.com@c.com'

class DeployTests(unittest.TestCase):

    def test_createCommunity(self):
        point = ''.join(random.choice("ABCDEFGHIJKLMNOPQRSTUVWXYZ") for i in range(6)) 
        owner = community.createCommunity(
            community_name = point,
            creator_auth = client,
            creator_key = clientKey,
            maximum_supply = testnet.Asset('100000000.000 %s'%point),
            reserve_amount = testnet.Asset('1000000.0000 CMN'))

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
        issuer = testnet.mongoClient["_CYBERWAY_c_point"]["param"].find_one({"max_supply._sym":"CATS"})["issuer"]
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

class CommunityLeaderTests(unittest.TestCase):
    @classmethod
    def setUpClass(self):
#        while True:
#            point = ''.join(random.choice("ABCDEFGHIJKLMNOPQRSTUVWXYZ") for i in range(6)) 
#            owner = 'c..' + point.lower()
#            try:
#                getAccount(owner)
#            except:
#                break

        # TODO: check such point not exist
        point = ''.join(random.choice("ABCDEFGHIJKLMNOPQRSTUVWXYZ") for i in range(6)) 
        owner = community.createCommunity(
            community_name = point,
            creator_auth = client,
            creator_key = clientKey,
            maximum_supply = testnet.Asset('100000000.000 %s'%point),
            reserve_amount = testnet.Asset('1000000.0000 CMN'))

        (voterPrivate, voterPublic) = testnet.createKey()
        voter = testnet.createRandomAccount(voterPublic, keys=techKey)
        community.openBalance(voter, point, 'tech', keys=techKey)
        community.buyCommunityPoints(voter, '%.4f CMN'%(random.randint(100, 20000)/10000), point, voterPrivate, clientKey)

        (appLeaderPrivate, appLeaderPublic) = testnet.createKey()
        appLeader = testnet.createRandomAccount(appLeaderPublic, keys=techKey)
        community.openBalance(appLeader, '', 'tech', keys=techKey)
        community.regLeader(commun_code='', leader=appLeader, url=testnet.randomText(16),
                providebw=appLeader+'/tech', keys=[techKey, appLeaderPrivate])

        leaders = {}
        for i in range(3):
            (private, public) = testnet.createKey()
            leader = testnet.createRandomAccount(public, keys=techKey)
            leaders[leader] = private
            community.openBalance(leader, point, 'tech', keys=techKey)
            community.regLeader(commun_code=point, leader=leader, url=testnet.randomText(16),
                    providebw=leader+'/tech', keys=[techKey, private])

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
    def setUp(self):
        self.eeHelper = ee.EEHelper(self)

    def tearDown(self):
        self.eeHelper.tearDown()

    # This test checks notifications when user sells community points
    def test_buyPoints(self):
        params = {}
        issuer = testnet.mongoClient["_CYBERWAY_c_point"]["param"].find_one({"max_supply._sym":"CATS"})["issuer"]

        (private, public) = testnet.createKey()
        alice = testnet.createRandomAccount(public, keys=techKey)

        community.issueCommunToken(alice, '1.0000 CMN', clientKey)
        community.openBalance(alice, 'CATS', 'tech', keys=techKey)

        # Buy community points through transfer tokens to 'c.point' account
        buyArgs = {'from':alice, 'to':'c.point', 'quantity':'1.0000 CMN', 'memo':'CATS'}
        buyResult = testnet.pushAction('cyber.token', 'transfer', alice, buyArgs, providebw=alice+'/c@providebw', keys=[private, clientKey])

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
                        'args': {'account': alice, 'balance': ee.Save(params,'point-balance')}
                    }, {
                        'code': 'c.point', 'event': 'exchange',
                        'args': {'amount': ee.Save(params,'exchange-points')}
                    }, {
                        'code': 'c.point', 'event': 'currency'
                    })
            }, {
                'receiver': 'c.ctrl', 'code': 'c.ctrl', 'action': 'changepoints',
                'auth': [{'actor': 'c.ctrl', 'permission': 'changepoints'}],
                'args': {'who': alice, 'diff': ee.Save(params,'alice-diff-points')},
            }, {
                'receiver': 'c.ctrl', 'code': 'c.ctrl', 'action': 'changepoints',
                'auth': [{'actor': 'c.ctrl', 'permission': 'changepoints'}],
                'args': {'who': issuer, 'diff': '10000 '},
            },
        ]

        self.eeHelper.waitEvents(
            [ ({'msg_type':'ApplyTrx', 'id':buyTrx}, {'block_num':buyBlock, 'actions':buyTrace, 'except':ee.Missing()}),
            ], buyBlock)
        self.assertRegex(params['exchange-points'], '[0-9]+.[0-9]{3} CATS')
        self.assertEqual(params['exchange-points'], params['point-balance'])   # Due initial point-balance equal zero
        self.assertEqual(params['alice-diff-points'], params['point-balance'])

    # This test checks notifications when user sells community points
    def test_sellPoints(self):
        params = {}
        issuer = testnet.mongoClient["_CYBERWAY_c_point"]["param"].find_one({"max_supply._sym":"CATS"})["issuer"]

        (private, public) = testnet.createKey()
        alice = testnet.createRandomAccount(public, keys=techKey)

        community.issueCommunToken(alice, '1.0000 CMN', clientKey)
        community.openBalance(alice, 'CATS', 'tech', keys=techKey)

        buyArgs = {'from':alice, 'to':'c.point', 'quantity':'1.0000 CMN', 'memo':'CATS'}
        testnet.pushAction('cyber.token', 'transfer', alice, buyArgs, providebw=alice+'/c@providebw', keys=[private, clientKey])

        # Sell community points through transfer them to 'c.point' account
        sellArgs = {'from':alice, 'to':'c.point', 'quantity':'1.000 CATS', 'memo':''}
        sellResult = testnet.pushAction('c.point', 'transfer', alice, sellArgs, providebw=alice+'/c@providebw', keys=[private, clientKey])

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
                        'args': {'account': alice, 'balance': '0.000 CATS'}
                    }, {
                        'code': 'c.point', 'event': 'exchange',
                        'args': {'amount': ee.Save(params,'exchange-tokens')}
                    }, {
                        'code': 'c.point', 'event': 'fee',
                        'args': {'amount': ee.Save(params,'fee-amount')}
                    }, {
                        'code': 'c.point', 'event': 'currency',
                    })
            }, {
                'receiver': 'c.ctrl', 'code': 'c.ctrl', 'action': 'changepoints',
                'auth': [{'actor': 'c.ctrl', 'permission': 'changepoints'}],
                'args': {'who': alice, 'diff': ee.Save(params,'alice-diff-points')},
            }, {
                'receiver': 'cyber.token', 'code': 'cyber.token', 'action': 'transfer',
                'auth': [{'actor': 'c.point', 'permission': 'active'}],
                'args': {'from':'c.point','to':alice,'quantity':ee.Load(params,'exchange-tokens'),'memo':'CATS sold'},
            }, {
                'receiver': 'c.ctrl', 'code': 'c.ctrl', 'action': 'changepoints',
                'auth': [{'actor': 'c.ctrl', 'permission': 'changepoints'}],
                'args': {'who': issuer, 'diff': '-9899 '}, # 1.0000 CMN, minus 0.010 CATS (1% fee) = 0.0100 CMN, and minus 0.0001 (convertation when buying)
            },
        ]

        self.eeHelper.waitEvents(
            [ ({'msg_type':'ApplyTrx', 'id':sellTrx}, {'block_num':sellBlock, 'actions':sellTrace, 'except':ee.Missing()}),
            ], sellBlock)
        self.assertRegex(params['fee-amount'], '[0-9]+.[0-9]{4} CMN')
        self.assertEqual(params['fee-amount'], '0.0100 CMN') # default fee is 1%, cw is 100%
        self.assertEqual(params['alice-diff-points'], '-1.000 CATS')

    # This test checks notifications when user transfers community points
    def test_transferPoints(self):
        params = {}

        (alicePrivate, alicePublic) = testnet.createKey()
        alice = testnet.createRandomAccount(alicePublic, keys=techKey)
        (bobPrivate, bobPublic) = testnet.createKey()
        bob = testnet.createRandomAccount(bobPublic, keys=techKey)

        community.issueCommunToken(alice, '1.0000 CMN', clientKey)
        community.openBalance(alice, 'CATS', 'tech', keys=techKey)

        buyArgs = {'from':alice, 'to':'c.point', 'quantity':'1.0000 CMN', 'memo':'CATS'}
        testnet.pushAction('cyber.token', 'transfer', alice, buyArgs, providebw=alice+'/c@providebw', keys=[alicePrivate, clientKey])

        transferArgs = {'from':alice, 'to':bob, 'quantity':'0.998 CATS', 'memo':'CATS'}
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
                        'args': {'amount': ee.Save(params,'fee-amount')}
                    }, 
                    {
                        'code': 'c.point', 'event': 'currency'
                    }, 
                    {
                        'code': 'c.point', 'event': 'balance',
                        'args': {'account': alice, 'balance': '0.000 CATS'}
                    }, 
                    {
                        'code': 'c.point', 'event': 'balance',
                        'args': {'account': bob, 'balance': ee.Save(params,'quantity')}
                    })
            }, {
                'receiver': 'c.ctrl', 'code': 'c.ctrl', 'action': 'changepoints',
                'auth': [{'actor': 'c.ctrl', 'permission': 'changepoints'}],
                'args': {'who': alice, 'diff': '-0.999 CATS'}, # transferred and 0.001 (fee)
            }, {
                'receiver': 'c.ctrl', 'code': 'c.ctrl', 'action': 'changepoints',
                'auth': [{'actor': 'c.ctrl', 'permission': 'changepoints'}],
                'args': {'who': bob, 'diff': ee.Load(params,'quantity')},
            },
        ]

        self.eeHelper.waitEvents(
            [ ({'msg_type':'ApplyTrx', 'id':transferTrx}, {'block_num':transferBlock, 'actions':transferTrace, 'except':ee.Missing()}),
            ], transferBlock)
        self.assertRegex(params['fee-amount'], '[0-9]+.[0-9]{3} CATS')
        self.assertEqual(params['fee-amount'], '0.001 CATS') # default fee is 1%
        self.assertRegex(params['quantity'], '[0-9]+.[0-9]{3} CATS')
        self.assertEqual(params['quantity'], '0.998 CATS') # 0.001 subtracted due conversion and 0.001 is fee

if __name__ == '__main__':
    unittest.main()
