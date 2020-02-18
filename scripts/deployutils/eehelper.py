#!/usr/bin/python3

# Tests for Event Engine.
# Preconditions:
# - nodeos run with enabled event_engine_plugin
# - event_engine_plugin write events in `events.dump` which located (or symlinked) in current directory

import unittest
import time
import json
from .testnet import *

# Class for save element from events with `name` into `params` dictionary
class Save:
    def __init__(self, params, name):
        self.params = params
        self.name = name

    def __repr__(self):
        return 'Save "%s"' % self.name

class Load:
    def __init__(self, params, name):
        self.params = params
        self.name = name

    def __repr__(self):
        return 'Load "%s" (%s)' % (self.name, self.params.get(self.name))

# Compare array with set of args (without order)
class Unorder:
    def __init__(self, *args):
        self.values = args

    def __repr__(self):
        return str(self.values)

# Compare array with args (all items with specified order)
class AllItems:
    def __init__(self, *args):
        self.values = args

    def __repr__(self):
        return str(self.values)

# Compare template with events exactly
class Exactly:
    def __init__(self, value):
        self.value = value

    def __repr__(self):
        return str(self.value)

class Missing:
    def __init__(self, *args):
        self.values = args

    def __repr__(self):
        return 'Missing'+str(self.values)


class Any():
    pass



def AssertException(data={}):
    return {'code':3050003, 'name': 'eosio_assert_message_exception', 
            'stack':[{'format':'assertion failure with message: ${s}', 'data': data}]
           }
def DeadlineException():
    return {'code':3080004, 'name':'deadline_exception', 'message': 'Transaction took too long'}

def UsageExceededException():
    return {'code':3080003, 'name':'tx_subjective_usage_exceeded', 
            'message':'Transaction subjectively exceeded the current usage limit imposed on the transaction'}

def SendDeferredTrace(action, arg, sender_id, delay, replace, params):
    def_action_in_event = {
        'account':testAccount, 'name':action, 
        'authorization':AllItems({'actor':testAccount, 'permission':'active'}), 
        'data':{'arg':arg}
    }
    return {
        "receiver":testAccount, "code":testAccount, "action":"senddeferred",
        "auth":[{"actor":testAccount,"permission":"active"}],
        "args":{"action":action, "arg":arg, "senderid":sender_id, "delay":delay, "replace":replace},
        "events":AllItems(
            {   'code':'', 'event':'senddeferred',
                'args':{
                    "sender":testAccount, 'sender_id':sender_id,
                    'trx':{'delay_sec':delay, 'context_free_actions':Exactly([]), 'actions':AllItems(def_action_in_event)},
                    "trx_id":Save(params, 'def_trx_id'),
                    'packed_trx':Save(params, 'def_packed_trx')
                },
            }
        )
    }
def CancelDeferTrace(sender_id):
    return {
        "receiver":testAccount, "code":testAccount, "action":"canceldefer",
        "auth":[{"actor":testAccount,"permission":"active"}],
        "args":{"senderid":sender_id},
        "events":AllItems(
            {"code":"", "event":"canceldefer", "args":{"sender":testAccount, 'sender_id':sender_id}}
        )
    }
def ActionTrace(action, args, events=[]):
    return {
        'receiver':testAccount, 'code':testAccount, 'action':action, 
        'auth':[{'actor':testAccount, 'permission':'active'}], 
        'args':args, 'events':events
    }

def ActionData(action, args):
    return {
        'account':testAccount, 'name':action, 
        'authorization':[{'actor':testAccount, 'permission':'active'}], 
        'data':args
    }

class MapItem:
    def __init__(self, index=None):
        self.index = index
        self.mapping = None

    def __repr__(self):
        return '({index}, {mapping})'.format(index=self.index, mapping=str(self.mapping))

class EEHelper():
    def __init__(self, tcase, eventFormatter=None):
        self.tcase = tcase
        self.eventFormatter = eventFormatter
        self.eventsFd = open(os.getenv('EVENTS_FILE',"events.dump"), "r")
        self.resetEvents()

    def tearDown(self):
        self.eventsFd.close()

    def resetEvents(self):
        self.eventsFd.seek(0, 2)

    def assertContains(self, templ, value):
        self._assertContains('', templ, value, MapItem())

    def checkContains(self, template, message):
        try:
            self.assertContains(template, message)
            return True
        except:
            return False

    def waitEvents(self, events, maxBlockNum):
        i = 0
        lines = []
        while i < len(events):
            (selector, predicat) = events[i]
            for line in self.eventsFd:
                lines.append(line)
                msg = json.loads(line)
                if msg['msg_type'] == 'AcceptBlock' and msg['block_num'] > maxBlockNum:
                    self.tcase.fail('Missed events at block %d: %s\n\nReaden lines:\n%s' % (maxBlockNum, events[i:], ''.join(lines)))
                if self.checkContains(selector, msg):
                    mapItem = MapItem()
                    self._assertContains('events[%d]'%i, predicat, msg, mapItem)
                    print('Found message for %d event: %s' % (i, selector))
                    if self.eventFormatter:
                        self.eventFormatter.formatEvent(i, selector, predicat, msg, mapItem.mapping)
                    i += 1
                    lines = []
                    break
            time.sleep(1)

    def _assertContains(self, path, templ, value, mapItem, msg=None):
        if templ is None:
            self.tcase.assertIs(value, None, '%s' % path)
        elif type(templ) is Any:
            pass
        elif type(templ) is Exactly:
            self.tcase.assertEqual(value, templ.value, '%s' % path)
        elif type(templ) is Save:
            templ.params[templ.name] = value
        elif type(templ) is Load:
            self._assertContains(path, templ.params[templ.name], value, msg=msg)
        elif type(templ) is type({}):
            mapItem.mapping = {}
            self.tcase.assertEqual(type(value), type({}), path)
            for key,val in templ.items():
                npath='%s["%s"]'%(path,key)
                if type(val) is Missing:
                    if key in value:
                        mItem = MapItem(key); mapItem.mapping[key] = mItem
                        self.tcase.fail("Key '%s' should be missing in %s' : %s" % (key, value, msg))
                else:
                    mItem = MapItem(key); mapItem.mapping[key] = mItem
                    self.tcase.assertIn(key, value, npath)
                    self._assertContains(npath, val, value[key], mItem)
        elif type(templ) is type([]):
            mapItem.mapping = [None] * len(templ)
            self.tcase.assertEqual(type(value), type([]), path)
            i = 0
            j = 0
            while i < len(templ):
                npath='%s[%d]' % (path, i)
                errors = []
                if type(templ[i]) is Missing:
                  for itm in templ[i].values:
                    for val in value:
                      if self.checkContains(itm, val):
                        self.tcase.fail("Item %s should missing in %s : %s" % (itm, value, path))
                else:
                  while j < len(value):
                    try:
                        mItem = MapItem(j)
                        self._assertContains(npath, templ[i], value[j], mItem)
                        break
                    except AssertionError as err:
                        errors.append(str(err))
                        pass
                    j += 1
                  if j == len(value):
                    self.tcase.fail("%s \ndoesn't contains %s : %s\nChecked items:\n%s" % (json.dumps(value,indent=3), templ[i], path, '\n'.join(errors)))
                mapItem.mapping[i] = mItem
                i += 1
        elif type(templ) is Unorder or type(templ) is AllItems:
            mapItem.mapping = [None] * len(templ.values)
            self.tcase.assertEqual(type(value), type([]), path)
            if type(templ) is AllItems and len(templ.values) != len(value):
                self.tcase.fail("Different items count: template %d, actual %d : %s" % (len(templ.values), len(value), path))
            was = set()
            i = 0
            for t in templ.values:
                npath='%s[%d]' % (path, i)
                j = 0
                errors = []
                if type(t) is Missing:
                  for itm in t.values:
                    for val in value:
                      if self.checkContains(itm, val):
                        self.tcase.fail("Item %s should missing in %s : %s" % (itm, value, path))
                else:
                  while j < len(value):
                    if j not in was:
                        try:
                            mItem = MapItem(j)
                            self._assertContains(npath, t, value[j], mItem)
                            was.add(j)
                            break
                        except AssertionError as err:
                            errors.append(str(err))
                            pass
                    j += 1
                if j == len(value):
                    self.tcase.fail("%s doesn't contains %s : %s\nChecked items:\n%s" % (value, t, path, '\n'.join(errors)))
                mapItem.mapping[i] = mItem
                i += 1
        else:
            self.tcase.assertEqual(type(value), type(templ), '%s' % path)
            self.tcase.assertEqual(value, templ, '%s' % path)
