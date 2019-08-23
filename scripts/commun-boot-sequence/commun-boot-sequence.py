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
    ('cmmn.owner',   None),
    ('cmmn.token',   'bancor.token'),
    ('cmmn.list',    'commun.list'),
    ('resrv.emit',   'reserve.emit'),
    ('registrar',    'registrar'),
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
    print('commun-boot-sequence.py:', command)
    logFile.write(command + '\n')
    if subprocess.call(command, shell=True):
        return False
    return True

def run(args):
    print('commun-boot-sequence.py:', args)
    logFile.write(args + '\n')
    if subprocess.call(args, shell=True):
        print('commun-boot-sequence.py: exiting because of error')
        sys.exit(1)

def retry(args):
    count = 5
    while count:
        count = count-1
        print('commun-boot-sequence.py:', args)
        logFile.write(args + '\n')
        if subprocess.call(args, shell=True):
            print('*** Retry: ', count)
            sleep(0.5)
        else:
            return True
    print('commun-boot-sequence.py: exiting because of error')
    sys.exit(1)

def background(args):
    print('commun-boot-sequence.py:', args)
    logFile.write(args + '\n')
    return subprocess.Popen(args, shell=True)

def getOutput(args):
    print('commun-boot-sequence.py:', args)
    logFile.write(args + '\n')
    proc = subprocess.Popen(args, shell=True, stdout=subprocess.PIPE)
    return proc.communicate()[0].decode('utf-8')

def getJsonOutput(args):
    print('commun-boot-sequence.py:', args)
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

def intToTokenCommun(value):
    return intToCurrency(value, 4, 'COMMUN')

# --------------------- EOSIO functions ---------------------------------------

def setCommunParams():
    retry(args.cleos + 'push action registrar setparams ' + jsonArg(
        {
            "params": [
                [
                    "token_param",
                    {
                        "sym": args.token
                    }
                ],
                [
                    "market_param",
                    {
                        "bid_increment_denom": 10,
                        "bancor_creation_fee": 3000,
                        "sale_fee": 0
                    }
                ],
                [
                    "checkwin_param",
                    {
                        "interval": 1440000,
                        "min_time_from_last_win": 216,
                        "min_time_from_last_bid_start": 216,
                        "min_time_from_last_bid_step": 27,
                        "max_bid_checks": 64
                        
                    }
                ]
            ]
        }
    ) + '-p registrar')

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

def stepKillAll():
    run('killall keosd || true')
    sleep(1.5)

def startWallet():
    run('rm -rf ' + os.path.abspath(args.wallet_dir))
    run('mkdir -p ' + os.path.abspath(args.wallet_dir))
    background(args.keosd + ' --unlock-timeout %d --http-server-address 127.0.0.1:6667 --unix-socket-path \'\' --wallet-dir %s' % (unlockTimeout, os.path.abspath(args.wallet_dir)))
    sleep(.4)

def importKeys():
    run(args.cleos + 'wallet create -n "commun" --to-console')
    keys = {}
    run(args.cleos + 'wallet import -n "commun" --private-key ' + args.private_key)
    keys[args.private_key] = True
    if not args.cyber_private_key in keys:
        run(args.cleos + 'wallet import -n "commun" --private-key ' + args.cyber_private_key)
        keys[args.cyber_private_key] = True
    for a in accounts:
        key = a['pvt']
        if not key in keys:
            if len(keys) >= args.max_user_keys:
                break
            keys[key] = True
            run(args.cleos + 'wallet import -n "commun" --private-key ' + key)

def createCommunAccounts():
    for acc in communAccounts:
        if not (args.golos_genesis and acc.inGenesis):
            createAccount('cyber', acc.name, args.public_key)

def stepInstallContracts():
    for acc in communAccounts:
        if (acc.contract != None):
            retry(args.cleos + 'set contract %s %s' % (acc.name, args.contracts_dir + acc.contract))

def stepCreateTokens():
    retry(args.cleos + 'push action cyber.token create ' + jsonArg(["cmmn.owner", intToTokenCommun(10000000000*10000)]) + ' -p cyber.token')
    sleep(1)

# Command Line Arguments

parser = argparse.ArgumentParser()

commands = [
#    Short Command          Function                    inAll  inDocker Description
    ('k', 'kill',           stepKillAll,                True,  False,   "Kill all nodeos and keosd processes"),
    ('w', 'wallet',         startWallet,                True,  False,   "Start wallet (start keosd)"),
    ('K', 'keys',           importKeys,                 True,  True,    "Create and fill wallet with keys"),
    ('A', 'accounts',       createCommunAccounts,       True,  True,    "Create golos accounts (cmmn.*)"),
    ('c', 'contracts',      stepInstallContracts,       True,  True,    "Install contracts (ctrl, emit, vesting, publish)"),
    ('t', 'tokens',         stepCreateTokens,           True,  True,    "Create tokens"),
    ('p', 'params',         setCommunParams,            True,  True,    "Create params"),
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
    print('commun-boot-sequence.py: Tell me what to do. -a does almost everything. -h shows options.')
