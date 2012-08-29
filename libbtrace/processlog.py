#!/usr/bin/env python
import json
import os
import re
import subprocess

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

assemblyExtensions = [".s", ".S"]


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

    outputIncludes = []
    outputDefines = []
    outputExtraArgs = []

    while len(args) > 0:

        arg = args.pop(0)

        if arg[0] == "-" and arg[1] in "oDI" and len(arg) == 2:
            assert len(args) >= 1
            arg += args.pop(0)

        if filterPattern.match(arg):
            pass
        elif arg[0] == "-":
            if arg[1] == "o":
                assert outputFile is None
                outputFile = arg[2:]
            elif arg[1] == "I":
                outputIncludes.append(
                    os.path.realpath(
                        os.path.join(command.cwd,
                                     arg[2:])))
            elif arg[1] == "D":
                outputDefines.append(arg[2:])
            elif arg == "-include":
                # Keep this and next argument.
                assert len(args) >= 1
                outputExtraArgs.append(arg)
                outputExtraArgs.append(
                    os.path.realpath(
                        os.path.join(command.cwd,
                                     args.pop(0))))
            elif arg in ["-MF", "-MT", "-MQ", "--param"]:
                # Discard this and next argument.
                assert len(args) >= 1
                args.pop(0)
            else:
                outputExtraArgs.append(arg)
        elif endsWithOneOf(arg, sourceExtensions):
            assert inputFile is None
            inputFile = arg
        elif endsWithOneOf(arg, assemblyExtensions):
            return None
        else:
            print("Unknown argument: " + arg)
            print(command.argv)
            outputExtraArgs.append(arg)

    if inputFile is None:
        return None

    output = """    {"file": """ + json.dumps(
        os.path.join(command.cwd, inputFile))

    if len(outputIncludes) > 0:
        output += """\n        ,"includes": """ + json.dumps(outputIncludes)
    if len(outputDefines) > 0:
        output += """\n        ,"defines": """ + json.dumps(outputDefines)
    if len(outputExtraArgs) > 0:
        output += """\n        ,"extraArgs": """ + json.dumps(outputExtraArgs)
    output += "}"

    return output


def main():
    firstFile = True
    commands = readFile("btrace.log")
    f = open("btrace.sources", "w")
    f.write("[\n")
    for x in commands:
        sourceFile = extractSourceFile(x)
        if sourceFile is not None:
            if not firstFile:
                f.write(",\n")
            f.write("    ")
            f.write(sourceFile)
            firstFile = False
    if not firstFile:
        f.write("\n")
    f.write("]")
    f.close()


if __name__ == "__main__":
    main()
