SourceWeb
=========

SourceWeb is a source code indexer and code navigation tool for C/C++ code.

Installation
------------

SourceWeb currently runs on Linux and OS X.


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

Download the [clang-redist packages][1] containing Clang 3.2 and gcc-libs 4.6
and extract them into a single directory of your choosing using the
`--strip-components=1` tar option.  The gcc-libs package contains
`libstdc++.so.6`, so you probably do not want to install it into any directory
in the default library search path (e.g. `/usr/local`).  It can be omitted if
you already have libstdc++ from gcc 4.6 or newer.

[1]: http://rprichard.github.com/clang-redist

    ARCH=x86     (or ARCH=x86_64)
    SRC=https://s3.amazonaws.com/rprichard-released-software/clang-redist/release-1
    mkdir $HOME/sourceweb-clang-3.2-1
    cd $HOME/sourceweb-clang-3.2-1
    wget $SRC/clang-3.2-1-$ARCH-linux.tar.bz2
    wget $SRC/gcc-libs-4.6.3-1-$ARCH-linux.tar.bz2
    tar --strip-components=1 -xf clang-3.2-1-$ARCH-linux.tar.bz2
    tar --strip-components=1 -xf gcc-libs-4.6.3-1-$ARCH-linux.tar.bz2

Build the software:

    ./configure --with-clang-dir $HOME/sourceweb-clang-3.2-1
    make -j4
    make install


### Building on OS X

SourceWeb is tested with OS X 10.8, but probably works with OS X 10.7.  It will not
work with OS X 10.6, because OS X 10.6 lacks libc++.  Satisfy the prerequisites:

1. Install Xcode.

2. Install the Command Line Tools.  (In Xcode's Preferences window, go to the
   Downloads tab.  Command Line Tools should be listed as an installable
   component.)

3. Install Qt 5 from qt-project.org.  Make note of where Qt was installed.

4. Download and extract the [clang-redist package][2] containing Clang 3.2.  The
   official llvm.org package for Clang 3.2 is built for libstdc++ and is not
   suitable.

        cd $HOME
        SRC=https://s3.amazonaws.com/rprichard-released-software/clang-redist/release-1
        curl -O $SRC/clang-3.2-1-x86_64-darwin.tar.bz2
        tar -xf clang-3.2-1-x86_64-darwin.tar.bz2

Configure and build SourceWeb:

    ./configure --with-clang-dir $HOME/clang-3.2-1-x86_64-darwin \
                --with-qmake <path-to-qt5>/<qt5ver>/clang_64/bin/qmake
    make -j4
    make install

[2]: http://rprichard.github.com/clang-redist


### Configuration notes

The Clang directory (e.g. `$HOME/sourceweb-clang-3.2-1`) is embedded into the
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
by the CMake project.  Clang has a [page][3] describing the format.

[3]: http://clang.llvm.org/docs/JSONCompilationDatabase.html

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
captures a trace of all executed commands using an `LD_PRELOAD` library, then
converts the execution trace into a `compile_commands.json` file.

To use the tool, first collect a trace by prepending `sw-btrace` to the
command(s) that build the system (e.g. `make`).  `sw-btrace` will load
`libsw-btrace.so` into each subprocess, and when this library is loaded, it
will log that subprocess' command-line (and other details) to a `btrace.log`
file in the working directory.  Once all commands are logged, run
`sw-btrace-to-compiledb` to create a `compile_commands.json` file from the
`btrace.log` file.

btrace is compatible with `ccache`, but it has not been tested with `distcc`.


### Indexing step

Run `sw-clang-indexer --index-project` to index the source code.  It will look
for a `compile_commands.json` file in the working directory and write an `index`
file to the working directory.  This step takes approximately as long as
compiling the code.


### Starting the GUI

Run `sourceweb` in a directory containing an `index` file to navigate the index.


Demo
----

The `demo/demo.sh` script demonstrates use of the `sw-btrace`,
`sw-clang-indexer`, and `sourceweb` commands on [bigint][4], a small C++
library.

[4]: https://mattmccutchen.net/bigint/


License
-------

The project is licensed under the BSD.  The Git repository embeds third-party
components under various permissive licenses.
