import json

class Items:
    def __init__(self):
        self.items = []
        self.newline = False
        self.otherFlags = {}

    def add(self, *items, **kwargs):
        for name in items:
            if self.newline:
                self.newline = False
                self.__append(name, **kwargs, atnewline=True)
            else:
                self.__append(name, **kwargs)
        return self

    def __append(self, name, **kwargs):
        self.items.append((name, kwargs,))

    def others(self, **kwargs):
        self.otherFlags = kwargs
        return self

    def line(self):
        self.newline = True
        return self


class JsonPrinter:
    def __init__(self, console, shift=4, maxstrlength=64, replacestr={}, replacer=None, defaultItem=None):
        self.console=console
        self.shift=shift
        self.maxstrlength=maxstrlength
        self.replacestr=replacestr
        self.replacer=replacer
        self.defaultItem=defaultItem or Items()

    def format(self, items, data, intend=''):
        if items is None: items = self.defaultItem
        if isinstance(data, dict):
            self.__formatDict(items, data, intend)
        elif isinstance(data, list):
            self.__formatList(items, data, intend)
        else:
            if isinstance(data, str):
                if self.replacer:
                    _data = self.replacer.replace(data)
#               if data in self.replacestr:
#                   _data = '<%s>'%self.replacestr[data]
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
        for name,flags in items.items:
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
            self.__printItem('"%s": '%name, data[name], intend+self.shift*' ', items.otherFlags, firstItem=firstItem)
            firstItem = False

        if items.newline: self.console.newline(intend)
        self.console.print('}')

    def __formatList(self, items, data, intend):
        self.console.print('[')
        for i,item in enumerate(data):
            self.__printItem('', item, intend+self.shift*' ', items.otherFlags, firstItem=(i==0))

        if items.newline: self.console.newline(intend)
        self.console.print(']')

    def __printItem(self, name, value, intend, flags, firstItem=False):
        if not firstItem: self.console.print(', ')
        if flags.get('atnewline', False): self.console.newline(intend)

        color = flags.get('color', None)
        items = flags.get('items', None)
        if color: self.console.textColor(color)
        self.console.print(name)
        self.format(items, value, intend)
        if color: self.console.textColor()

