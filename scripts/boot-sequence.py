#!/usr/bin/env python3
import os
import sys
import subprocess

default_contracts_dir = '/opt/cyberway/bin/data-dir/contracts/'
nodeos_url = os.environ.get('CYBERWAY_URL', 'http://nodeosd:8888')

args = {
    'basedir': os.path.dirname(os.path.dirname(os.path.realpath(__file__))),
    'cleos':'/opt/cyberway/bin/cleos --url=%s ' % nodeos_url,
    'public_key':'GLS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV',
    'private_key':'5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3',
    'cyber_private_key':'5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3',
    'cyberway_contracts_dir': os.environ.get('CYBERWAY_CONTRACTS', default_contracts_dir),
    'commun_contracts_dir': os.environ.get('COMMUN_CONTRACTS', default_contracts_dir),
}

commun_boot_sequence=('{basedir}/scripts/commun-boot-sequence/commun-boot-sequence.py '
                     '--cleos "{cleos}" --contracts-dir "{commun_contracts_dir}" ' 
                     '--public-key {public_key} --private-key {private_key} '
                     '--cyber-private-key {cyber_private_key} '
                     '--docker --all').format(**args)
if subprocess.call(commun_boot_sequence, shell=True):
    print('commun-boot-sequence.py exited with error')
    sys.exit(1)

community_boot_sequence=('{basedir}/scripts/community-boot-sequence/community-boot-sequence.py '
                     '--cleos "{cleos}" --contracts-dir "{commun_contracts_dir}" ' 
                     '--public-key {public_key} --private-key {private_key} '
                     '--cyber-private-key {cyber_private_key} '
                     '--docker --all').format(**args)
if subprocess.call(community_boot_sequence, shell=True):
    print('community-boot-sequence.py exited with error')
    sys.exit(1)

sys.exit(0)
