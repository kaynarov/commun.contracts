#!/usr/bin/env python3

import argparse
import json
import os
import subprocess
import sys
import time
import tempfile
from deployutils.testnet import *
from deployutils import JsonPrinter, ColoredConsole, Console
from collections import OrderedDict

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
def loadContract(account, contract, *, contracts_dir, warmup_code=None, resultReader=None, **kwargs):
    contract = os.path.relpath(os.path.join(contracts_dir, contract))
    abi_file = os.path.join(contract, os.path.basename(contract) + '.abi')

    with open(abi_file) as json_file:
        abi = json.load(json_file)
        if len(abi['tables']) > 5:
            tables = abi['tables'].copy()
            for x in range(1, len(tables)//3, 1):
                abi['tables'] = tables[:x*3]
                with tempfile.NamedTemporaryFile(mode='w') as f:
                    json.dump(abi, f)
                    f.flush()
                    result = cleos('set abi {account} {abi}'.format(account=account, abi=os.path.relpath(f.name)), **kwargs)
                    if resultReader: resultReader(result, 'preabi{}'.format(x))
            result = cleos('set abi {account} {abi}'.format(account=account, abi=abi_file), **kwargs)
            if resultReader: resultReader(result, 'abi')
        if not warmup_code is None:
            for (i,code) in enumerate(warmup_code):
                while os.path.basename(code) == '':
                    code = os.path.dirname(code)
                code_file = os.path.relpath(os.path.join(contracts_dir, code, os.path.basename(code) + '.wasm'))
                result = cleos('set code {account} {code_file}'.format(account=account, code_file=code_file), **kwargs)
                if resultReader: resultReader(result, 'precode{}'.format(i+1))
        result = cleos('set contract {account} {contract_dir}'.format(account=account, contract_dir=contract), **kwargs)
        if resultReader: resultReader(result, 'contract')


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
    r = requests.post(params['nodeos_url']+'/v1/chain/get_table_rows',  json={
            "json":True, "limit":1, "index":"action", "encode_type":"dec", "reverse":False,"show_payer":False,
            "code":"cyber", "scope":"", "table":"permlink",
            "lower_bound":{"account":account,"required_permission":"","code":code,"message_type":action},
            "upper_bound":None})
    if r.status_code != 200: raise RequestException(r)
    result = r.json()
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

class DeployChanges:
    def __init__(self, *, skip=None, expiration=None):
        self.diffCount = 0
        self.expiration = expiration
        self.skip = set([int(a) for a in skip.split(',')]) if skip else set()
        self.trxs = OrderedDict()

    def difference(self, message):
        diffId = self.diffCount
        skipAction = diffId in self.skip
        act = '-' if skipAction else '+'
        self.diffCount += 1
        print("{act}[{id}] {message}".format(id=diffId, act=act, message=message), file=sys.stderr)
        return self.Difference(self, diffId, skipAction)

    def addAction(self, trxName, contract, action, actors, args):
        if trxName not in self.trxs:
            self.trxs[trxName] = Trx(expiration=self.expiration)
            self.trxs[trxName].provided = set()
        trx = self.trxs[trxName]
        def addProviderAction(trx, account):
            if account != 'c' and account not in trx.provided:
                trx.provided.add(account)
                trx.addAction('cyber', 'providebw', 'c', {'provider':'c', 'account': account})
        actorsAuth = parseAuthority(actors)
        if isinstance(actorsAuth, list):
            for auth in acotrsAuth: addProviderAction(trx, auth['actor'])
        else: addProviderAction(trx, actorsAuth['actor'])
        trx.addAction(contract, action, actors, args)
        return trx.trx['actions'][-1]

    def addTrx(self, trxName, trx):
        class TrxData:
            def __init__(self, trx): self.trx = trx
            def getTrx(self): return self.trx
        if trxName in self.trxs.keys(): raise BaseException("Transaction already exists")
        self.trxs[trxName] = TrxData(trx)

    def fillActionArgs(self):
        for trx in self.trxs.values():
            for action in trx.getTrx()['actions']:
                if 'args' in action: continue
                try: action['args'] = unpackActionData(action['account'], action['name'], action['data'])
                except: pass

    def printTrx(self):
        jsonPrinter = JsonPrinter(ColoredConsole())
        trxItems = initTrxItems()
        for (trxName,trx) in self.trxs.items():
            print("Trx '{name}':\n{trx}".format(name=trxName, trx=jsonPrinter.format(trxItems, trx.getTrx())))

    def saveTrx(self, path):
        trxId = 0
        jsonPrinter = JsonPrinter(Console(), maxstrlength=None)
        trxItems = initTrxItems()
        for (trxName, trx) in self.trxs.items():
            with open(os.path.join(path, '{:0>3}-{}.trx'.format(trxId, trxName)), 'wt') as f:
                f.write(jsonPrinter.format(trxItems, trx.getTrx()))
            trxId += 1

    class Difference:
        def __init__(self, changes, diffId, skip):
            self.changes = changes
            self.diffId = diffId
            self.skip = skip
        
        def __enter__(self):
            return self

        def __exit__(self, exc_type, exc_val, exc_tb):
            return False

        def _addTrx(self, trxName, trx):
            if self.skip: return
            for action in trx['actions']:
                action['id'] = '[{id}]'.format(id=self.diffId)
            self.changes.addTrx(trxName, trx)

        def _addAction(self, trxName, code, action, actor, args):
            if self.skip: return
            action = self.changes.addAction(trxName, code, action, actor, args)
            action['id'] = '[{id}]'.format(id=self.diffId)

        def addCreateAccount(self, account):
            self._addAction('accounts', 'cyber', 'newaccount', 'c@active', {
                    'creator': 'c',
                    'name': account,
                    'owner': createAuthority([], ['c@owner']),
                    'active': createAuthority([], ['c@active'])
                })
    
        def addUpdateAuth(self, account, perm):
            self._addAction('auth-{}'.format(account), 'cyber', 'updateauth', account, {
                    'account': account,
                    'permission': perm.name,
                    'parent': perm.parent,
                    'auth': createAuthority(perm.keys, perm.accounts)
                })
    
        def addLinkAuth(self, account, perm, link):
            (code, action) = link.split(':', 2)
            self._addAction('auth-{}'.format(account), 'cyber', 'linkauth', account, {
                    'account': account,
                    'code': code or account,
                    'type': action,
                    'requirement': perm
                })

        @staticmethod
        def extractTrx(info):
            trx = json.loads(info[info.find('{'):])
            return trx

        def addLoadContractTrxs(self, account, contract, warmup_code):
            def trxReader(info, desc):
                self._addTrx('code-{account}-{desc}'.format(desc=desc, account=account), self.extractTrx(info))
            loadContract(account, contract, additional=' --skip-sign --dont-broadcast',
                    contracts_dir=args.contracts_dir, warmup_code=warmup_code,
                    providebw=account+'/c', resultReader=trxReader)

def checkCommunAccounts(changes):
    for accInfo in communAccounts:
        oldAccount = getAccount(accInfo.name)
        if oldAccount is None:
            with changes.difference("Missing account {acc}".format(acc=accInfo.name)) as diff:
                diff.addCreateAccount(accInfo.name)
                for perm in accInfo.permissions:
                    diff.addUpdateAuth(accInfo.name, perm)
                    for link in perm.links:
                        diff.addLinkAuth(accInfo.name, perm.name, link)
                if accInfo.contract:
                    diff.addLoadContractTrxs(accInfo.name, accInfo.contract, accInfo.warmup_code)
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
                with changes.difference("Missing '{pname}' permission for {acc}".format(acc=accInfo.name, pname=permInfo.name)) as diff:
                    diff.addUpdateAuth(accInfo.name, permInfo)
                    for link in perm.links:
                        diff.addLinkAuth(accInfo.name, perm.name, link)
                continue

            actualAuth = oldAccount['permissions'][permInfo.name]['required_auth']
            expectAuth = createAuthority(permInfo.keys, permInfo.accounts)
            if actualAuth != expectAuth:
                with changes.difference("Invalid '{pname}' permission for {acc}:\n" \
                        "    expect: {expect}\n" \
                        "    actual: {actual}".format(acc=accInfo.name, pname=permInfo.name,
                        expect=json.dumps(expectAuth,sort_keys=True),
                        actual=json.dumps(actualAuth,sort_keys=True))) as diff:
                    diff.addUpdateAuth(accInfo.name, permInfo)

            for link in permInfo.links:
                (code, action) = link.split(':',2)
                if not code: code = accInfo.name
                actualLink = getPermLink(accInfo.name, code, action, output=False)
                if actualLink is None:
                    with changes.difference("Missing permlink {code}:{action} for {acc}".format(acc=accInfo.name, code=code, action=action)) as diff:
                        diff.addLinkAuth(accInfo.name, perm.name, link)
                elif actualLink['required_permission'] != permInfo.name:
                    with changes.difference("Invalid permlink {code}:{action} -> {chainPerm} for {acc}: {perm}".format(
                            acc=accInfo.name, code=code, action=action,
                            chainPerm=actualLink['required_permission'], perm=permInfo.name)) as diff:
                        diff.addLinkAuth(accInfo.name, perm.name, link)

        if accInfo.contract:
            filenameCode = os.path.join(args.contracts_dir, accInfo.contract, os.path.basename(accInfo.contract) + '.wasm')
            expectCodeHash = Code.getHashFromFile(filenameCode)
            filenameAbi = os.path.join(args.contracts_dir, accInfo.contract, os.path.basename(accInfo.contract) + '.abi')
            expectAbiHash = ABI.getHashFromFile(filenameAbi)

            actualCodeHash = Code.getHashFromChain(params['nodeos_url'], accInfo.name)
            actualAbiHash = ABI.getHashFromChain(params['nodeos_url'], accInfo.name)

            if actualCodeHash is None or actualAbiHash is None:
                with changes.difference('Code or ABI missing for {acc}'.format(acc=accInfo.name)) as diff:
                    diff.addLoadContractTrxs(accInfo.name, accInfo.contract, accInfo.warmup_code)
            elif actualCodeHash != expectCodeHash or actualAbiHash != expectAbiHash:
                with changes.difference('Code or ABI different for {acc}:\n'
                        '   expect: {expectCode} {expectAbi}\n'
                        '   actual: {actualCode} {actualAbi}'.format(acc=accInfo.name, 
                        actualCode=actualCodeHash, expectCode=expectCodeHash,
                        actualAbi=actualAbiHash, expectAbi=expectAbiHash)) as diff:
                    contract = os.path.relpath(os.path.join(args.contracts_dir, accInfo.contract))
                    result = cleos('set contract {account} {path}'.format(account=accInfo.name, path=contract),
                            additional=' --dont-broadcast --skip-sign', providebw=accInfo.name+'/c', output=False)
                    diff._addTrx('code-{}-update'.format(accInfo.name), diff.extractTrx(result))

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

parser.add_argument('--private-key', metavar='', help="Commun Private Key", default='5KekbiEKS4bwNptEtSawUygRb5sQ33P6EUZ6c4k4rEyQg7sARqW', dest="private_key")
parser.add_argument('--programs-dir', metavar='', help="Programs directory for cleos, nodeos, keosd", default='/mnt/working/eos-debug.git/build/programs');
parser.add_argument('--keosd', metavar='', help="Path to keosd binary (default in programs-dir)", default='keosd/keosd')
parser.add_argument('--contracts-dir', metavar='', help="Path to contracts directory", default=os.environ.get("COMMUN_CONTRACTS", '../../build/'))
parser.add_argument('--wallet-dir', metavar='', help="Path to wallet directory", default='./wallet/')
parser.add_argument('--log-path', metavar='', help="Path to log file", default='./output.log')

parser.add_argument('--check', help="Check dApp with current version", action='store_true')
parser.add_argument('--output', help="Directory for created transactions for update dApp", default=None)
parser.add_argument('--expiration', help='Expiration time for generated transactions (sec)', default=3600)
parser.add_argument('--skip', help='Skip actions in update sequence (comma separated list of difference id)', default=None)
parser.add_argument('--layout', help='Deploy layout', default=None)

args = parser.parse_args()

if (parser.get_default('keosd') == args.keosd):
    args.keosd = args.programs_dir + '/' + args.keosd

logFile = open(args.log_path, 'a')
logFile.write('\n\n' + '*' * 80 + '\n\n\n')

layouts = {
    # links `chaind_id` with known layout
    '591c8aa5cade588b1ce045d26e5f2a162c52486262bd2d7abcb7fa18247e17ec': 'mainnet'
}
chainInfo = json.loads(cleos('get info', output=False))
deployLayout = args.layout or layouts.get(chainInfo['chain_id'], 'testnet')
print('Deploy layout: {layout}'.format(layout=deployLayout))

communAccounts = parseAccountInfo(_communAccounts, deployLayout)

if (args.check):
    if args.output and os.path.exists(args.output):
        print("Output directory '{}' already exists".format(args.output), file=sys.stderr)
        exit(1)

    changes = DeployChanges(expiration=args.expiration, skip=args.skip)
    checkCommunAccounts(changes)
    if getBalance('cyber.null', 'CMN', output=False) is None:
        with changes.difference("Missing 'CMN' balance for 'cyber.null'") as diff:
            diff._addAction('other', 'cyber.token', 'open', 'c', {
                    "owner": "cyber.null",
                    "symbol": Symbol(4, 'CMN'),
                    "ram_payer": "c"})

    if args.output:
        os.makedirs(args.output)
        changes.fillActionArgs()
        changes.saveTrx(args.output)
        changes.printTrx()
        exit(0)
    else:
        exit(0 if changes.diffCount==0 else 1)

startWallet()
importKeys()
createCommunAccounts()
installContracts()
configureCommun()
OpenNullCMN()
