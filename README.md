SourceWeb
=========

SourceWeb is a source code indexer and code navigation tool for C/C++ code.

Installation
------------

SourceWeb currently runs only on Linux.


### Dependencies

SourceWeb is written in C++11.  The indexer links against Clang 3.2's C++ API.
Clang's C++ APIs are not compatible between releases, so this version of
SourceWeb requires exactly Clang 3.2.  The GUI uses Qt 4.6 or later.  Follow
the build instructions to satisfy these dependencies.


### Building on Linux

Install prerequisite packages:

| Distribution         | Packages
| -------------------- | -------------------------
| Debian 6.0           | make g++ libqt4-dev
| Ubuntu 10.04 and up  | make g++ libqt4-dev
| Fedora               | make gcc-c++ qt-devel
| CentOS 6.0           | make gcc-c++ qt-devel
| OpenSUSE 11.4 and up | make gcc-c++ libqt4-devel

Download the [clang-redist-linux packages][1] containing Clang 3.2 and gcc-libs
4.6 and extract them into a single directory of your choosing using the
`--strip-components=1` tar option.  The gcc-libs package contains
`libstdc++.so.6`, so you probably do not want to install it into any directory
in the default library search path (e.g. `/usr/local`).  It can be omitted if
you already have libstdc++ from gcc 4.6 or newer.

[1]: http://rprichard.github.com/clang-redist-linux

    ARCH=x86     (or ARCH=x86_64)
    SRC=https://s3.amazonaws.com/rprichard-released-software/clang-redist-linux/release-20121230
    mkdir $HOME/sourceweb-clang-3.2
    cd $HOME/sourceweb-clang-3.2
    wget $SRC/clang-3.2-$ARCH-linux.tar.bz2
    wget $SRC/gcc-libs-4.6.3-$ARCH-linux.tar.bz2
    tar --strip-components=1 -xf clang-3.2-$ARCH-linux.tar.bz2
    tar --strip-components=1 -xf gcc-libs-4.6.3-$ARCH-linux.tar.bz2

Build the software:

    $ ./configure --with-clang-dir $HOME/sourceweb-clang-3.2
    $ make -j8
    $ make install

The Clang directory (`$HOME/sourceweb-clang-3.2`) is embedded into the
SourceWeb build output, so it must not be moved later.

The `configure` script is a wrapper around qmake, which is SourceWeb's build
tool.  The `configure` script supports out-of-tree builds (like qmake) and
allows configuring the install path at configure-time via --prefix (unlike
qmake).


Usage
-----

Opening a C/C++ project with SourceWeb is a three-step process:

1. Create a JSON compilation database (i.e. `compile_commands.json`).

2. Run the `sw-clang-indexer` on the database to generate an `index` file.

3. Open the `index` file in the `sourceweb` GUI program.


### JSON compilation database

The JSON compilation database is a file specifying the command-line for every
compilation of a translation unit into an object file.  It was first introduced
by the CMake project.  Clang has a [page][2] describing the format.

[2]: http://clang.llvm.org/docs/JSONCompilationDatabase.html

Example:

    [
      {
        "directory": "/home/user/foo-build",         # current working directory
        "command": "g++ -c -o file.o ../foo/file.c -DMACRO -I/home/user/bar",
        "file": "/home/user/foo/file.c"
      },
      ...
    ]

To index a project using CMake, invoke cmake with the
`-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` command-line option, which will direct
CMake to output a `compile_commands.json` file.


### btrace

For projects that do not use CMake, the btrace tool included in this project
can be used to create a `compile_commands.json` file.  btrace is a tool that
captures a trace of `exec` calls using an `LD_PRELOAD` library, then processes
the execution trace into a `compile_commands.json` file.

To use the tool, first collect a trace by prepending `btrace.sh` to the
command(s) that build the system (e.g. `make`).  `btrace.sh` will load
`libbtrace.so` into each subprocess, and all `exec` calls will be logged to a
`btrace.log` file in the working directory.  Next run `processlog.py` to
convert the `btrace.log` in the working directory file into a
`compile_commands.json` file.

btrace is compatible with `ccache`, but it has not been tested with `distcc`.


### Indexing step

Run `sw-clang-indexer --index-project` to index the source code.  It will look
for a `compile_commands.json` file in the working directory and write an `index`
file to the working directory.  This step takes approximately as long as
compiling the code.


### Starting the GUI

Run `sourceweb` in a directory containing an `index` file to navigate the index.


License
-------

The project is licensed under the BSD.  The Git repository embeds third-party
components under various permissive licenses.
