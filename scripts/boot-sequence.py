#!/usr/bin/env python3
import os
import sys
import subprocess

default_contracts_dir = '/opt/cyberway/bin/data-dir/contracts/'
nodeos_url = os.environ.get('CYBERWAY_URL', 'http://nodeosd:8888')

os.environ['CYBERWAY_URL'] = nodeos_url
os.environ['CLEOS'] = '/opt/cyberway/bin/cleos'

args = {
    'basedir': os.path.dirname(os.path.dirname(os.path.realpath(__file__))),
    'cleos':'/opt/cyberway/bin/cleos --url=%s ' % nodeos_url,
    'public_key':'GLS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV',
    'private_key':'5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3',
    'cyber_private_key':'5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3',
    'commun_contracts_dir': os.environ.get('COMMUN_CONTRACTS', default_contracts_dir),
}

commun_boot_sequence=('{basedir}/scripts/commun-boot-sequence.py '
                     '--contracts-dir "{commun_contracts_dir}" ' 
                     '--private-key {private_key} ').format(**args)
if subprocess.call(commun_boot_sequence, shell=True):
    print('commun-boot-sequence.py exited with error')
    sys.exit(1)

community_params = {
    'owner_account': 'tech.cats',
    'community_name': 'cats',
    'maximum_supply': '1000000000.000 CATS',
    'reserve_amount': '1000000.0000 COMMUN',
    'cw': 10000,
    'fee': 100,
    'annual_emission_rate': 1000,
    'leader_reward_prop': 2000,
}
community_args = ''
for (key, value) in community_params.items():
    community_args += ' --{arg} "{value}"'.format(arg=key.replace('_', '-'), value=value)

community_boot_sequence=('{basedir}/scripts/community-boot-sequence.py '
                     '--creator-account tech '
                     '--commun-private-key {private_key} '
                     '--creator-private-key {private_key} '
                     '--owner-private-key {private_key} '
                     '--owner-public-key {public_key} '
                     + community_args).format(**args)
print(community_boot_sequence)
if subprocess.call(community_boot_sequence, shell=True):
    print('community-boot-sequence.py exited with error')
    sys.exit(1)

sys.exit(0)
