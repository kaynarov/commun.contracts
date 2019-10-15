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
    #('comn',        None),    # comn - owner for COMMUN. Must be created outside of this script!
    ('comn.point',   'commun.point',         None,                [("active", [], ["comn@active", "comn.point@cyber.code"], []),
                                                                   ("issueperm", [], ["comn.emit@cyber.code"], ["comn.point:issue"]),
                                                                   ("transferperm", [], ["comn.emit@cyber.code"], ["comn.point:transfer"])]),
    ('comn.ctrl',    'commun.ctrl',          None,                [("changepoints", [], ["comn.point@cyber.code"], ["comn.ctrl:changepoints"]),
                                                                   ("create", [], ["comn.list@cyber.code"], ["comn.ctrl:create"])]),
    ('comn.emit',    'commun.emit',          None,                [("rewardperm", [], ["comn.ctrl@cyber.code", "comn.gallery@cyber.code"], ["comn.emit:issuereward"]),
                                                                   ("create", [], ["comn.list@cyber.code"], ["comn.emit:create"])]),
    ('comn.list',    'commun.list',          None,                []),
    ('comn.gallery', 'commun.publication',   ['commun.gallery'],  []),
    ('comn.social',  'commun.social',        None,                []),
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
        createAccount('comn', acc.name, 'comn@owner', 'comn@active')

    for acc in communAccounts:
        for perm in acc.permissions:
            updateAuth(acc.name, perm.name, perm.parent, perm.keys, perm.accounts, providebw=acc.name+'/comn')
            for link in perm.links:
                (code, action) = link.split(':',2)
                linkAuth(acc.name, code, action, perm.name, providebw=acc.name+'/comn')

def installContracts():
    for acc in communAccounts:
        if (acc.contract != None):
            loadContract(acc.name, acc.contract, providebw=acc.name+'/comn', retry=3, contracts_dir=args.contracts_dir, warmup_code=acc.warmup_code)


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
