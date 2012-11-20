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
    def __init__(self, cwd, parent, program, argv):
        assert isinstance(cwd, str)
        assert parent is None or isinstance(parent, Command)
        assert isinstance(program, str)
        self.cwd = cwd
        self.parent = parent
        self.program = program
        self.argv = argv
        self.isCompilerDriver = (os.path.basename(self.program) in DRIVERS)


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
    commandIDToCommand = {}
    fp = open(path)

    # Each entry must be terminated with a newline.  A file with no entries
    # has a size of 0 bytes.
    while True:
        recType = fp.readline()
        if recType == "":
            break

        assert recType == "exec\n"
        parentPid = int(fp.readline())
        parentTime = int(fp.readline())
        selfPid = int(fp.readline())
        selfTime = int(fp.readline())
        (cwd, delim) = readString(fp)
        assert delim == "\n"
        (program, delim) = readString(fp)
        assert delim == '\n'
        argv = []
        while True:
            (arg, delim) = readString(fp)
            argv.append(arg)
            assert delim in [' ', '\n']
            if delim == '\n':
                break
        assert fp.readline() == "\n"

        parentID = str(parentPid) + "-" + str(parentTime)
        selfID = str(selfPid) + "-" + str(selfTime)
        parent = commandIDToCommand.get(parentID)
        command = Command(cwd, parent, program, argv)
        commands.append(command)
        commandIDToCommand[selfID] = command

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


def isChildOfCompilerDriver(command):
    parent = command.parent
    while parent is not None:
        if parent.isCompilerDriver:
            return True
        parent = parent.parent
    return False


def extractSourceFile(command):
    """Attempt to extract a compile step from a driver command line."""

    # Detect ccache.  With ccache, a parent process will be exec'ed using a
    # program name like "g++", but the g++ is really a symlink to a ccache
    # program.  ccache invokes children g++ subprocesses, but first it does
    # unsavory things to the arguments.  e.g. Instead of:
    #    g++ -c file.cc -o file.o
    # we get something like this:
    #    g++ -E file.cc   [output redirected to a tmp file]
    #    g++ -c $HOME/.ccache/tmp/tmp.i -o $HOME/.ccache/tmp.o.1234
    # The translation unit compilation is now split into two, and the output
    # filename is lost.  The approach taken here is to ignore any subprocesses
    # of a compiler driver invocation.
    #
    # (An alternative approach would be to fail on ccache.  This approach is
    # poor because:
    #  - ccache is actually really useful at speeding up builds.
    #  - ccache is turned on by default on Fedora 16.
    #  - I'm not sure the failure mode would be obvious, and confusing users is
    #    depressing.)

    if not command.isCompilerDriver or isChildOfCompilerDriver(command) or \
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
