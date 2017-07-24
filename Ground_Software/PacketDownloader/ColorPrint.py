#!/usr/bin/env python3

__author__ = 'Collin'
from enum import Enum

NC = "\033[0m"

class Color(Enum):
    BLACK = 30
    RED = 31
    GREEN = 32
    YELLOW = 33
    BLUE = 34
    MAGENTA = 35
    CYAN = 36
    WHITE = 37


def printColor(msg, frgndColor, bright = False):
    if not isinstance(frgndColor, Color):
        raise TypeError("Must be a member of Color")
    if not isinstance(bright, bool):
        raise TypeError("Brightness must be a bool")

    out = "\033["
    if bright:
        out += "1"
    else:
        out += "0"
    out += ";"
    out += str(frgndColor.value)
    out += "m"
    out += str(msg)
    out += NC
    print(out)


def printColorBckgnd(msg, frgndColor, bckgrndColor, frgndBright = False, bckgrnBright = False):
    if not isinstance(frgndColor, Color):
        raise TypeError("Must be a member of Color")
    if not isinstance(bckgrndColor, Color):
        raise TypeError("Must be a member of Color")
    if not isinstance(frgndBright, bool):
        raise TypeError("Brightness must be a bool")
    if not isinstance(bckgrnBright, bool):
        raise TypeError("Brightness must be a bool")

    out = "\033["
    if frgndBright:
        out += "1"
    else:
        out += "0"
    out += ";"
    out += str(frgndColor.value)
    out += ";"
    if bckgrnBright:
        out += "1"
    else:
        out += "0"
    out += ";"
    out += str(bckgrndColor.value + 10)
    out += "m"
    out += str(msg)
    out += NC
    print(out)


def test():
    print("No background, regular foreground")
    for color in Color:
        printColor("I am colorful", color)

    print()
    print("No background, bright foreground")
    for color in Color:
        printColor("I am colorful", color, True)

    print()
    print("All background colors")
    for f in Color:
        for b in Color:
                    printColorBckgnd("I am very Colorful", f, b)

    print()
    print("All background colors with bright foreground")
    for f in Color:
        for b in Color:
                    printColorBckgnd("I am very Colorful", f, b, True)

    print()
    print("All bright background colors with bright foreground")
    for f in Color:
        for b in Color:
                    printColorBckgnd("I am very Colorful", f, b, True, True)

    print()
    print("All bright background colors with regular foreground")
    for f in Color:
        for b in Color:
                    printColorBckgnd("I am very Colorful", f, b, False, True)