class Console:
    REGULAR=0
    BOLD=1
    ITALIC=3
    UNDERLINE=4
    STRIKE=9

    def textColor(self, color=None): return ''
    def backColor(self, color=None): return ''
    def textStyle(self, style=None): return ''

class ColoredConsole(Console):
    def __init__(self):
        self.textColors = []
        self.backColors = []
        self.textStyles = []

    def __resetAttributes(self):
        text = '\033[0m'
        if len(self.textColors): text += self.__textColor(self.textColors[-1])
        if len(self.backColors): text += self.__backColor(self.backColors[-1])
        if len(self.textStyles) and self.textStyles[-1] != self.REGULAR:
            text += self.__textStyle(self.textStyles[-1])
        return text

    def __textColor(self, color): return '\033[38;5;{color}m'.format(color=color)
    def __backColor(self, color): return '\033[48;5;{color}m'.format(color=color)
    def __textStyle(self, style): return '\033[{style}m'.format(style=style)

    def textColor(self, color=None):
        if color:
            self.textColors.append(color)
            return self.__textColor(color)
        else:
            del self.textColors[-1]
            if len(self.textColors): return self.__textColor(self.textColors[-1])
            else: return self.__resetAttributes()

    def backColor(self, color=None):
        if color:
            self.backColors.append(color)
            return self.__backColor(color)
        else:
            del self.backColors[-1]
            if len(self.backColors): return self.__backColor(self.backColors[-1])
            else: return self.__resetAttributes()

    def textStyle(self, style=None):
        if style:
            self.textStyles.append(style)
            if style != self.REGULAR: return self.__textStyle(style)
            else: return __resetAttributes()
        else:
            del self.textStyles[-1]
            if len(self.textStyles) and self.textStyles[-1] != self.REGULAR:
                return self.__textStyle(self.textStyles[-1])
            else: return self.__resetAttributes()

