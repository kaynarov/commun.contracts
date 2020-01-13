#!/usr/bin/python3

import sys
import json
import os
import codecs

sys.stdout = codecs.getwriter('utf-8')(sys.stdout.buffer, 'strict')

cpp_path = sys.argv[1]
include_path = os.path.dirname(os.path.dirname(cpp_path))
fp = open(cpp_path, 'r', encoding="utf-8")
if os.path.basename(include_path) != "include":
    print(fp.read())
    exit()
contract_path = os.path.dirname(include_path)
contract_name = os.path.basename(contract_path)
abi_path = contract_path + "/abi/" + contract_name + ".abi"

if os.path.exists(abi_path):
    with open(abi_path) as abifile:
        abi = json.load(abifile)

event_prefix = "_event {"
table_prefix = '// DOCS_TABLE: '

def print_table(table):
    print("    Table name: <b>" + table["name"] + "</b>")
    print("<br>Scope type: " + (table["scope_type"] if 'scope_type' in table else "name"))
    print("<br>Indexes:")
    for idx in table["indexes"]:
        unique = "unique" if idx["unique"] else "non-unique"
        print("    - <b>" + idx["name"] + "</b>, " + unique + ", Orders:")
        for order in idx["orders"]:
            print("        - <b>" + order["field"] + "</b>, " + order["order"] + "ending")

def process_table(line):
    start = line.find(table_prefix)
    if start == -1:
        return False
    table_name = line[start+len(table_prefix):]
    print("/**")
    for table in abi["tables"]:
        if table["type"] != table_name:
            continue
        print_table(table)
        break
    print("*/")
    return True

def process_event(line):
    if line.find(event_prefix) == 1:
        return False
    print(line.replace(event_prefix, " {"))
    return True

line = fp.readline()
while line:
    line = line.strip()
    if process_table(line):
        pass
    elif process_event(line):
        pass
    else:
        print(line)
    line = fp.readline()
