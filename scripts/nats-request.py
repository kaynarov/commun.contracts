#!/usr/bin/python3

# Script to get data from nats streaming server.
# This script allow to detect sequence number of last message from file with
# messages and continue download data from last disconnect.
# Also this script reorder message in correct order.
# This scripts use `stan-sub` from https://github.com/nats-io/cnats/archive/v1.8.0.tar.gz

import subprocess
import fileinput
import atexit
import shlex
import sys
import re
import os

pattern = re.compile('^Received on \[Blocks\]: sequence:([0-9]+) ')
lines = {}
def processLine(line):
    global lastSeq
    result = pattern.findall(line)
    if len(result) == 0:
        sys.stderr.write(line)
        return

    seq = int(result[0])
    if lastSeq is None:
        lastSeq = seq-1

    if seq == lastSeq+1:          # correct order
        sys.stdout.write(line)
        lastSeq = seq
    elif seq <= lastSeq:          # duplicate
        return
    else:                         # future
        lines[seq] = line

    while lastSeq+1 in lines:
        sys.stdout.write(lines[lastSeq+1])
        del lines[lastSeq+1]
        lastSeq += 1

    if len(lines) > 10000:
        removed = 0
        for key in sorted(lines.keys()):
            if key <= lastSeq:
                del lines[key]
                removed += 1
            else:
                break
        if removed == 0:
            sys.stderr.write("Too many future data\n")
            exit(1)

if len(sys.argv) < 2:
    print("Use: <nats-server> [<data-file>]", file=sys.stderr)
    exit(1)
natsServer = sys.argv[1]
dataFilename = sys.argv[2] if 2 < len(sys.argv) else None

lastSeq = None
if dataFilename and dataFilename.startswith('sequence='):
    lastSeq = int(dataFilename[9:])
elif dataFilename and os.path.exists(dataFilename):
    result = subprocess.check_output('tail -n 10 %s'%dataFilename, shell=True, universal_newlines=True)
    for line in result.split('\n'):
        res = pattern.findall(line)
        seq = int(res[0]) if len(res) else None
#        print(line, res, seq)
        if seq and (lastSeq is None or seq > lastSeq):
            lastSeq = seq
    if lastSeq is None:
        print("Can't detect last sequence", file=sys.stderr)
        exit(1)

print("Last sequence:", lastSeq, file=sys.stderr)

def closenats(nats):
    nats.terminate()

if natsServer.startswith('nats://'):
    cmd = './stan-sub -s {host} -print {start} -c cyberway -subj Blocks'.format(host=natsServer, start='-seq %d'%lastSeq if lastSeq else '')
    nats = subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE)
    atexit.register(closenats, nats)
    while nats.poll() is None:
        line = nats.stdout.readline()
        line = line.decode('utf-8', errors='backslashreplace')
        if line:
            processLine(line)
else:
    for line in open(natsServer,'rt'):
        processLine(line)

