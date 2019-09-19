import os
import json
import subprocess

params = {
    'cleos_path': os.environ.get("CLEOS", "cleos"),
    'nodeos_url': os.environ.get("CYBERWAY_URL", "http://localhost:8888"),
}
cleosCmd = "{cleos_path} --url {nodeos_url} ".format(**params)

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
        else:
            super().default(self, value)

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

def createAccount(creator, account, key, *, providebw=None, keys=None):
    return cleos('create account {creator} {acc} {key}'.format(creator=creator, acc=account, key=key), providebw=providebw, keys=keys)

def getAccount(account):
    return json.loads(cleos('get account -j {acc}'.format(acc=account)))

def updateAuth(account, permission, parent, keys, accounts):
    return pushAction('cyber', 'updateauth', account, {
        'account': account,
        'permission': permission,
        'parent': parent,
        'auth': createAuthority(keys, accounts)
    })

def linkAuth(account, code, action, permission):
    return cleos('set action permission {acc} {code} {act} {perm}'.format(acc=account, code=code, act=action, perm=permission))

def transfer(sender, recipient, amount, memo="", *, providebw=None, keys=None):
    return pushAction('cyber.token', 'transfer', sender, {'from':sender, 'to':recipient, 'quantity':amount, 'memo':memo}
        , providebw=providebw, keys=keys)
