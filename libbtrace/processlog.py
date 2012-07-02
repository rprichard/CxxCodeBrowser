#!/usr/bin/env python
import os
import subprocess
import re

DRIVERS = [
    "cc", "c++",
    "gcc", "g++",
    "clang", "clang++",
]


class Command:
    def __init__(self, cwd, program, argv):
        self.cwd = cwd
        self.program = program
        self.argv = argv


class SourceFile:
    def __init__(self, path, args):
        self.path = path
        self.args = args


def readString(fp):
    result = ""
    inQuote = False
    while True:
        ch = fp.read(1)
        if ch == '':
            return (result, ch)
        elif ch == '\\':
            result += fp.read(1)
        elif ch == ' ' and not inQuote:
            return (result, ch)
        elif ch == '\n' and not inQuote:
            return (result, ch)
        elif ch == '"':
            inQuote = not inQuote
        else:
            result += ch


def readFile(path):
    commands = []
    fp = open(path)

    # Each entry must be terminated with a newline.  A file with no entries
    # has a size of 0 bytes.
    while True:
        (cwd, delim) = readString(fp)
        if cwd == "" and delim == "":
            break
        assert delim == '\n'
        (program, delim) = readString(fp)
        assert delim == '\n'
        argv = []
        while True:
            (arg, delim) = readString(fp)
            argv.append(arg)
            assert delim in [' ', '\n']
            if delim == '\n':
                break
        commands.append(Command(cwd, program, argv))
    return commands


filterPattern = re.compile(
    r"-W | ( -[gwc] | -pedantic | -O[0-9] )$",
    re.VERBOSE)


sourceExtensions = [".c", ".cc", ".cpp", ".cxx", ".c++"]


def endsWithOneOf(text, extensions):
    for extension in extensions:
        if text.endswith(extension):
            return True
    return False


def extractSourceFile(command):
    """Attempt to extract a compile step from a driver command line."""

    if os.path.basename(command.program) not in DRIVERS or \
            "-c" not in command.argv:
        return None

    args = command.argv[1:]
    inputFile = None
    outputFile = None

    outputArgs = []

    while len(args) > 0:

        if args[0][0] == "-" and args[0][1] in "oDI" and len(args[0]) == 2:
            assert len(args) >= 2
            args[0] = args[0] + args[1]
            args.pop(1)

        if filterPattern.match(args[0]):
            args.pop(0)
        elif args[0][0] == "-":
            if args[0][1] == "o":
                assert outputFile is None
                outputFile = args[0][2:]
                args.pop(0)
            elif args[0][1] == "I":
                outputArgs.append("-I" +
                                  os.path.realpath(os.path.join(command.cwd,
                                                                args[0][2:])))
                args.pop(0)
            else:
                outputArgs.append(args.pop(0))
        elif endsWithOneOf(args[0], sourceExtensions):
            assert inputFile is None
            inputFile = args.pop(0)
        else:
            print("Unknown argument: " + args.pop(0))

    if inputFile is None:
        return None

    return SourceFile(os.path.join(command.cwd, inputFile), outputArgs)


def main():
    commands = readFile("btrace.log")
    f = open("btrace.sources", "w")
    for x in commands:
        sourceFile = extractSourceFile(x)
        if sourceFile is not None:
            # TODO: escaping
            f.write(" ".join([sourceFile.path] + sourceFile.args) + "\n")
    f.close()


if __name__ == "__main__":
    main()
