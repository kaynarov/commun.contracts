import os
import json
import subprocess
import json
import re
import random
import time

from pymongo import MongoClient
from copy import deepcopy
import pymongo
from .prettylog import log_action
from .jsonprinter import Items

class TrxLogger:
    def formatCleosCmd(self, cmd, output=False): print('cleos:', cmd)
    def formatTrx(self, trx): pass
    def formatReceipt(self, receipt): pass
    def formatError(self, error): print(error)

def setTrxLogger(newLogger):
    global trxLogger
    prevTrxLogger = trxLogger
    trxLogger = newLogger
    return prevTrxLogger

params = {
    'cleos_path': os.environ.get("CLEOS", "cleos"),
    'nodeos_url': os.environ.get("CYBERWAY_URL", "http://localhost:8888"),
    'mongodb':    os.environ.get("MONGODB", "mongodb://localhost:27018"),
}
cleosCmd = "{cleos_path} --url {nodeos_url} ".format(**params)
mongoClient = MongoClient(params["mongodb"])
trxLogger = TrxLogger()

class Symbol():
    def __init__(self, precision, code):
        if not isinstance(precision, int):
            raise Exception("Invalid precision")
        if not isinstance(code, str):
            raise Exception("Invalid code")
        self.precision = precision
        self.code = code

    def __str__(self):
        return '{p},{s}'.format(p=self.precision, s=self.code)

    def __eq__(self, other):
        return self.precision == other.precision and self.code == other.code

class Asset():
    def __init__(self, amount, symbol):
        if not isinstance(amount, int):
            raise Exception("Invalid amount: "+ str(amount) + " " + str(type(amount)))
        self.amount = amount
        self.symbol = symbol

    @classmethod
    def fromdb(cls, data):
        return Asset(data['_amount'], Symbol(int(data['_decs'].to_decimal()), data['_sym']))

    @classmethod
    def fromstr(cls, string):
        (value, code) = string.split(' ')
        (p, f) = value.split('.')
        return Asset(int(p+f), Symbol(len(f), code))
    
    def __str__(self):
        return '{:.{prec}f} {code}'.format(self.amount/(10**self.symbol.precision), prec=self.symbol.precision, code=self.symbol.code)

    def __add__(self, other):
        if isinstance(other, Asset):
            if self.symbol != other.symbol:
                raise Exception("Try to add asset with different symbol")
            return Asset(self.amount + other.amount, self.symbol)
        else:
            return NotImplemented

    def __sub__(self, other):
        if isinstance(other, Asset):
            if self.symbol != other.symbol:
                raise Exception("Try to sub asset with different symbol")
            return Asset(self.amount - other.amount, self.symbol)
        else:
            return NotImplemented

    def __floordiv__(self, other):
        if isinstance(other, int):
            return Asset(self.amount // other, self.symbol)
        elif isinstance(other, float):
            return Asset(int(self.amount / other), self.symbol)
        else:
            return NotImplemented

    def __mul__(self, other):
        if isinstance(other, int):
            return Asset(self.amount * other, self.symbol)
        elif isinstance(other, float):
            return Asset(int(self.amount * other), self.symbol)
        else:
            return NotImplemented

    def __imul__(self, other):
        return self.__mul__(other)

    def __neg__(self):
        return Asset(-self.amount, self.symbol)

    def __eq__(self, other):
        if isinstance(other, Asset):
            if self.symbol != other.symbol:
                raise Exception("Try to compare asset with different symbol")
            return self.amount == other.amount
        else:
            return NotImplemented

    def __gt__(self, other):
        if isinstance(other, Asset):
            if self.symbol != other.symbol:
                raise Exception("Try to compare asset with different symbol")
            return self.amount > other.amount
        else:
            return NotImplemented

class JSONEncoder(json.JSONEncoder):
    def default(self, value):
        if isinstance(value, Symbol) or isinstance(value, Asset):
            return value.__str__()
        if isinstance(value, Trx):
            return value.getTrx()
        super().default(value)

def jsonArg(a):
    return " '" + json.dumps(a, cls=JSONEncoder) + "' "

class CleosException(subprocess.CalledProcessError):
    def __init__(self, returncode, cmd, output=None, stderr=None):
        super().__init__(returncode, cmd, output, stderr)

    def __str__(self):
        return super().__str__() + ' with output:\n' + self.output + '\n'

    @staticmethod
    def fromCalledProcessError(err):
        return CleosException(err.returncode, err.cmd, err.output, err.stderr)


def _cleos(arguments, *, output=False, retry=None, logger=None):
    cmd = cleosCmd + arguments
    if logger: logger.formatCleosCmd(cmd, output=output)
    while True:
      (exception, traceback) = (None, None)
      try:
        return subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT, universal_newlines=True)
      except subprocess.CalledProcessError as e:
        if logger: logger.formatError(e)
        import sys
        (exception, traceback) = (e, sys.exc_info()[2])

      if retry is None or retry <= 1:
          raise CleosException.fromCalledProcessError(exception).with_traceback(traceback)
      else: retry -= 1


def cleos(arguments, *, additional='', providebw=None, keys=None, retry=None, **kwargs):
    #if len(kwargs) != 0: raise Exception("Unparsed arguments "+str(kwargs))
    if type(providebw) == type([]):
        additional += ''.join(' --bandwidth-provider ' + provider for provider in providebw)
    else:
        additional += ' --bandwidth-provider ' + providebw if providebw else ''
    if keys:
        trx = _cleos(arguments + additional + ' --skip-sign --dont-broadcast', logger=trxLogger, **kwargs)
        if kwargs.get('output',False) and trxLogger:
            trxLogger.formatTrx(trx)
        if isinstance(keys, str): keys=[keys]
        for k in keys:
            trx = _cleos("sign '%s' --private-key %s 2>/dev/null" % (trx, k))

        try:
            result = _cleos("push transaction -j --skip-sign '%s'" % trx, retry=retry)
            if kwargs.get('output', False) and trxLogger: trxLogger.formatReceipt(result)
            return result
        except Exception as err:
            if kwargs.get('output', True) and trxLogger: trxLogger.formatError(err)
            raise
    else:
        return _cleos(arguments + additional, retry=retry, logger=trxLogger if kwargs.get('output',True) else None, **kwargs)
    
def pushAction(code, action, actor, args, *, additional='', delay=None, expiration=None, **kwargs):
    additional += ' --delay-sec %d' % delay if delay else ''
    additional += ' --expiration %d' % expiration if expiration else ''
    if type(actor) == type([]):
        additional += ''.join(' -p ' + a for a in actor)
    else:
        additional += ' -p ' + actor
    cmd = 'push action -j {code} {action} {args}'.format(code=code, action=action, args=jsonArg(args))
    result = json.loads(cleos(cmd, additional=additional, **kwargs))
#    if trxLogger: trxLogger.format(result)
    return result


def unpackActionData(code, action, data):
    if len(data) == 0: return {}
    args = _cleos('convert unpack_action_data {code} {action} "{data}"'.format(code=code, action=action, data=data))
    return json.loads(args)


def pushTrx(trx, **kwargs):
    cmd = 'push transaction -j {trx}'.format(trx=jsonArg(trx.getTrx()))
    return json.loads(cleos(cmd, **kwargs))


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


def createAndExecProposal(commun_code, permission, trx, leaders, clientKey, *, providebw=None, output=None, jsonPrinter=None):
    (proposer, proposerKey) = leaders[0]
    proposal = randomName()

    with log_action("Account {} create and execute proposal {}".format(proposer, proposal)):
      if jsonPrinter:
        requestedAuth = [parseAuthority(lead[0]) for lead in leaders]
        print("Requested authority:", jsonPrinter.format(Items(atnewline=True), requestedAuth))

        t = deepcopy(trx.getTrx())
        for action in t['actions']:
            action['args'] = unpackActionData(action['account'], action['name'], action['data'])
        print("Trx:", jsonPrinter.format(initTrxItems(), t))

      pushAction('c.ctrl', 'propose', proposer, {
            'commun_code': commun_code,
            'proposer': proposer,
            'proposal_name': proposal,
            'permission': permission,
            'trx': trx
        }, providebw=proposer+'/c@providebw', keys=[proposerKey, clientKey])

      for (leaderName, leaderKey) in leaders:
        pushAction('c.ctrl', 'approve', leaderName, {
                'proposer': proposer,
                'proposal_name': proposal,
                'approver': leaderName
            }, providebw=leaderName+'/c@providebw', keys=[leaderKey, clientKey])

      providebw = [] if providebw is None else (providebw if type(providebw)==type([]) else [providebw])
      providebw.append(proposer+'/c@providebw')

      pushAction('c.ctrl', 'exec', proposer, {
            'proposer': proposer,
            'proposal_name': proposal,
            'executer': proposer
        }, providebw=providebw, keys=[proposerKey, clientKey])


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

def createAccount(creator, account, owner_auth, active_auth=None, **kwargs):
    if active_auth is None:
        active_auth = owner_auth
    creator_auth = parseAuthority(creator)
    additional = '' if creator_auth['permission'] is None else ' -p {auth}'.format(auth=creator)
    return cleos('create account {creator} {acc} {owner} {active}'.format(creator=creator_auth['actor'], acc=account, owner=owner_auth, active=active_auth), 
            additional=additional, **kwargs)

def getAccount(account):
    acc = json.loads(cleos('get account -j {acc}'.format(acc=account)))
    perm = {}
    for p in acc['permissions']:
        perm[p['perm_name']] = p
    acc['permissions'] = perm
    return acc

def updateAuth(account, permission, parent, keyList, accounts, **kwargs):
    actor = account + ('@owner' if permission == 'owner' else '')
    return pushAction('cyber', 'updateauth', actor, {
        'account': account,
        'permission': permission,
        'parent': parent,
        'auth': createAuthority(keyList, accounts)
    }, **kwargs)

def linkAuth(account, code, action, permission, **kwargs):
    return cleos('set action permission {acc} {code} {act} {perm}'.format(acc=account, code=code, act=action, perm=permission),
            **kwargs)

def transfer(sender, recipient, amount, memo="", **kwargs):
    return pushAction('cyber.token', 'transfer', sender, {'from':sender, 'to':recipient, 'quantity':amount, 'memo':memo}, **kwargs)



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

def getRandomAccount():
    while True:
        name = randomName()
        try: getAccount(name)
        except: break
    return name

def createRandomAccount(owner_auth, active_auth=None, *, creator='tech', **kwargs):
    name = getRandomAccount()
    createAccount(creator, name, owner_auth, active_auth, **kwargs)
    return name

def getResourceUsage(account):
    info = getAccount(account)
    return {
        'cpu': info['cpu_limit']['used'],
        'net': info['net_limit']['used'],
        'ram': info['ram_limit']['used'],
        'storage': info['storage_limit']['used']
    }

def msigPropose(proposer, proposal_name, requested_permissions, trx, **kwargs):
    pushAction('cyber.msig', 'propose', proposer, {
            'proposer': proposer,
            'proposal_name': proposal_name,
            'requested': requested_permissions,
            'trx': trx
        }, **kwargs)

def msigApprove(approver, proposer, proposal_name, **kwargs):
    pushAction('cyber.msig', 'approve', approver, {
            'proposer': proposer,
            'proposal_name': proposal_name,
            'level': {'actor': approver, 'permission': 'active'}
        }, **kwargs)

def msigExec(executer, proposer, proposal_name, **kwargs):
    pushAction('cyber.msig', 'exec', proposer, {
            'executer': executer,
            'proposer': proposer,
            'proposal_name': proposal_name
        }, **kwargs)



def initApplyTrxItems(hideUnused=False):
    flags={'color': 248}

    authItems = Items(**flags) \
            .add('actor', 'permission')

    eventItems = Items(**flags).line() \
            .add('code', 'event').line() \
            .add('args', items=Items(**flags)).line() \
            .add('data', hide=True).line()

    actionItems = Items(**flags).line() \
            .add('receiver', 'code', 'action').line() \
            .add('auth', items=Items().others(items=authItems)).line() \
            .add('args', items=Items(**flags)).line() \
            .add('data', hide=True).line() \
            .add('events', items=Items(hide=hideUnused, items=eventItems)).line()

    applyTrxItems = Items().line() \
            .add('msg_channel', 'msg_type').line() \
            .add('id').line() \
            .add('block_num', 'block_time').line() \
            .add('actions', items=Items(hide=hideUnused, items=actionItems)).line()

    return applyTrxItems


def initAcceptBlockItems():
    flags={'color': 248}

    acceptBlockItems = Items(**flags).line() \
            .add('msg_channel', 'msg_type').line() \
            .add('id').line() \
            .add('block_num', 'block_time').line() \
            .add('producer', 'producer_signature').line() \
            .add('previous').line() \
            .add('dpos_irreversible_blocknum', 'block_slot', 'scheduled_shuffle_slot', 'scheduled_slot').line() \
            .add('active_schedule').line() \
            .add('next_schedule').line() \
            .add('next_block_time').line() \
            .add('trxs').line() \
            .add('events', 'block_extensions').line()

    return acceptBlockItems


def initTrxItems():
    authItems = Items() \
            .add('actor', color=137).add('permission', color=95)
    
    actionItems = Items().line() \
            .add('account', 'name', color=20).line() \
            .add('authorization', color=128, items=Items().others(atnewline=False, items=authItems)).line() \
            .add('args', color=34, _items=None, optional=True).line() \
            .add('data', hide=False, max_length=16, color=248).line() \
            .others(atnewline=False, color=250)
    
    trxItems = Items().line() \
            .add('delay_sec', 'expiration').line() \
            .add('ref_block_num', 'ref_block_prefix').line() \
            .add('max_net_usage_words', 'max_cpu_usage_ms', 'max_ram_kbytes', 'max_storage_kbytes').line() \
            .add('actions', items=Items().others(atnewline=False, items=actionItems)).line() \
            .add('context_free_actions', 'context_free_data').line() \
            .add('transaction_extensions').line() \
            .add('signatures').line()

    return trxItems



