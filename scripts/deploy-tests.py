#!/usr/bin/python3

import unittest
import json
import os
import re
import random
import time
import testnet
import community

techKey   ='5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3'
communKey ='5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3'

def wait(timeout):
    while timeout > 0:
        w = 3 if timeout > 3 else timeout
        print('\r                    \rWait %d sec' % timeout, flush=True, end='')
        timeout -= w
        time.sleep(w)

def randomUsername():
    letters = 'abcdefghijklmnopqrstuvwxyz0123456789'
    return ''.join(random.choice(letters) for i in range(16))

def randomPermlink():
    letters = "abcdefghijklmnopqrstuvwxyz0123456789-"
    return ''.join(random.choice(letters) for i in range(128))

def randomText(length):
    letters = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+-=;:,.<>/?\\~`"
    return ''.join(random.choice(letters) for i in range(length))

def createKey():
    data = testnet.cleos("create key --to-console")
    m = re.search('Private key: ([a-zA-Z0-9]+)\nPublic key: ([a-zA-Z0-9]+)', data)
    return (m.group(1), m.group(2))

def importPrivateKey(private):
    testnet.cleos("wallet import --name test --private-key %s" % private)

def createRandomAccount(key, *, creator='tech', providebw=None, keys=None):
    letters = "abcdefghijklmnopqrstuvwxyz12345"
    while True:
        name = ''.join(random.choice(letters) for i in range(12))
        try:
            getAccount(name)
        except:
            break
    testnet.createAccount(creator, name, key, providebw=providebw, keys=keys)
    return name

def getAccount(account):
    return json.loads(testnet.cleos('get account %s -j' % account))

def getResourceUsage(account):
    info = getAccount(account)
    return {
        'cpu': info['cpu_limit']['used'],
        'net': info['net_limit']['used'],
        'ram': info['ram_limit']['used'],
        'storage': info['storage_limit']['used']
    }

def buyCommunityPoints(owner, quantity, community, ownerKey):
    testnet.pushAction('cyber.token', 'issue', 'comn', {
            'to':owner,
            'quantity':quantity,
            'memo':'issue for '+owner
        }, keys=communKey)
    testnet.pushAction('cyber.token', 'transfer', owner, {
            'from':owner,
            'to':'comn.point',
            'quantity':quantity,
            'memo':community
        }, providebw=owner+'/tech', keys=[ownerKey, techKey])
    


class TestCommunity(unittest.TestCase):
    
    def test_createPost(self):
        (private, public) = createKey()
        author = createRandomAccount(public, keys=techKey)
        community.openBalance(author, '3,CATS', 'tech', keys=techKey)
        buyCommunityPoints(author, '1000.0000 COMMUN', 'CATS', private)

        (private2, public2) = createKey()
        voter = createRandomAccount(public2, keys=techKey)
        community.openBalance(voter, '3,CATS', 'tech', keys=techKey)
        buyCommunityPoints(voter, '10.0000 COMMUN', 'CATS', private2)

        permlink = randomPermlink()
        header = randomText(128)
        body = randomText(1024)
        community.createPost('CATS', author, permlink, 'cats', header, body, providebw=author+'/tech', keys=[private, techKey])

        community.upvotePost('CATS', voter, author, permlink, 10000, providebw=voter+'/tech', keys=[private2, techKey])



if __name__ == '__main__':
    unittest.main()
