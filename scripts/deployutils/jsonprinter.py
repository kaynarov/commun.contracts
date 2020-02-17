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
            return self.__formatDict(items, data, intend)
        elif isinstance(data, list):
            return self.__formatList(items, data, intend)
        else:
            if isinstance(data, str):
                _data = None
                if self.replacer:
                    _data = self.replacer.replace(data)
                if _data: _data = '<%s>'%_data
                elif self.maxstrlength:
                    _data = json.dumps(data if len(data) <= self.maxstrlength else data[0:self.maxstrlength-3]+'...')
                else: _data = json.dumps(data)
            else: _data = json.dumps(data)
            return self.console.textStyle(self.console.BOLD) + _data + self.console.textStyle()

    def __formatDict(self, items, data, intend):
        result = '{'
        firstItem = True
        hasHidden = False
        printed = set()
        for name,flags in items.ITEMS.items():
            if name in data:
                if not flags.get('hide', False):
                    result += self.__printItem('"%s": '%name, data[name], intend+self.shift*' ', flags, firstItem=firstItem)
                    firstItem = False
                else: hasHidden = True
                printed.add(name)
            elif not flags.get('optional', False):
                result += self.__printItem('"%s": '%name, '???', intend+self.shift*' ', flags, backColor=202, firstItem=firstItem)
                firstItem = False
            else:
                pass


        for name,value in data.items():
            if name in printed: continue
            if not items.default.get('hide', False):
                result += self.__printItem('"%s": '%name, data[name], intend+self.shift*' ', items.default, firstItem=firstItem)
                firstItem = False
            else: hasHidden = True; break

        if hasHidden: result += '...'
        if items.newline: result += '\n'+intend
        result += '}'
        return result

    def __formatList(self, items, data, intend):
        result = '['
        firstItem = True
        for i,item in enumerate(data):
            flags = items.ITEMS.get(i, items.default)
            if not flags.get('hide', False):
                result += self.__printItem('', item, intend+self.shift*' ', flags, firstItem=firstItem)
                firstItem = False

        if items.newline: result += '\n'+intend
        result += ']'
        return result

    def __printItem(self, name, value, intend, flags, backColor=None, firstItem=False):
        result = ''
        if not firstItem: result += ', '
        if flags.get('atnewline', False): result += '\n'+intend

        color = flags.get('color', None)
        items = flags.get('items', Items())
        if backColor: result += self.console.backColor(backColor)
        if color: result += self.console.textColor(color)
        if name: result += name
        result += self.format(items, value, intend)
        if backColor: result += self.console.backColor()
        if color: result += self.console.textColor()
        return result

