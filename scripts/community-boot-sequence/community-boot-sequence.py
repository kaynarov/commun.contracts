#!/usr/bin/env python3

import argparse
import json
import numpy
import os
import random
import re
import subprocess
import sys
import time

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from config import golos_curation_func_str
from utils import fixp_max

class Struct(object): pass

args = None
logFile = None

unlockTimeout = 999999999

_communAccounts = [
    # name           contract
    ('c.ctrl',    'golos.ctrl'),
    ('c.emit',    'golos.emit'),
    ('c.publish', 'commun.publication'),
    ('c.social',  'commun.social'),
    ('c.charge',  'golos.charge'),
    ('c.ref',     'golos.referral'),
    ('c.worker',   None),
    ('c.issuer',   None),
]

communAccounts = []
for (name, contract) in _communAccounts:
    acc = Struct()
    (acc.name, acc.contract) = (name, contract)
    communAccounts.append(acc)

def jsonArg(a):
    return " '" + json.dumps(a) + "' "

def cleos(arguments):
    command=args.cleos + arguments
    print('community-boot-sequence.py:', command)
    logFile.write(command + '\n')
    if subprocess.call(command, shell=True):
        return False
    return True

def run(args):
    print('community-boot-sequence.py:', args)
    logFile.write(args + '\n')
    if subprocess.call(args, shell=True):
        print('community-boot-sequence.py: exiting because of error')
        sys.exit(1)

def retry(args):
    count = 5
    while count:
        count = count-1
        print('community-boot-sequence.py:', args)
        logFile.write(args + '\n')
        if subprocess.call(args, shell=True):
            print('*** Retry: ', count)
            sleep(0.5)
        else:
            return True
    print('community-boot-sequence.py: exiting because of error')
    sys.exit(1)

def background(args):
    print('community-boot-sequence.py:', args)
    logFile.write(args + '\n')
    return subprocess.Popen(args, shell=True)

def getOutput(args):
    print('community-boot-sequence.py:', args)
    logFile.write(args + '\n')
    proc = subprocess.Popen(args, shell=True, stdout=subprocess.PIPE)
    return proc.communicate()[0].decode('utf-8')

def getJsonOutput(args):
    print('community-boot-sequence.py:', args)
    logFile.write(args + '\n')
    proc = subprocess.Popen(args, shell=True, stdout=subprocess.PIPE)
    return json.loads(proc.communicate()[0])

def sleep(t):
    print('sleep', t, '...')
    time.sleep(t)
    print('resume')

def intToCurrency(value, precision, symbol):
    devider=10**precision
    return '%d.%0*d %s' % (value // devider, precision, value % devider, symbol)

def intToToken(value):
    return intToCurrency(value, args.token_precision, args.symbol)

def intToVesting(value):
    return intToCurrency(value, args.vesting_precision, args.symbol)

def intToCommun(value):
    return intToCurrency(value, 4, 'COMMUN')

# --------------------- EOSIO functions ---------------------------------------

def transfer(sender, recipient, amount, memo=""):
    retry(args.cleos + 'push action cyber.token transfer ' +
        jsonArg([sender, recipient, amount, memo]) + '-p %s'%sender)

def bancorTransfer(sender, recipient, amount, memo=""):
    retry(args.cleos + 'push action cmmn.token transfer ' +
        jsonArg([sender, recipient, amount, memo]) + '-p %s'%sender)        

def createAccount(creator, account, key):
    retry(args.cleos + 'create account %s %s %s' % (creator, account, key))

def updateAuth(account, permission, parent, keys, accounts):
    retry(args.cleos + 'push action cyber updateauth' + jsonArg({
        'account': account,
        'permission': permission,
        'parent': parent,
        'auth': createAuthority(keys, accounts)
    }) + '-p ' + account)

def linkAuth(account, code, action, permission):
    retry(args.cleos + 'set action permission %s %s %s %s -p %s'%(account, code, action, permission, account))

def createAuthority(keys, accounts):
    keys.sort()
    accounts.sort()
    keysList = []
    accountsList = []
    for k in keys:
        keysList.extend([{'weight':1,'key':k}])
    for a in accounts:
        d = a.split('@',2)
        if len(d) == 1:
            d.extend(['active'])
        accountsList.extend([{'weight':1,'permission':{'actor':d[0],'permission':d[1]}}])
    return {'threshold': 1, 'keys': keysList, 'accounts': accountsList, 'waits':[]}

# --------------------- COMMUN functions ---------------------------------------

def openCommunBalance(account):
    retry(args.cleos + 'push action cyber.token open' +
        jsonArg([account, '4,COMMUN', account]) + '-p %s'%account)

def openTokenBalance(account):
    retry(args.cleos + 'push action cmmn.token open' +
        jsonArg([account, args.token, account]) + '-p %s'%account)

def openVestingBalance(account):
    retry(args.cleos + 'push action cmmn.vesting open' +
        jsonArg([account, args.vesting, account]) + '-p %s'%account)

def issueCommunToken(account, amount, memo='memo'):
    retry(args.cleos + 'push action cyber.token issue ' + jsonArg([account, amount, memo]) + ' -p cmmn.owner')

def buyToken(account, amount):
    issueCommunToken(account, intToCommun(amount * 10000), "issue commun token")
    transfer(account, 'cmmn.token', intToCommun(amount * 10000), args.symbol)

def buyVesting(account, amount):
    bancorTransfer(account, 'cmmn.vesting', intToToken(amount * (10 ** args.token_precision)), 'send to: ' + account)   # buy vesting

def registerWitness(ctrl, witness):
    retry(args.cleos + 'push action ' + ctrl + ' regwitness' + jsonArg({
        'witness': witness,
        'url': 'http://%s.witnesses.golos.io' % witness
    }) + '-p %s'%witness)

def voteWitness(ctrl, voter, witness):
    retry(args.cleos + ' push action ' + ctrl + ' votewitness ' +
        jsonArg([voter, witness]) + '-p %s'%voter)

def createPost(author, permlink, header, body, curatorsPrcnt, *, beneficiaries=[]):
    retry(args.cleos + 'push action c.publish createmssg' +
        jsonArg([author, permlink, "", "", beneficiaries, 0, False, header, body, 'ru', [], '', curatorsPrcnt]) +
        '-p %s'%author)

def createComment(author, permlink, pauthor, ppermlink, header, body, curatorsPrcnt, *, beneficiaries=[]):
    retry(args.cleos + 'push action c.publish createmssg' +
        jsonArg([author, permlink, pauthor, ppermlink, beneficiaries, 0, False, header, body, 'ru', [], '', curatorsPrcnt]) +
        '-p %s'%author)

def upvotePost(voter, author, permlink, weight):
    retry(args.cleos + 'push action c.publish upvote' +
        jsonArg([voter, author, permlink, weight]) +
        '-p %s'%voter)

def downvotePost(voter, author, permlink, weight):
    retry(args.cleos + 'push action c.publish downvote' +
        jsonArg([voter, author, permlink, weight]) +
        '-p %s'%voter)

def unvotePost(voter, author, permlink):
    retry(args.cleos + 'push action c.publish unvote' +
        jsonArg([voter, author, permlink]) +
        '-p %s'%voter)

# --------------------- ACTIONS -----------------------------------------------

def regestrationCommunity():
    run(args.cleos + 'push action cmmn.list create ' + jsonArg(
        {
            'token_name':args.symbol,
            'community_name':'community 1',
        }) + '-p cmmn.list')

def infoCommunity():
    run(args.cleos + 'push action cmmn.list addinfo ' + jsonArg(
        {
            'token_name':args.symbol,
            'community_name':'community 1',
            'ticker':'ticker text',
            'avatar':'avatar url',
            'cover_img_link':'img link',
            'description':'description community',
            'rules':'rules text'
        }) + '-p c.issuer')

def stepKillAll():
    run('killall keosd || true')
    sleep(1.5)

def startWallet():
    run('rm -rf ' + os.path.abspath(args.wallet_dir))
    run('mkdir -p ' + os.path.abspath(args.wallet_dir))
    background(args.keosd + ' --unlock-timeout %d --http-server-address 127.0.0.1:6667 --unix-socket-path \'\' --wallet-dir %s' % (unlockTimeout, os.path.abspath(args.wallet_dir)))
    sleep(.4)

def importKeys():
    run(args.cleos + 'wallet create -n "community" --to-console')
    keys = {}
    run(args.cleos + 'wallet import -n "community" --private-key ' + args.private_key)
    keys[args.private_key] = True
    if not args.cyber_private_key in keys:
        run(args.cleos + 'wallet import -n "community" --private-key ' + args.cyber_private_key)
        keys[args.cyber_private_key] = True
    for a in accounts:
        key = a['pvt']
        if not key in keys:
            if len(keys) >= args.max_user_keys:
                break
            keys[key] = True
            run(args.cleos + 'wallet import -n "community" --private-key ' + key)

def createCommunAccounts():
    for acc in communAccounts:
        if not (args.golos_genesis and acc.inGenesis):
            createAccount('cyber', acc.name, args.public_key)
    updateAuth('c.issuer',  'witn.major', 'active', [args.public_key], [])
    updateAuth('c.issuer',  'witn.minor', 'active', [args.public_key], [])
    updateAuth('c.issuer',  'witn.smajor', 'active', [args.public_key], [])
    updateAuth('c.issuer',  'active', 'owner', [args.public_key], ['c.ctrl@cyber.code', 'c.emit@cyber.code'])
    updateAuth('c.ctrl',    'active', 'owner', [args.public_key], ['c.ctrl@cyber.code'])
    updateAuth('c.publish', 'active', 'owner', [args.public_key], ['c.publish@cyber.code'])
    updateAuth('c.social',  'active', 'owner', [args.public_key], ['c.publish@cyber.code'])
    updateAuth('c.emit',    'active', 'owner', [args.public_key], ['c.emit@cyber.code'])
    
    updateAuth('c.issuer',  'issue', 'active', ['GLS5a2eDuRETEg7uy8eHbiCqGZM3wnh2pLjiXrFduLWBKVZKCkB62'], [])
    linkAuth('c.issuer', 'cmmn.token', 'issue', 'issue')
    linkAuth('c.issuer', 'cmmn.token', 'transfer', 'issue')
    linkAuth('c.issuer', 'cyber.token', 'issue', 'issue')
    linkAuth('c.issuer', 'cyber.token', 'transfer', 'issue')

def stepInstallContracts():
    for acc in communAccounts:
        if (acc.contract != None):
            retry(args.cleos + 'set contract %s %s' % (acc.name, args.contracts_dir + acc.contract))

def stepCreateTokens():
    retry(args.cleos + 'push action cmmn.token create ' + jsonArg(["c.issuer", intToToken(1000000000*1000), 10000, 100]) + ' -p cmmn.token')
    retry(args.cleos + 'push action cmmn.vesting create ' + jsonArg([args.vesting, 'c.ctrl']) + '-p c.issuer')
    retry(args.cleos + 'push action cyber.token issue ' + jsonArg(["c.issuer", intToCommun(500000000*10000), ""]) + ' -p cmmn.owner')
    retry(args.cleos + 'push action cyber.token transfer ' + jsonArg(["c.issuer", "cmmn.token", intToCommun(500000000*10000), "restock: " + args.symbol]) + ' -p c.issuer')
    retry(args.cleos + 'push action cmmn.token issue ' + jsonArg(["c.issuer", intToToken(500000000*1000), "initial supply"]) + ' -p c.issuer')
    for acc in communAccounts:
        openTokenBalance(acc.name)
    sleep(1)

def createCommunity():
    retry(args.cleos + 'push action c.emit setparams ' + jsonArg([
        [
            ['inflation_rate',{
                'start':1500,
                'stop':95,
                'narrowing':250000
            }],
            ['reward_pools',{
                'pools':[
                    {'name':'c.ctrl','percent':0},
                    {'name':'c.publish','percent':6000},
                    {'name':'cmmn.vesting','percent':2400}
                ]
            }],
            ['emit_token',{
                'symbol':args.token
            }],
            ['emit_interval',{
                'value':900
            }]
        ]]) + '-p c.emit')
    retry(args.cleos + 'push action cmmn.vesting setparams ' + jsonArg([
        args.vesting,
        [
            ['vesting_withdraw',{
                'intervals':13,
                'interval_seconds':120
            }],
            ['vesting_amount',{
                'min_amount':10000
            }],
            ['vesting_delegation',{
                'min_amount':5000000,
                'min_remainder':15000000,
                'min_time':0,
                'max_interest':0,
                'return_time':120
            }]
        ]]) + '-p c.issuer')
    retry(args.cleos + 'push action c.ctrl setparams ' + jsonArg([
        [
            ['ctrl_token',{
                'code':args.symbol
            }],
            ['multisig_acc',{
                'name':'c.issuer'
            }],
            ['max_witnesses',{
                'max':5
            }],
            ['multisig_perms',{
                'super_majority':0,
                'majority':0,
                'minority':0
            }],
            ['max_witness_votes',{
                'max':30
            }],
            ['update_auth',{
                'period':300
            }]
        ]]) + '-p c.ctrl')

    retry(args.cleos + 'push action c.ref setparams ' + jsonArg([[
        ['breakout_parametrs', {'min_breakout': intToToken(10000), 'max_breakout': intToToken(100000)}],
        ['expire_parametrs', {'max_expire': 7*24*3600}],  # sec
        ['percent_parametrs', {'max_percent': 5000}],     # 50.00%
        ['delay_parametrs', {'delay_clear_old_ref': 1*24*3600}] # sec
    ]]) + '-p c.ref')

def createWitnessAccounts():
    for i in range(firstWitness, firstWitness + numWitness):
        a = accounts[i]
        createAccount('cyber', a['name'], a['pub'])
        openCommunBalance(a['name'])
        openTokenBalance(a['name'])
        openVestingBalance(a['name'])
        buyToken(a['name'], 1000000)
        buyVesting(a['name'], 100000)   # TODO buy vesting on all community token
        registerWitness('c.ctrl', a['name'])
        voteWitness('c.ctrl', a['name'], a['name'])

def initCommunity():
    retry(args.cleos + 'push action c.publish setparams ' + jsonArg([[
        ['st_max_vote_changes', {'value':5}],
        ['st_cashout_window', {'window': 120, 'upvote_lockout': 15}],
        ['st_max_beneficiaries', {'value': 64}],
        ['st_max_comment_depth', {'value': 127}],
        ['st_social_acc', {'value': 'c.social'}],
        ['st_referral_acc', {'value': ''}], # TODO Add referral contract to posting-settings
        ['st_curators_prcnt', {'min_curators_prcnt': 0, 'max_curators_prcnt': 9000}]
    ]]) + '-p c.publish')

    retry(args.cleos + 'push action c.publish setrules ' + jsonArg({
        "mainfunc":{"str":"x","maxarg":fixp_max},
        "curationfunc":{"str":golos_curation_func_str,"maxarg":fixp_max},
        "timepenalty":{"str":"x/1800","maxarg":1800},
        "maxtokenprop":5000,
        "tokensymbol":args.token
    }) + '-p c.publish')

    limits = [
        # action            charge_id   price   cutoff  vest_price  min_vest
        ("post",            0,          -1,     0,      0,          0),
        ("comment",         0,          -1,     0,      0,          0),
        ("vote",            0,          -1,     0,      0,          0),
        ("post bandwidth",  0,          -1,     0,      0,          0),
    ]
    for (action, charge_id, price, cutoff, vest_price, min_vest) in limits:
        retry(args.cleos + 'push action c.publish setlimit ' + jsonArg({
            "act": action,
            "token_code": args.symbol,
            "charge_id" : charge_id,
            "price": price,
            "cutoff": cutoff,
            "vesting_price": vest_price,
            "min_vesting": min_vest
        }) + '-p c.publish')

    retry(args.cleos + 'push action c.emit start ' + jsonArg([]) + '-p c.emit')

def addUsers():
    for i in range(0, firstWitness-1):
        a = accounts[i]
        createAccount('cyber', a['name'], a['pub'])
        openCommunBalance(a['name'])
        openTokenBalance(a['name'])
        openVestingBalance(a['name'])
        buyToken(a['name'], 1000)
        buyVesting(a['name'], 100)  # TODO Buy vesting on all community token

# Command Line Arguments

parser = argparse.ArgumentParser()

commands = [
#    Short Command          Function                    inAll  inDocker Description
    ('k', 'kill',           stepKillAll,                True,  False,   "Kill all nodeos and keosd processes"),
    ('w', 'wallet',         startWallet,                True,  False,   "Start wallet (start keosd)"),
    ('K', 'keys',           importKeys,                 True,  True,    "Create and fill wallet with keys"),
    ('A', 'accounts',       createCommunAccounts,       True,  True,    "Create golos accounts (c.*)"),
    ('c', 'contracts',      stepInstallContracts,       True,  True,    "Install contracts (ctrl, emit, vesting, publish)"),
    ('t', 'tokens',         stepCreateTokens,           True,  True,    "Create tokens"),
    ('r', 'registration',   regestrationCommunity,      True,  True,    "Registration community"),
    ('C', 'community',      createCommunity,            True,  True,    "Create community"),
    ('U', 'witnesses',      createWitnessAccounts,      True,  True,    "Create witnesses accounts"),
    ('i', 'init',           initCommunity,              True,  True,    "Init community"),
    ('u', 'users',          addUsers,                   True,  True,    "Add users"),
    ('ic','info',           infoCommunity,              True,  True,    "Add info community"),
]

parser.add_argument('--public-key', metavar='', help="Golos Public Key", default='GLS6Tvw3apAGeHCUTWpf9DY4xvUoKwmuDatW7GV8ygkuZ8F6y4Yor', dest="public_key")
parser.add_argument('--private-key', metavar='', help="Golos Private Key", default='5KekbiEKS4bwNptEtSawUygRb5sQ33P6EUZ6c4k4rEyQg7sARqW', dest="private_key")
parser.add_argument('--cyber-private-key', metavar='', help="Cyberway Private Key", default='5K463ynhZoCDDa4RDcr63cUwWLTnKqmdcoTKTHBjqoKfv4u5V7p', dest="cyber_private_key")
parser.add_argument('--programs-dir', metavar='', help="Programs directory for cleos, nodeos, keosd", default='/mnt/working/eos-debug.git/build/programs');
parser.add_argument('--cleos', metavar='', help="Cleos command (default in programs-dir)", default='cleos/cleos')
parser.add_argument('--keosd', metavar='', help="Path to keosd binary (default in programs-dir", default='keosd/keosd')
parser.add_argument('--contracts-dir', metavar='', help="Path to contracts directory", default='../../build/')
parser.add_argument('--wallet-dir', metavar='', help="Path to wallet directory", default='./wallet/')
parser.add_argument('--log-path', metavar='', help="Path to log file", default='./output.log')
parser.add_argument('--user-limit', metavar='', help="Max number of users. (0 = no limit)", type=int, default=3000)
parser.add_argument('--max-user-keys', metavar='', help="Maximum user keys to import into wallet", type=int, default=100)
parser.add_argument('--witness-limit', metavar='', help="Maximum number of witnesses. (0 = no limit)", type=int, default=0)
parser.add_argument('--symbol', metavar='', help="The system token symbol", default='GLS')
parser.add_argument('--token-precision', metavar='', help="The Golos community token precision", type=int, default=3)
parser.add_argument('--vesting-precision', metavar='', help="The Golos community vesting precision", type=int, default=6)
parser.add_argument('--docker', action='store_true', help='Run actions only for Docker (used with -a)')
parser.add_argument('--golos-genesis', action='store_true', help='Run action only for nodeos with golos-genesis')
parser.add_argument('-a', '--all', action='store_true', help="Do everything marked with (*)")
parser.add_argument('-H', '--http-port', type=int, default=8000, metavar='', help='HTTP port for cleos')

for (flag, command, function, inAll, inDocker, help) in commands:
    prefix = ''
    if inAll or inDocker: prefix += ('*' if inAll else ' ') + ('D' if inDocker else ' ')
    if prefix: help = '(' + prefix + ') ' + help
    if flag:
        parser.add_argument('-' + flag, '--' + command, action='store_true', help=help, dest=command)
    else:
        parser.add_argument('--' + command, action='store_true', help=help, dest=command)

args = parser.parse_args()

if (parser.get_default('cleos') == args.cleos):
    args.cleos = args.programs_dir + '/' + args.cleos
    args.cleos += ' --wallet-url http://127.0.0.1:6667 --url http://127.0.0.1:%d ' % args.http_port

if (parser.get_default('keosd') == args.keosd):
    args.keosd = args.programs_dir + '/' + args.keosd

args.token = '%d,%s' % (args.token_precision, args.symbol)
args.vesting = '%d,%s' % (args.vesting_precision, args.symbol)

logFile = open(args.log_path, 'a')

logFile.write('\n\n' + '*' * 80 + '\n\n\n')

accounts_filename = os.path.dirname(os.path.realpath(__file__)) + '/accounts.json'
with open(accounts_filename) as f:
    a = json.load(f)
    if args.user_limit:
        del a['users'][args.user_limit:]
    if args.witness_limit:
        del a['witnesses'][args.witness_limit:]
    firstWitness = len(a['users'])
    numWitness = len(a['witnesses'])
    accounts = a['users'] + a['witnesses']

haveCommand = False
for (flag, command, function, inAll, inDocker, help) in commands:
    if getattr(args, command) or (inDocker if args.docker else inAll) and args.all:
        if function:
            haveCommand = True
            function()
if not haveCommand:
    print('community-boot-sequence.py: Tell me what to do. -a does almost everything. -h shows options.')
