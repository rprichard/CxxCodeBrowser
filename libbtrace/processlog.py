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


sourceExtensions = [".c", ".cc", ".cpp", ".cxx", ".c++"]

assemblyExtensions = [".s", ".S"]


def endsWithOneOf(text, extensions):
    for extension in extensions:
        if text.endswith(extension):
            return True
    return False


def joinCommandLine(argv):
    # TODO: Actually get this right -- quotes,spaces,escapes,newlines
    # TODO: Review what Clang's libtooling and CMake do, especially on Windows.
    return " ".join(argv)


def extractSourceFile(command):
    """Attempt to extract a compile step from a driver command line."""

    if os.path.basename(command.program) not in DRIVERS or \
            "-c" not in command.argv:
        return None

    args = command.argv[1:]
    inputFile = None

    while len(args) > 0:
        arg = args.pop(0)
        if arg[0] == "-":
            pass
        elif endsWithOneOf(arg, sourceExtensions):
            assert inputFile is None
            inputFile = arg
        elif endsWithOneOf(arg, assemblyExtensions):
            return None

    if inputFile is None:
        return None

    output =  '{\n'
    output += '  "directory" : %s,\n' % json.dumps(command.cwd)
    output += '  "command" : %s,\n' % json.dumps(joinCommandLine(command.argv))
    output += '  "file" : %s\n' % json.dumps(inputFile)
    output += '}'

    return output


def main():
    firstFile = True
    commands = readFile("btrace.log")
    f = open("compile_commands.json", "w")
    f.write("[")
    for x in commands:
        sourceFile = extractSourceFile(x)
        if sourceFile is not None:
            if not firstFile:
                f.write(",")
            firstFile = False
            f.write("\n")
            f.write(sourceFile)
    f.write("\n]")
    f.close()


if __name__ == "__main__":
    main()
