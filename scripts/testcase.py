import unittest
import eehelper as ee
import testnet

import json
from copy import deepcopy, copy
from jsonprinter import Item, Items, JsonPrinter
from console import Console, ColoredConsole


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


gApplyTrxItems = initApplyTrxItems()
gAcceptBlockItems = initAcceptBlockItems()
gTrxItems = initTrxItems()


class TestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        TestCase.initialAccounts = None
        cls.accounts = {}
        pass

    @classmethod
    def tearDownClass(cls):
        pass

    class AccountReplacer:
        def __init__(self, testcase):
            self.testcase = testcase

        def replace(self, string):
            return self.testcase.accounts.get(string, None)

    class TrxLogger(testnet.TrxLogger):
        def __init__(self, testcase):
            self.testcase = testcase

        def formatCleosCmd(self, cmd, **kwargs): self.testcase.formatCleosCmd(cmd, **kwargs)
        def formatTrx(self, trx): self.testcase.formatTrx(trx)
        def formatReceipt(self, receipt): self.testcase.formatReceipt(receipt)
        def formatError(self, error): self.testcase.formatError(error)

    def setUp(self):
        if TestCase.initialAccounts is None:
            TestCase.initialAccounts = copy(self.accounts)
        else:
            self.accounts = copy(TestCase.initialAccounts)

        self.console = ColoredConsole()
        self.jsonPrinter = JsonPrinter(self.console, replacer=self.AccountReplacer(self))
        self.eeHelper = ee.EEHelper(self, eventFormatter=self)

        self.savedTrxLogger = testnet.trxLogger
        testnet.trxLogger = self.TrxLogger(self)

    def tearDown(self):
        testnet.trxLogger = self.savedTrxLogger
        self.eeHelper.tearDown()

    def formatCleosCmd(self, cmd, output=False):
        print("cleos: " + cmd)

    def formatTrx(self, data):
        trx = json.loads(data)
        for action in trx['actions']:
            action['args'] = testnet.unpackActionData(action['account'], action['name'], action['data'])
        print("Send transaction:\n", self.jsonPrinter.format(gTrxItems, trx))

    def formatReceipt(self, data):
        receipt = json.loads(data)
        items = Items() \
                .add('transaction_id') \
                .add('processed', items=Items().add('block_num', 'block_time').others(hide=True)) \
                .others(hide=True)
        print("Successfully executed:", self.jsonPrinter.format(items, receipt))

    def formatError(self, error):
        print("Executed with error:")
        if isinstance(error, testnet.CleosException):
            print('    ', error.output.replace('\n', '\n    '), sep='')
        else:
            print('    ', error.replace('\n', '\n    '), sep='')
    
    def _processItem(self, mItem, items, predicat, data, mapping):
        subitem = items[mItem.index]
        if mItem.mapping is None:
            subitem.setFlags(color=92,hide=None)
        else:
            itms = subitem.get('items',None)
            if itms is None: itms = Items()
            else: itms = deepcopy(itms)
            subitem.setFlags(items=itms,hide=None,color=0)
            self._processMapping(itms, None, data[mItem.index], mItem.mapping)

    def _processMapping(self, items, predicat, data, mapping):
        if isinstance(mapping, dict):
            for key,mItem in mapping.items():
                self._processItem(mItem, items, predicat, data, mapping)
        elif isinstance(mapping, list):
            for mItem in mapping:
                self._processItem(mItem, items, predicat, data, mapping)
        else: raise Exception("Unknown mapping type %s"%type(mapping))

    def formatEvent(self, i, selector, predicat, msg, mapping):
        items = deepcopy({
                'ApplyTrx': gApplyTrxItems,
                'AcceptBlock': gAcceptBlockItems,
            }.get(msg['msg_type'], None))
        self._processMapping(items, predicat, msg, mapping)
        print(self.jsonPrinter.format(items, msg))

