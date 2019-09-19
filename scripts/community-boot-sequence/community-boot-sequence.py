#!/usr/bin/env python3

import argparse
import os
import subprocess
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
from testnet import *


def createCommunity(community_name, owner_account, maximum_supply, reserve_amount, *, cw=10000, fee=100, annual_emission_rate=1000, leader_reward_prop=2000):
    symbol = maximum_supply.symbol

    # 1. Buy some value of COMMUN tokens
    pushAction('cyber.token', 'issue', 'cmmn', {
        'to':owner_account,
        'quantity':reserve_amount,
        'memo':"Reserve for {c}".format(c=community_name)
    }, keys=args.commun_private_key)

    # 2. Create community points (need cmmn permission)
    pushAction('cmmn.point', 'create', 'cmmn.point', {
        'issuer': owner_account,
        'maximum_supply': maximum_supply,
        'cw': cw,
        'fee': fee
    }, providebw='cmmn.point/cmmn', keys=args.commun_private_key)

    # 3. Restock COMMUN tokens for community points
    transfer(owner_account, 'cmmn.point', reserve_amount, 'restock: {code}'.format(code=symbol.code),
        providebw='{acc}/cmmn'.format(acc=owner_account), keys=args.commun_private_key)
    
    # 4. Set ctrl-params (cmmn.ctrl:setparams)
    pushAction('cmmn.ctrl', 'setparams', owner_account, {
        "point":symbol.code,
        "params":[
            ["multisig_acc",{"name":owner_account}],
            ["max_witnesses",{"max":21}],
            ["multisig_perms", {"super_majority":0,"majority":0,"minority":0}],
            ["max_witness_votes",{"max":30}]
        ]
    }, providebw='{acc}/cmmn'.format(acc=owner_account), keys=args.owner_private_key)

    # 5. Create emission (cmmn.emit:create)
    pushAction('cmmn.emit', 'create', 'cmmn.emit', {
        "commun_symbol": symbol,
        "annual_emission_rate": annual_emission_rate,
        "leaders_reward_prop": leader_reward_prop
    }, providebw='cmmn.emit/cmmn', keys=args.commun_private_key)

    # 6. Open point balance for cmmn.gallery
    pushAction('cmmn.point', 'open', 'cmmn', {
        "owner": "cmmn.gallery",
        "symbol": symbol,
        "ram_payer": "cmmn"
    }, keys=args.commun_private_key)

    # 7. Set publication params (cmmn.gallery:setparams) (fix to check issuer permission instead of _self)
    pushAction('cmmn.gallery', 'setparams', 'cmmn.gallery', {
        "commun_code":symbol.code,
        "params":[
            ['st_max_comment_depth', {'value': 127}],
            ['st_social_acc', {'value': 'cmmn.social'}],
        ]
    }, providebw='cmmn.gallery/cmmn', keys=args.commun_private_key)

    # 8. Register community (cmmn.list:create)
    pushAction('cmmn.list', 'create', 'cmmn.list', {
        "token_name": symbol.code,
        "community_name": community_name
    }, providebw='cmmn.list/cmmn', keys=args.commun_private_key)


# Command Line Arguments
parser = argparse.ArgumentParser()

parser.add_argument('--creator-account', metavar='', help="Creator for community owner account", default=None, dest="creator_account")
parser.add_argument('--creator-private-key', metavar='', help="Private key for --creator-account", default=None, dest="creator_private_key")

parser.add_argument('--commun-private-key', metavar='', help="Private key for commun owner", default=None, dest="commun_private_key")

parser.add_argument('--owner-account', metavar='', help="Community owner account", dest="owner_account")
parser.add_argument('--owner-private-key', metavar='', help="Private key for community owner account", dest="owner_private_key")
parser.add_argument('--owner-public-key', metavar='', help="Public key for community owner account", dest="owner_public_key")

parser.add_argument('--community-name', metavar='', help="Community owner account", dest="community_name")
parser.add_argument('--maximum-supply', metavar='', help="Maximum supply of community point", dest="maximum_supply")
parser.add_argument('--reserve-amount', metavar='', help="Reserve amount for community point (in COMMUN tokens)", dest="reserve_amount")
parser.add_argument('--cw', metavar='', help="CW parameter for community point", default=10000, type=int, dest="cw")
parser.add_argument('--fee', metavar='', help="FEE parameter for community point", default=100, type=int, dest="fee")
parser.add_argument('--annual-emission-rate', metavar='', help="Annual emission rate", default=1000, type=int, dest="annual_emission_rate")
parser.add_argument('--leader-reward-prop', metavar='', help="Leader reward proportion", default=2000, type=int, dest="leader_reward_prop")


args = None
args = parser.parse_args()

# 0. Create community owner account
if not args.creator_account is None:
    createAccount(
        args.creator_account,
        args.owner_account,
        args.owner_public_key,
        keys=args.creator_private_key)

createCommunity(
    args.community_name,
    args.owner_account,
    Asset(args.maximum_supply),
    Asset(args.reserve_amount),
    cw=args.cw,
    fee=args.fee,
    annual_emission_rate=args.annual_emission_rate,
    leader_reward_prop=args.leader_reward_prop)
