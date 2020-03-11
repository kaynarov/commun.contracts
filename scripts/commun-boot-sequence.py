#!/usr/bin/env python3

import argparse
import json
import os
import subprocess
import sys
import time
import tempfile
from deployutils.testnet import *

class Struct(object): pass

args = None
logFile = None

unlockTimeout = 999999999

_communAccounts = [
    # name           contract                warmup_code     permissions (layout, name, keys, accounts, links)
    #('c',        None),    # c - owner for CMN. Must be created outside of this script!
    # Account for commun.com site
    ('c.com',     None,                   None, [
            ('mainnet', 'owner',     ['GLS5UuLvruMSU6cvNX2kmBpc2c8rwBA4rbcMJ5cb1boNdPvuWoebk'], [], []),
            ('mainnet', 'active',    ['GLS7WtstrTkLJCCfwzZdQT4vbuQ16fkhW5KiNE843WtdeLfXwuDv9'], [], []),
            ('mainnet', 'c.com',     ['GLS6rCjhCGo4aEjWcKPerpnpAAB7moL5Sr9UQPFM3V3arx73SU36F'], [], ['cyber:newaccount']),
            ('testnet', 'c.com',     ['GLS5a2eDuRETEg7uy8eHbiCqGZM3wnh2pLjiXrFduLWBKVZKCkB62'], [], ['cyber:newaccount'])
        ]),

    ('c',         None,                   None, [
            (None,      'lead.smajor',  [], ['c.ctrl@cyber.code'], []),
            (None,      'lead.major',   [], ['c.ctrl@cyber.code'], []),
            (None,      'lead.minor',   [], ['c.ctrl@cyber.code'], []),
            (None,      'lead.recover', [], ['c@active'], []), # empty auth, active is just filler
            (None,      'clients',      [], ['c.com@c.com'], ['cyber.domain:newusername']),
            (None,      'providebw',    [], ['c@clients'], ['cyber:providebw', 'c.point:open']),
        ]),
    ('c.issuer',  None,                   None, [
            ('testnet', 'issue',        [], ['c@clients'], ['cyber.token:issue', 'cyber.token:transfer']),    # Only for testnet purposes
        ]),

    ('c.point',   'commun.point',         None, [
            (None,      'active',       [], ['c@active', 'c.point@cyber.code'], []),
            (None,      'issueperm',    [], ['c.emit@cyber.code'], [':issue']),
            (None,      'clients',      [], ['c@clients'], [':create']),
        ]),
    ('c.ctrl',    'commun.ctrl',          None, [
            (None,      'changepoints', [], ['c.point@cyber.code'], [':changepoints']),
            (None,      'init',         [], ['c.list@cyber.code'], [':init']),
            (None,      'clients',      [], ['c@clients'], [':emit']),
            (None,      'transferperm', [], ['c.ctrl@cyber.code'], ['c.point:transfer']),
        ]),
    ('c.emit',    'commun.emit',          None, [
            (None,      'rewardperm',   [], ['c.ctrl@cyber.code', 'c.gallery@cyber.code'], [':issuereward']),
            (None,      'init',         [], ['c.list@cyber.code'], [':init']),
        ]),
    ('c.list',    'commun.list',          None,                [
            (None,      'clients',      [], ['c@clients'], [':create',':setsysparams',':follow',':unfollow',':hide',':unhide']),
        ]),
    ('c.gallery', 'commun.publication',   ['commun.gallery'],  [
            (None,      'clients',      [], ['c@clients'], [':create',':update',':remove',':settags',':report',':upvote',':downvote',':unvote',':reblog',':erasereblog',':emit']),
            (None,      'init',         [], ['c.list@cyber.code'], [':init']),
            (None,      'transferperm', [], ['c.gallery@cyber.code'], ['c.point:transfer']),
        ]),
    ('c.social',  'commun.social',        None,                [
            (None,      'clients',      [], ['c@clients'], [':pin',':unpin',':block',':unblock',':updatemeta',':deletemeta']),
        ]),
    ('c.recover', 'commun.recover',       None,                [
            (None,      'recover',      ['GLS71iAcPXAqzruvh1EFu28S89Cy8GoYNQXKSQ6UuaBYFuB7usyCB'], [], ['c.recover:recover']),
        ]),
]

def createPermInfo(name, parent, keys, accounts, links):
    perm = Struct()
    (perm.name, perm.parent, perm.keys, perm.accounts, perm.links) = (name, parent, keys, accounts, links)
    return perm

def parseAccountInfo(accountInfo, deployLayout):
    layoutAccounts = []
    for (name, contract, warmup_code, permissions) in accountInfo:
        acc = Struct()
        perms = []
        for (layout, pname, keys, accounts, links) in permissions:
            if layout is None or layout == deployLayout:
                perms.append(createPermInfo(pname, "owner" if pname == "active" else "active", keys, accounts, links))
        (acc.name, acc.contract, acc.warmup_code, acc.permissions) = (name, contract, warmup_code, perms)
        layoutAccounts.append(acc)
    return layoutAccounts

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

def getPermLink(account, code, action, **kwargs):
    result = json.loads(cleos("get table --limit 1 --index action -L {index} cyber '' permlink".format(
            index=jsonArg({"account":account, "code":code, "message_type":action})), **kwargs))
    if len(result['rows']) == 0: return None
    link = result['rows'][0]
    if link['account'] != account or link['code'] != code or link['message_type'] != action:
        return None
    return link

def getBalance(account, symbol, **kwargs):
    result = json.loads(cleos("get table --limit 1 -L {index} cyber.token {acc} accounts".format(
            index=jsonArg({"balance":{"_sym":symbol}}), acc=account), **kwargs))
    if len(result['rows']) == 0: return None
    balance = result['rows'][0]
    if balance['balance'].endswith(' '+symbol): return balance
    else: None

def getAbi(account, **kwargs):
    try:
        return cleos('get abi {acc}'.format(acc=account), **kwargs)
    except CleosException as err:
        if err.output.find('Failed with error: Key Not Found') != -1:
            return None
        else: raise

def compareAbiFiles(first, second):
    try:
        subprocess.check_output(['/mnt/working/cyberway.cdt.git/build/bin/cyberway-abidiff', first, second], universal_newlines=True)
        return True
    except subprocess.CalledProcessError as err:
        if err.returncode == 1:
            return False
        else: raise

class DeployChanges:
    def __init__(self):
        self.diffCount = 0

    def add(self, message):
        print("[{id}] {message}".format(id=self.diffCount, message=message), file=sys.stderr)
        self.diffCount += 1

def checkCommunAccounts(changes):
    for accInfo in communAccounts:
        oldAccount = getAccount(accInfo.name, output=args.verbose)
        if oldAccount is None:
            changes.add("Missing account {acc}".format(acc=accInfo.name))
            continue

        permissions = {}
        for perm in accInfo.permissions: permissions[perm.name] = perm

        if accInfo.name != 'c':
            if 'active' not in permissions: 
                permissions['active'] = createPermInfo('active', 'owner', [], ['c@active'], [])
            if 'owner' not in permissions:
                permissions['owner'] = createPermInfo('owner', '', [], ['c@owner'], [])

        for permInfo in permissions.values():
            if permInfo.name not in oldAccount['permissions']:
                changes.add("Missing '{pname}' permission for {acc}".format(acc=accInfo.name, pname=permInfo.name))
                continue

            actualPerm = oldAccount['permissions'][permInfo.name]
            expectPerm = createAuthority(permInfo.keys, permInfo.accounts)
            if actualPerm['required_auth'] != expectPerm:
                changes.add("Invalid '{pname}' permission for {acc}:\n" \
                      "    expect: {expect}\n" \
                      "    actual: {actual}".format(acc=accInfo.name, pname=permInfo.name,
                      expect=json.dumps(expectPerm,sort_keys=True), actual=json.dumps(actualPerm['required_auth'],sort_keys=True)))

            for link in permInfo.links:
                (code, action) = link.split(':',2)
                if not code: code = accInfo.name
                actualLink = getPermLink(accInfo.name, code, action, output=args.verbose)
                if actualLink is None:
                    changes.add("Missing permlink {code}:{action} for {acc}".format(acc=accInfo.name, code=code, action=action))
                elif actualLink['required_permission'] != permInfo.name:
                    changes.add("Invalid permlink {code}:{action} -> {chainPerm} for {acc}: {perm}".format(
                            acc=accInfo.name, code=code, action=action, chainPerm=actualLink['required_permission'], perm=permInfo.name))

        if accInfo.contract:
            actualCode = cleos('get code {acc}'.format(acc=accInfo.name), output=args.verbose).split(' ')[2].rstrip('\n')
            if actualCode == 64*'0':
                changes.add('Code missing for {acc}'.format(acc=accInfo.name))
            else:
                filenameCode = os.path.join(args.contracts_dir, accInfo.contract, os.path.basename(accInfo.contract) + '.wasm')
                expectCode = subprocess.check_output(['sha256sum', filenameCode], universal_newlines=True).split(' ')[0]
    
                if actualCode != expectCode:
                    changes.add('Code different for {acc}: {actual} {expect}'.format(acc=accInfo.name, actual=actualCode, expect=expectCode))

            actualAbiData = getAbi(accInfo.name, output=args.verbose)
            if actualAbiData is None:
                changes.add('ABI missing for {acc}'.format(acc=accInfo.name))
            else:
                expectAbi = os.path.join(args.contracts_dir, accInfo.contract, os.path.basename(accInfo.contract) + '.abi')
                with tempfile.NamedTemporaryFile(mode='wt', delete=False) as f:
                    f.write(actualAbiData)
                    f.flush()
                    if False == compareAbiFiles(f.name, expectAbi):
                        changes.add('ABI different for {acc}: {actual} {expect}'.format(acc=accInfo.name, actual=f.name, expect=expectAbi))

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
    if deployLayout != 'mainnet':
        pushAction('c.list', 'setappparams', 'c.list', {
                "leaders_num":5
            }, providebw='c.list/c')

    c = getAccount('c')
    updateAuth('c', 'active', 'owner', [], ['c@lead.smajor', 'c@lead.recover', 'c@owner'])

def OpenNullCMN():
    pushAction('cyber.token', 'open', 'c', {
        "owner": "cyber.null",
        "symbol": Symbol(4, 'CMN'),
        "ram_payer": "c"
    })

# -------------------- Argument parser ----------------------------------------
# Command Line Arguments

parser = argparse.ArgumentParser()

parser.add_argument('--private-key', metavar='', help="Golos Private Key", default='5KekbiEKS4bwNptEtSawUygRb5sQ33P6EUZ6c4k4rEyQg7sARqW', dest="private_key")
parser.add_argument('--programs-dir', metavar='', help="Programs directory for cleos, nodeos, keosd", default='/mnt/working/eos-debug.git/build/programs');
parser.add_argument('--keosd', metavar='', help="Path to keosd binary (default in programs-dir)", default='keosd/keosd')
parser.add_argument('--contracts-dir', metavar='', help="Path to contracts directory", default=os.environ.get("COMMUN_CONTRACTS", '../../build/'))
parser.add_argument('--wallet-dir', metavar='', help="Path to wallet directory", default='./wallet/')
parser.add_argument('--log-path', metavar='', help="Path to log file", default='./output.log')

parser.add_argument('--update', help="Create proposals for update dApp contracts", action='store_true')
parser.add_argument('--check', help="Check dApp contracts with current version", action='store_true')
parser.add_argument('--verbose', help="Verbose cleos actions", action='store_true')

args = parser.parse_args()

if (parser.get_default('keosd') == args.keosd):
    args.keosd = args.programs_dir + '/' + args.keosd

logFile = open(args.log_path, 'a')
logFile.write('\n\n' + '*' * 80 + '\n\n\n')

layouts = {
    # links `chaind_id` with known layout
    '591c8aa5cade588b1ce045d26e5f2a162c52486262bd2d7abcb7fa18247e17ec': 'mainnet'
}
chainInfo = json.loads(cleos('get info', output=args.verbose))
deployLayout = layouts.get(chainInfo['chain_id'], 'testnet')
print('Deploy layout: {layout}'.format(layout=deployLayout))

communAccounts = parseAccountInfo(_communAccounts, deployLayout)

if (args.check):
    changes = DeployChanges()
    checkCommunAccounts(changes)
    if getBalance('cyber.null', 'CMN', output=args.verbose) is None:
        changes.add("Missing 'CMN' balance for 'cyber.null'")

    exit(0 if changes.diffCount==0 else 1)

startWallet()
importKeys()
createCommunAccounts()
installContracts()
configureCommun()
OpenNullCMN()
