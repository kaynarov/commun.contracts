#!/usr/bin/env python3

import argparse
import os
import subprocess
import sys
from deployutils.testnet import *
import community

# Command Line Arguments
parser = argparse.ArgumentParser()

parser.add_argument('--creator-auth', metavar='', help="Creator for community owner account", default=None, dest="creator_auth")
parser.add_argument('--creator-key', metavar='', help="Private key for --creator-account", default=None, dest="creator_key")

parser.add_argument('--community-name', metavar='', help="Community owner account", dest="community_name")
parser.add_argument('--maximum-supply', metavar='', help="Maximum supply of community point", dest="maximum_supply")
parser.add_argument('--reserve-amount', metavar='', help="Reserve amount for community point (in CMN tokens)", dest="reserve_amount")
parser.add_argument('--cw', metavar='', help="CW parameter for community point", default=10000, type=int, dest="cw")
parser.add_argument('--fee', metavar='', help="FEE parameter for community point", default=100, type=int, dest="fee")

args = None
args = parser.parse_args()

commun_code = Asset.fromstr(args.reserve_amount).symbol.code

owner = community.createCommunity(
    community_name  = args.community_name,
    creator_auth    = args.creator_auth,
    creator_key     = args.creator_key,
    maximum_supply  = Asset.fromstr(args.maximum_supply),
    reserve_amount  = Asset.fromstr(args.reserve_amount),
    cw              = args.cw,
    fee             = args.fee)

print("Community created: {code} {owner}".format(code=commun_code, owner=owner))
