#!/usr/bin/env python3

import argparse
import json
import os
import subprocess
import sys
import time
import tempfile
from testnet import *

class Struct(object): pass

args = None
logFile = None

unlockTimeout = 999999999

_communAccounts = [
    # name           contract                warmup_code     permissions (name, keys, accounts, links)
    #('c',        None),    # c - owner for CMN. Must be created outside of this script!
    # Account for commun.com site
    ('c.com',     None,                   None, [
            ('c.com',     ['GLS5a2eDuRETEg7uy8eHbiCqGZM3wnh2pLjiXrFduLWBKVZKCkB62'], [], ['cyber:newaccount'])
        ]),

    ('c',         None,                   None, [
            ('lead.smajor',  [], ['c.ctrl@cyber.code'], []),
            ('lead.major',   [], ['c.ctrl@cyber.code'], []),
            ('lead.minor',   [], ['c.ctrl@cyber.code'], []),
            ('lead.recover', [], ['c@active'], []), # empty auth, active is just filler
            ('clients',      [], ['c.com@c.com'], ['cyber.domain:newusername']),
            ('providebw',    [], ['c@clients'], ['cyber:providebw', 'c.point:open']),
        ]),
    ('c.issuer',  None,                   None, [
            ('issue',        [], ['c@clients'], ['cyber.token:issue', 'cyber.token:transfer']),    # Only for testnet purposes
        ]),

    ('c.point',   'commun.point',         None, [
            ('active',       [], ['c@active', 'c.point@cyber.code'], []),
            ('issueperm',    [], ['c.emit@cyber.code'], [':issue']),
            ('clients',      [], ['c@clients'], [':create']),
        ]),
    ('c.ctrl',    'commun.ctrl',          None, [
            ('changepoints', [], ['c.point@cyber.code'], [':changepoints']),
            ('init',         [], ['c.list@cyber.code'], [':init']),
            ('clients',      [], ['c@clients'], [':emit']),
            ('transferperm', [], ['c.ctrl@cyber.code'], ['c.point:transfer']),
        ]),
    ('c.emit',    'commun.emit',          None, [
            ('rewardperm',   [], ['c.ctrl@cyber.code', 'c.gallery@cyber.code'], [':issuereward']),
            ('init',         [], ['c.list@cyber.code'], [':init']),
        ]),
    ('c.list',    'commun.list',          None,                [
            ('clients',      [], ['c@clients'], [':create',':setsysparams',':follow',':unfollow',':hide',':unhide']),
        ]),
    ('c.gallery', 'commun.publication',   ['commun.gallery'],  [
            ('clients',      [], ['c@clients'], [':create',':update',':remove',':settags',':report',':upvote',':downvote',':unvote',':reblog',':erasereblog',':emit']),
            ('init',         [], ['c.list@cyber.code'], [':init']),
            ('transferperm', [], ['c.gallery@cyber.code'], ['c.point:transfer']),
        ]),
    ('c.social',  'commun.social',        None,                [
            ('clients',      [], ['c@clients'], [':pin',':unpin',':block',':unblock',':updatemeta',':deletemeta']),
        ]),
]

communAccounts = []
for (name, contract, warmup_code, permissions) in _communAccounts:
    acc = Struct()
    perms = []
    for (pname, keys, accounts, links) in permissions:
        perm = Struct()
        parent = "owner" if pname == "active" else "active"
        (perm.name, perm.parent, perm.keys, perm.accounts, perm.links) = (pname, parent, keys, accounts, links)
        perms.append(perm)
    (acc.name, acc.contract, acc.warmup_code, acc.permissions) = (name, contract, warmup_code, perms)
    communAccounts.append(acc)

# In case of large amount of tables transaction `set contract` failed due
# timeout (`Transaction took too long`). In this case we split ABI-file
# tables section in small portions and iteratively load ABI.
def loadContract(account, contract, *, contracts_dir, providebw=None, keys=None, retry=None, warmup_code=None):
    contract = os.path.join(contracts_dir, contract)
    while os.path.basename(contract) == '':
        contract = os.path.dirname(contract)
    abi_file = os.path.join(contract, os.path.basename(contract) + '.abi')

    with open(abi_file) as json_file:
        abi = json.load(json_file)
        if len(abi['tables']) > 5:
            tables = abi['tables'].copy()
            for x in range(3, len(tables), 3):
                abi['tables'] = tables[:x]
                with tempfile.NamedTemporaryFile(mode='w') as f:
                    json.dump(abi, f)
                    f.flush()
                    cleos('set abi {account} {abi}'.format(account=account, abi=f.name),
                            providebw=providebw, keys=keys, retry=retry)
            cleos('set abi {account} {abi}'.format(account=account, abi=abi_file),
                    providebw=providebw, keys=keys, retry=retry)
        if not warmup_code is None:
            for code in warmup_code:
                while os.path.basename(code) == '':
                    code = os.path.dirname(code)
                code_file = os.path.join(contracts_dir, code, os.path.basename(code) + '.wasm')
                cleos('set code {account} {code_file}'.format(account=account, code_file=code_file),
                        providebw=providebw, keys=keys, retry=retry)
        return cleos('set contract {account} {contract_dir}'.format(account=account, contract_dir=contract),
                providebw=providebw, keys=keys, retry=retry)


def run(args):
    print('commun-boot-sequence.py:', args)
    logFile.write(args + '\n')
    if subprocess.call(args, shell=True):
        print('commun-boot-sequence.py: exiting because of error')
        sys.exit(1)

def background(args):
    print('commun-boot-sequence.py:', args)
    logFile.write(args + '\n')
    return subprocess.Popen(args, shell=True)

def sleep(t):
    print('sleep', t, '...')
    time.sleep(t)
    print('resume')

# --------------------- EOSIO functions ---------------------------------------
def startWallet():
    run('rm -rf ' + os.path.abspath(args.wallet_dir))
    run('mkdir -p ' + os.path.abspath(args.wallet_dir))
    background(args.keosd + ' --unlock-timeout %d --http-server-address 127.0.0.1:6667 --unix-socket-path \'\' --wallet-dir %s' % (unlockTimeout, os.path.abspath(args.wallet_dir)))
    sleep(.4)

def importKeys():
    cleos('wallet create -n "commun" --to-console')
    cleos('wallet import -n "commun" --private-key ' + args.private_key)

# -------------------- Commun functions ---------------------------------------

def createCommunAccounts():
    for acc in communAccounts:
        if not (acc.name == 'c' or acc.name == 'c.issuer'):
            createAccount('c', acc.name, 'c@owner', 'c@active')

    for acc in communAccounts:
        providebw = acc.name+'/c' if acc.name!='c' else ''
        for perm in acc.permissions:
            updateAuth(acc.name, perm.name, perm.parent, perm.keys, perm.accounts, providebw=providebw)
            for link in perm.links:
                (code, action) = link.split(':',2)
                linkAuth(acc.name, code if code else acc.name, action, perm.name, providebw=providebw)

def installContracts():
    for acc in communAccounts:
        if (acc.contract != None):
            loadContract(acc.name, acc.contract, providebw=acc.name+'/c', retry=3, contracts_dir=args.contracts_dir, warmup_code=acc.warmup_code)

def configureCommun():
    # Only for testnet purposes
    pushAction('c.list', 'setappparams', 'c.list', {
            "leaders_num":5
        }, providebw='c.list/c')

    c = getAccount('c')
    updateAuth('c', 'active', 'owner', [], ['c@lead.smajor', 'c@lead.recover'])

# -------------------- Argument parser ----------------------------------------
# Command Line Arguments

parser = argparse.ArgumentParser()

parser.add_argument('--private-key', metavar='', help="Golos Private Key", default='5KekbiEKS4bwNptEtSawUygRb5sQ33P6EUZ6c4k4rEyQg7sARqW', dest="private_key")
parser.add_argument('--programs-dir', metavar='', help="Programs directory for cleos, nodeos, keosd", default='/mnt/working/eos-debug.git/build/programs');
parser.add_argument('--keosd', metavar='', help="Path to keosd binary (default in programs-dir)", default='keosd/keosd')
parser.add_argument('--contracts-dir', metavar='', help="Path to contracts directory", default='../../build/')
parser.add_argument('--wallet-dir', metavar='', help="Path to wallet directory", default='./wallet/')
parser.add_argument('--log-path', metavar='', help="Path to log file", default='./output.log')

args = parser.parse_args()

if (parser.get_default('keosd') == args.keosd):
    args.keosd = args.programs_dir + '/' + args.keosd

logFile = open(args.log_path, 'a')
logFile.write('\n\n' + '*' * 80 + '\n\n\n')

startWallet()
importKeys()
createCommunAccounts()
installContracts()
configureCommun()
