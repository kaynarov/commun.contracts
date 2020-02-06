_100percent = 10000
int64_max = 2**63 - 1
uint64_max = 2**64 - 1

fract_digits = {
    'fixp': 12,
    'elaf': 62
}
fixp_max = 2**(63 - fract_digits['fixp']) - 1

import hashlib
import bson
from decimal import Decimal
# from bson import decimal128
from config import BALANCE_PRECISION


def Int64(value):
    return bson.Int64(value)

def UInt64(value):
    return bson.decimal128.Decimal128(Decimal(value))

def Int128(value):
    val = value
    if val < 0:
        val += (1<<128)
    return {
        "binary": bson.Binary((0).to_bytes(1,"big") + val.to_bytes(16,"big")),
        "string": "%d"%value
    }

def UInt128(value):
    return {
        "binary": bson.Binary((1).to_bytes(1,"big") + value.to_bytes(16,"big")),
        "string": "%d"%value
    }


def Decimal128(value):
    return bson.decimal128.Decimal128(Decimal(value))

def get_checked_int64(arg):
    if arg > int64_max:
        raise OverflowError('get_checked_int64 error', arg)
    return int(arg);
    
def uint64_to_int64(arg):
    if arg > uint64_max:
        raise OverflowError('uint64_to_int64 error', arg)
    return arg if arg <= int64_max else -(2**64 - arg)

def get_fixp_raw(arg):
    return get_checked_int64(arg * pow(2, fract_digits['fixp']))

def get_prop_raw(arg):
    return get_checked_int64((arg * pow(2, fract_digits['elaf'])) / _100percent)
    
def get_golos_asset_amount(arg):
    return get_checked_int64(arg * pow(10, BALANCE_PRECISION))
    
def convert_hash(param):
    a=hashlib.sha256(param.encode('ascii')).hexdigest()[:16]
    return int(a[14:16] + a[12:14] + a[10:12] + a[8:10] + a[6:8] + a[4:6] + a[2:4] + a[0:2], 16)

def printProgressBar (iteration, total, prefix = '', suffix = '', decimals = 1, length = 100, fill = '█'):
    percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
    filledLength = int(length * iteration // total)
    bar = fill * filledLength + '-' * (length - filledLength)
    print('\r%s |%s| %s%% %s' % (prefix, bar, percent, suffix), end = '\r')
    if iteration == total:
        print()

def uint_to_hex_str(val, n):
    return '0x' + hex(val)[2:].zfill(n)
     
def char_to_symbol(c):
  if (c >= ord('a') and c <= ord('z')):
    return (c - ord('a')) + 6
  if (c >= ord('1') and c <= ord('5')):
    return (c - ord('1')) + 1;
  return 0;
     
def string_to_name(s):
  len_str = len(s)
  sbytes = str.encode(s)
  value = 0
  
  i = 0
  while (i <= 12):
    c = 0
    
    if (i < len_str and i <= 12):
      c = char_to_symbol(sbytes[i])
    
    if (i < 12):
      c = c & 0x1f
      c = c << 64-5*(i+1)
    else:
      c = c & 0x0f
      
    value = value | c
    i += 1

  return value



from contextlib import contextmanager
import sys
from console import ColoredConsole

console = ColoredConsole()

@contextmanager
def log_action(msg=None, *, prefix='|  '):
    try:
        if msg: print('/{}=== {} ==={}'.format(console.textStyle(console.BOLD), msg, console.textStyle()))
        if isinstance(sys.stdout, PrefixWriter):
            saved_prefix = sys.stdout.prefix
            sys.stdout.prefix += prefix
        yield
    finally:
        if isinstance(sys.stdout, PrefixWriter):
            sys.stdout.prefix = saved_prefix
        if msg: print('\===')

class PrefixWriter:
    def __init__(self, stdout) :
        self.stdout = stdout
        self.needprefix = True
        self.prefix = ''

    def write(self, text):
        if len(text):
            if self.needprefix:
                self.stdout.write(self.prefix)
            if len(self.prefix) and len(text):
                if text[-1] == '\n': text = text[0:-1].replace('\n', '\n'+self.prefix)+'\n'
                else: text = text.replace('\n', '\n'+self.prefix)
            result = self.stdout.write(text)
            self.needprefix = (text[-1] == '\n')
            return result
        else:
            return self.stdout.write(text)

    def flush(self):
        return self.stdout.flush()

