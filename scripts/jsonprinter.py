import json
from collections import OrderedDict
from copy import deepcopy

class Item(object):
    def __init__(self, owner, name):
        self.owner = owner
        self.name = name

    def __str__(self):
        return 'Item({owner}, {name})'.format(**self.__dict__)

    def __getitem__(self, name):
        if self.name in self.owner.items:
            return self.owner.ITEMS[self.name][name]
        else:
            try:
                return self.owner.default[name]
            except KeyError:
                print("key {key} missing in {default}".format(key=name, default=str(self.owner.default)))
                raise

    def __setitem__(self, name, value):
        return self.owner.setFlags(self.name, **{name: value})

    def getFlags(self):
        return self.owner.ITEMS.get(self.name, self.owner.default)

    def setFlags(self, **kwargs):
        self.owner.setFlags(self.name, **kwargs)

    def get(self, name, default):
        return self.getFlags().get(name, default)


class Items(object):
    def __init__(self, **kwargs):
        self.ITEMS = OrderedDict()
        self.newline = False
        self.default = kwargs
        self.__checkFlags(**kwargs)

#    def __checkFlags(self, color=None, atnewline=None, hide=None, items=None):
    def __checkFlags(self, **kwargs):
        pass

    def __getitem__(self, name):
        return Item(self, name)

    def add(self, *names, **kwargs):
        for name in names:
            if self.newline: self.setFlags(name, **kwargs, atnewline=True)
            else: self.setFlags(name, **kwargs)
            self.newline = False
        return self

    def setFlags(self, name, **flags):
        if not name in self.ITEMS: self.ITEMS[name] = self.default
        if len(flags):
            if id(self.ITEMS[name]) == id(self.default):
                self.ITEMS[name] = deepcopy(self.default)
            item = self.ITEMS[name]
            for k,v in flags.items():
                if v == None:
                    if k in item: del item[k]
                else: item[k] = v
            return item
        else: return self.ITEMS[name]

    def others(self, **kwargs):
        self.default = kwargs
        return self

    def line(self):
        self.newline = True
        return self


class JsonPrinter:
    def __init__(self, console, shift=4, maxstrlength=64, replacer=None):
        self.console=console
        self.shift=shift
        self.maxstrlength=maxstrlength
        self.replacer=replacer

    def format(self, items, data, intend=''):
        if isinstance(data, dict):
            self.__formatDict(items, data, intend)
        elif isinstance(data, list):
            self.__formatList(items, data, intend)
        else:
            if isinstance(data, str):
                if self.replacer:
                    _data = self.replacer.replace(data)
                if _data: _data = '<%s>'%_data
                elif self.maxstrlength:
                    _data = json.dumps(data if len(data) <= self.maxstrlength else data[0:self.maxstrlength-3]+'...')
                else: _data = json.dumps(data)
            else: _data = json.dumps(data)
            self.console.textStyle(self.console.BOLD)
            self.console.print(_data)
            self.console.textStyle()

    def __formatDict(self, items, data, intend):
        self.console.print('{')
        firstItem = True
        printed = set()
        for name,flags in items.ITEMS.items():
            if name in data:
                if not flags.get('hide', False):
                    self.__printItem('"%s": '%name, data[name], intend+self.shift*' ', flags, firstItem=firstItem)
                    firstItem = False
                printed.add(name)
            elif not flags.get('optional', False):
                self.console.backColor(202)
                self.__printItem('"%s": '%name, '???', intend+self.shift*' ', flags, firstItem=firstItem)
                self.console.backColor()
                firstItem = False
            else:
                pass


        for name,value in data.items():
            if name in printed: continue
            self.__printItem('"%s": '%name, data[name], intend+self.shift*' ', items.default, firstItem=firstItem)
            firstItem = False

        if items.newline: self.console.newline(intend)
        self.console.print('}')

    def __formatList(self, items, data, intend):
        self.console.print('[')
        firstItem = True
        for i,item in enumerate(data):
            flags = items.ITEMS.get(i, items.default)
            if not flags.get('hide', False):
                self.__printItem('', item, intend+self.shift*' ', flags, firstItem=firstItem)
                firstItem = False

        if items.newline: self.console.newline(intend)
        self.console.print(']')

    def __printItem(self, name, value, intend, flags, firstItem=False):
        if not firstItem: self.console.print(', ')
        if flags.get('atnewline', False): self.console.newline(intend)

        color = flags.get('color', None)
        items = flags.get('items', Items())
        if color: self.console.textColor(color)
        self.console.print(name)
        self.format(items, value, intend)
        if color: self.console.textColor()

