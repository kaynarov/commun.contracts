#!/usr/bin/python3

import unittest
import time
import json
from deployutils.testnet import *
import deployutils.eehelper as ee

techKey    ='5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3'

class EETestCase(unittest.TestCase):
    def setUp(self):
        self.eeHelper = ee.EEHelper(self)

    def tearDown(self):
        self.eeHelper.tearDown()

    def test_userSendTransaction(self):
        actor = 'tech'
        result = pushAction('cyber', 'checkwin', actor, [], keys=techKey)
        print("Pushed transaction with %s" % result['transaction_id'])
        self.assertEqual(result['processed']['receipt']['status'], 'executed')

        trx_id = result['transaction_id']
        block_num = result['processed']['block_num']

        actionData = {
                "account": "cyber",
                "name": "checkwin",
                "authorization": [{"actor": actor, "permission": "active"}],
                "data": {},
            }
        actionTrx = {'actions':ee.AllItems(actionData)}
        actionTrace = {
            "receiver": "cyber",
            "code": "cyber",
            "action": "checkwin",
            "auth": [{"actor": "tech", "permission": "active"}],
            "args": {},
            "events": []
        }
        self.eeHelper.waitEvents(
            [ ({'msg_type':'AcceptTrx', 'id':trx_id},                          {'accepted':True, 'implicit':False, 'scheduled':False, 'trx':actionTrx}),
              ({'msg_type':'ApplyTrx', 'id':trx_id},                           {'block_num':block_num, 'actions':ee.AllItems(actionTrace), 'except':ee.Missing()}),
              ({'msg_type':'AcceptBlock', 'block_num':block_num},              {'trxs':ee.Unorder({'id':trx_id, 'status':'executed'})})
            ], block_num)


if __name__ == '__main__':
    unittest.main()
