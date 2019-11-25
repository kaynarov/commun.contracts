import os
import json
import subprocess
import json
import re
import random
import time

from pymongo import MongoClient
import pymongo

params = {
    'cleos_path': os.environ.get("CLEOS", "cleos"),
    'nodeos_url': os.environ.get("CYBERWAY_URL", "http://localhost:8888"),
    'mongodb':    os.environ.get("MONGODB", "mongodb://localhost:27018"),
}
cleosCmd = "{cleos_path} --url {nodeos_url} ".format(**params)
mongoClient = MongoClient(params["mongodb"])

class Symbol():
    def __init__(self, code, precission):
        self.code = code
        self.precission = precission

    def __str__(self):
        return '{p},{s}'.format(p=self.precission, s=self.code)

class Asset():
    def __init__(self, string):
        (value, code) = string.split(' ')
        (p, f) = value.split('.')
        self.symbol = Symbol(code, len(f))
        self.amount = int(p+f)
    
    def __str__(self):
        return '{:.{prec}f} {code}'.format(self.amount/(10**self.symbol.precission), prec=self.symbol.precission, code=self.symbol.code)

class JSONEncoder(json.JSONEncoder):
    def default(self, value):
        if isinstance(value, Symbol) or isinstance(value, Asset):
            return value.__str__()
        if isinstance(value, Trx):
            return value.getTrx()
        super().default(value)

def jsonArg(a):
    return " '" + json.dumps(a, cls=JSONEncoder) + "' "

def _cleos(arguments, *, output=True, retry=None):
    cmd = cleosCmd + arguments
    if output:
        print("cleos: " + cmd)
    msg = '' if retry is None else '*** Retry: {retry}\n'.format(retry=retry)
    while True:
      (exception, traceback) = (None, None)
      try:
        return subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT, universal_newlines=True)
        #return subprocess.check_output(cmd, shell=True, universal_newlines=True)
      except subprocess.CalledProcessError as e:
        import sys
        (exception, traceback) = (e, sys.exc_info()[2])

      msg += str(exception) + ' with output:\n' + exception.output + '\n'
      if retry is None or retry <= 1:
        raise Exception(msg).with_traceback(traceback)
      else:
        retry -= 1
        msg += '*** Retry: {retry}\n'.format(retry=retry)

def cleos(arguments, *, additional='', providebw=None, keys=None, retry=None):
    if type(providebw) == type([]):
        additional += ''.join(' --bandwidth-provider ' + provider for provider in providebw)
    else:
        additional += ' --bandwidth-provider ' + providebw if providebw else ''
    if keys:
        trx = _cleos(arguments + additional + ' --skip-sign --dont-broadcast')
        if isinstance(keys, str):
            keys=[keys]
        for k in keys:
            trx = _cleos("sign '%s' --private-key %s 2>/dev/null" % (trx, k), output=False)
        return _cleos("push transaction -j --skip-sign '%s'" % trx, output=False, retry=retry)
    else:
        return _cleos(arguments + additional, retry=retry)
    
def pushAction(code, action, actor, args, *, additional='', delay=None, expiration=None, providebw=None, keys=None, retry=None):
    additional += ' --delay-sec %d' % delay if delay else ''
    additional += ' --expiration %d' % expiration if expiration else ''
    if type(actor) == type([]):
        additional += ''.join(' -p ' + a for a in actor)
    else:
        additional += ' -p ' + actor
    cmd = 'push action -j {code} {action} {args}'.format(code=code, action=action, args=jsonArg(args))
    return json.loads(cleos(cmd, additional=additional, providebw=providebw, keys=keys, retry=retry))


def pushTrx(trx, *, additional='', keys=None):
    cmd = 'push transaction -j {trx}'.format(trx=jsonArg(trx.getTrx()))
    return json.loads(cleos(cmd, additional=additional, keys=keys))


def parseAuthority(auth):
    if type(auth) == type([]):
        return [parseAuthority(a) for a in auth]
    d = auth.split('@',2)
    if len(d) == 1:
        d.extend(['active'])
    return {'actor':d[0], 'permission':d[1]}

def createAction(contract, action, actors, args):
    data = cleos("convert pack_action_data {contract} {action} {args}".format(
            contract=contract, action=action, args=jsonArg(args))).rstrip()
    return {
        'account':contract,
        'name':action,
        'authorization':parseAuthority(actors if type(actors)==type([]) else [actors]),
        'data':data
    }

class Trx:
    def __init__(self, expiration=None, delay_sec=None):
        additional = '--skip-sign --dont-broadcast'
        if expiration:
            additional += ' --expiration {exp}'.format(exp=expiration)
        if delay_sec:
            additional += ' --delay-sec {sec}'.format(sec=delay_sec)
        self.trx = pushAction('cyber', 'checkwin', 'cyber', {}, additional=additional)
        self.trx['actions'] = []

    def addAction(self, contract, action, actors, args):
        self.trx['actions'].append(createAction(contract, action, actors, args))

    def getTrx(self):
        return self.trx


# --------------------- CYBER functions ---------------------------------------

def createAuthority(keys, accounts):
    keys.sort()
    keysList = []
    accountsList = []
    for k in keys:
        keysList.extend([{'weight':1,'key':k}])

    acc = []
    for a in accounts:
        d = a.split('@',2)
        if len(d) == 1:
            d.extend(['active'])
        acc.append(d)
    acc.sort()
    for d in acc:
        accountsList.extend([{'weight':1,'permission':{'actor':d[0],'permission':d[1]}}])
    return {'threshold': 1, 'keys': keysList, 'accounts': accountsList, 'waits':[]}

def createAccount(creator, account, owner_auth, active_auth=None, *, providebw=None, keys=None):
    if active_auth is None:
        active_auth = owner_auth
    creator_auth = parseAuthority(creator)
    additional = '' if creator_auth['permission'] is None else ' -p {auth}'.format(auth=creator)
    return cleos('create account {creator} {acc} {owner} {active}'.format(creator=creator_auth['actor'], acc=account, owner=owner_auth, active=active_auth), 
            providebw=providebw, keys=keys, additional=additional)

def getAccount(account):
    acc = json.loads(cleos('get account -j {acc}'.format(acc=account)))
    perm = {}
    for p in acc['permissions']:
        perm[p['perm_name']] = p
    acc['permissions'] = perm
    return acc

def updateAuth(account, permission, parent, keyList, accounts, *, providebw=None, keys=None):
    return pushAction('cyber', 'updateauth', account, {
        'account': account,
        'permission': permission,
        'parent': parent,
        'auth': createAuthority(keyList, accounts)
    }, providebw=providebw, keys=keys)

def linkAuth(account, code, action, permission, *, providebw=None, keys=None):
    return cleos('set action permission {acc} {code} {act} {perm}'.format(acc=account, code=code, act=action, perm=permission),
            providebw=providebw, keys=keys)

def transfer(sender, recipient, amount, memo="", *, providebw=None, keys=None):
    return pushAction('cyber.token', 'transfer', sender, {'from':sender, 'to':recipient, 'quantity':amount, 'memo':memo}
        , providebw=providebw, keys=keys)



def wait(timeout):
    while timeout > 0:
        w = 3 if timeout > 3 else timeout
        print('\r                    \rWait %d sec' % timeout, flush=True, end='')
        timeout -= w
        time.sleep(w)

def randomName():
    letters = "abcdefghijklmnopqrstuvwxyz12345"
    return ''.join(random.choice(letters) for i in range(12))

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
    data = cleos("create key --to-console")
    m = re.search('Private key: ([a-zA-Z0-9]+)\nPublic key: ([a-zA-Z0-9]+)', data)
    return (m.group(1), m.group(2))

def importPrivateKey(private):
    cleos("wallet import --name test --private-key %s" % private)

def createRandomAccount(owner_auth, active_auth=None, *, creator='tech', providebw=None, keys=None):
    while True:
        name = randomName()
        try:
            getAccount(name)
        except:
            break
    createAccount(creator, name, owner_auth, active_auth, providebw=providebw, keys=keys)
    return name

def getResourceUsage(account):
    info = getAccount(account)
    return {
        'cpu': info['cpu_limit']['used'],
        'net': info['net_limit']['used'],
        'ram': info['ram_limit']['used'],
        'storage': info['storage_limit']['used']
    }

def msigPropose(proposer, proposal_name, requested_permissions, trx, providebw=None, keys=None):
    pushAction('cyber.msig', 'propose', proposer, {
            'proposer': proposer,
            'proposal_name': proposal_name,
            'requested': requested_permissions,
            'trx': trx
        }, providebw=providebw, keys=keys)

def msigApprove(approver, proposer, proposal_name, providebw=None, keys=None):
    pushAction('cyber.msig', 'approve', approver, {
            'proposer': proposer,
            'proposal_name': proposal_name,
            'level': {'actor': approver, 'permission': 'active'}
        }, providebw=providebw, keys=keys)

def msigExec(executer, proposer, proposal_name, providebw=None, keys=None):
    pushAction('cyber.msig', 'exec', proposer, {
            'executer': executer,
            'proposer': proposer,
            'proposal_name': proposal_name
        }, providebw=providebw, keys=keys)
