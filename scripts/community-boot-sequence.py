#!/usr/bin/env python3

import argparse
import os
import subprocess
import sys
from testnet import *
import community

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

# Create community owner account
if not args.creator_account is None:
    createAccount(
        args.creator_account,
        args.owner_account,
        args.owner_public_key,
        keys=args.creator_private_key)

community.createCommunity(
    args.community_name,
    args.owner_account,
    Asset(args.maximum_supply),
    Asset(args.reserve_amount),
    commun_private_key=args.commun_private_key,
    owner_private_key=args.owner_private_key,
    cw=args.cw,
    fee=args.fee,
    annual_emission_rate=args.annual_emission_rate,
    leader_reward_prop=args.leader_reward_prop)
