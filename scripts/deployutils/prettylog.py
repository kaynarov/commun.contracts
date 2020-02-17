from contextlib import contextmanager
import sys

@contextmanager
def log_action(msg=None, *, prefix='|  '):
    try:
        if msg: print('/=== {} ==='.format(msg))
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
