#!/usr/bin/python3

import json
import fileinput
import hashlib

def tracery(mssg_id):
    a = hashlib.sha256('{author}/{permlink}'.format(**mssg_id).encode()).hexdigest()
    return int(a[14:16] + a[12:14] + a[10:12] + a[8:10] + a[6:8] + a[4:6] + a[2:4] + a[0:2], 16)


def cyber_updateauth(action):
    args = action['args']
    auth = "%d " % args['auth']['threshold']
    for account in args['auth']['accounts']:
        auth += " %s@%s/%d" % (account['permission']['actor'], account['permission']['permission'], account['weight'])
    return "%s@%s %s" % (args['account'], args['permission'], auth)


trxAccepts = {}
trxs = {}

def acceptTrx(msg):
    trxAccepts[msg['id']] = msg['accepted']


class Contract():
    def __init__(self, actions, events):
        self.actions = actions
        self.events = events

    def processAction(self, data):
        s = self.actions.get(data['action'], None)
        if isinstance(s, str):
            return s.format(**data['args'])
        elif s:
            return s(data)
        else:
            return None

    def processEvent(self, data):
        s = self.events.get(data['event'], None)
        return s.format(**data['args']) if s else None


class GalleryContract(Contract):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def processAction(self, data):
        if data['action'] in ('create', 'upvote', 'downvote', 'unvote'):
            data['args']['tracery'] = tracery(data['args']['message_id'])
        return super().processAction(data)


contracts = {
    'cyber': Contract({
            'onblock':      ' ',
            'providebw':    '{provider} for {account}',
            'updateauth':   cyber_updateauth,
            'linkauth':     '{account}  {code}:{type}  {requirement}',
            'newaccount':   '{creator}  {name}',
        },{
        }),
    'cyber.domain': Contract({
            'newusername':  '{creator}  {owner}={name}',
        },{
        }),
    'cyber.govern': Contract({
            'onblock':      '{producer}',
        },{
        }),
    'cyber.token': Contract({
            'issue':        '{quantity} --> {to}  "{memo}"',
            'transfer':     '{from} --({quantity})--> {to}  "{memo}"',
            'open':         '{owner}  {symbol}  {ram_payer}',
        },{
            'currency':     'supply: {supply},  max_supply: {max_supply}',
            'balance':      '{account}:  {balance}  {payments}',
        }),
    'c.point': Contract({
            'issue':        '{quantity} --> {to}  "{memo}"',
            'transfer':     '{from} --({quantity})--> {to}  "{memo}"',
            'open':         '{owner}  {commun_code}  {ram_payer}',
        },{
            'currency':     'supply: {supply},  reserve: {reserve}',
            'balance':      '{account}:  {balance}',
            'exchange':     '{amount}',
            'fee':          '{amount}',
        }),
    'c.ctrl': Contract({
            'voteleader':   '{commun_code}  {voter}  {leader}  {pct}',
            'changepoints': '{who}  {diff}',
        },{
            'leaderstate':  '{commun_code}  {leader}  {weight}  {active}',
        }),
    'c.emit': Contract({
            'issuereward':  '{commun_code} for {to_contract}',
        },{
        }),
    'c.list': Contract({
            'follow':       '{commun_code}  {follower}',
        },{
        }),
    'c.gallery': GalleryContract({
            'create':       '{commun_code}  {message_id[author]}/{message_id[permlink]} ~{tracery}  {weight}',
            'upvote':       '{commun_code}  {voter}  {message_id[author]}/{message_id[permlink]} ~{tracery}  {weight}',
            'downvote':     '{commun_code}  {voter}  {message_id[author]}/{message_id[permlink]} ~{tracery}  {weight}',
            'unvote':       '{commun_code}  {voter}  {message_id[author]}/{message_id[permlink]} ~{tracery}',
        },{
            'gemstate':     '{creator}/{owner} ~{tracery}  {shares}  {points}',
            'mosaicstate':  '{creator} ~{tracery}  {gem_count} gems,  {shares}/{damn_shares} shares,  {reward},  {collection_end_date} end collection',
            'gemchop':      '{creator}/{owner} ~{tracery}  {reward}  {unfrozen}',
            'mosaicchop':   '{commun_code}  {tracery}',
            'mosaictop':    '{commun_code}  {tracery}  {place}  {comm_rating}/{lead_rating}',
            'inclstate':    '{account}  {quantity}',
        })
}


def applyTrx(msg):
#    if len(msg['actions']) > 1:
#        firstAction = msg['actions'][0]
#        if firstAction['receiver'] == 'cyber' and firstAction['action'] == 'onblock':
#            return
    result = '?'
    if msg['id'] in trxAccepts:
        result = "OK" if trxAccepts[msg['id']] else "FAIL"
    info = "----------- Apply transaction in block %d:  %s  (%-4s)  %s" % (msg['block_num'], msg['id'], result, msg['block_time'])
    block_time = '{m}/{d} {t}'.format(m=msg['block_time'][5:7], d=msg['block_time'][8:10], t=msg['block_time'][11:-4])
    prefix = '%s %-4s'%(block_time, result)
    for action in msg['actions']:
        header = "%-13s  %s:%s"%(action['receiver'], action['code'], action['action'])
        contract = contracts.get(action['code'], None)
        data = contract.processAction(action) if contract else None
        info += "\n%s   %-40s  %s" % (prefix, header, data if data else "+++ "+str(action['args']))
        for event in action['events']:
            hdr = "%15c   %s:%s"%(' ', event['code'], event['event'])
            contract = contracts.get(event['code'], None)
            data = contract.processEvent(event) if contract else None
            info += "\n%s   %-43s  %s" % (prefix, hdr, data if data else "--- "+str(event['args']))

    trxs[msg['id']] = info

def acceptBlock(msg):
    print("================= Accept block: %d ===================" % msg['block_num'])
    for trx in msg['trxs']:
        print(trxs[trx['id']])
    pass

def commitBlock(msg):
    pass

process_funcs = {'AcceptTrx': acceptTrx,
                 'ApplyTrx': applyTrx,
                 'AcceptBlock': acceptBlock,
                 'CommitBlock': commitBlock}

def process_line(line):
    first = line.find('{')
    last = line.rfind('}')
    if first != -1 and last != -1:
        line = line[first:last+1]

    if line[0] != '{':
        return
    try:
        msg = json.loads(line)
    except:
        print(line)
        exit(1)
    func = process_funcs.get(msg['msg_type'], None)
    if func != None:
        func(msg)
    #print("Msg: %s" % msg['msg_type'])

for line in fileinput.input():
    process_line(line)

