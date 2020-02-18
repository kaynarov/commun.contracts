import sys
import json
import unittest

from copy import deepcopy, copy

from .testnet import *
from .eehelper import EEHelper
from .prettylog import log_action,PrefixWriter
from .jsonprinter import Items, JsonPrinter
from .console import Console, ColoredConsole


gApplyTrxItems = initApplyTrxItems()
gAcceptBlockItems = initAcceptBlockItems()
gTrxItems = initTrxItems()


class TestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.savedStdOut = sys.stdout
        sys.stdout = PrefixWriter(sys.stdout)
        TestCase.initialAccounts = None
        cls.accounts = {}
        pass

    @classmethod
    def tearDownClass(cls):
        sys.stdout = cls.savedStdOut
        pass

    def run(self, result=None):
        with log_action(str(self)):
            desc = self.shortDescription()
            if desc: print(desc)
            super().run(result)

    class AccountReplacer:
        def __init__(self, testcase):
            self.testcase = testcase

        def replace(self, string):
            return self.testcase.accounts.get(string, None)

    class TrxLogger(TrxLogger):
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
        self.eeHelper = EEHelper(self, eventFormatter=self)

        self.savedTrxLogger = setTrxLogger(self)

    def tearDown(self):
        setTrxLogger(self.savedTrxLogger)
        self.eeHelper.tearDown()

    def formatCleosCmd(self, cmd, output=False):
        print("cleos: " + cmd)

    def formatTrx(self, data):
        trx = json.loads(data)
        for action in trx['actions']:
            action['args'] = unpackActionData(action['account'], action['name'], action['data'])
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
        if isinstance(error, CleosException):
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

