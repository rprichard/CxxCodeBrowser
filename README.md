SourceWeb
=========

SourceWeb is a source code indexer and code navigation tool for C/C++ code.

Installation
------------

SourceWeb currently runs on Linux and OS X.


### Dependencies

SourceWeb is written in C++11.  The indexer links against Clang's C++ API.
Clang's C++ APIs are not compatible between releases, so this version of
SourceWeb requires exactly Clang 4.0.  The GUI uses Qt 4.6 or later.  Follow
the build instructions to satisfy these dependencies.


### Building on Linux

Install prerequisite packages:

Debian-based:

    sudo apt-get install make g++ libqt4-dev zlib1g-dev libncurses5-dev \
                         libclang-3.8-dev llvm-3.8-dev

Fedora/CentOS:

    sudo yum install make gcc-c++ qt-devel zlib-devel ncurses-devel

If your distribution doesn't have a Clang package (of the right version), you
can try looking for a prebuilt binary package on llvm.org.  If there isn't one,
you will have to compile from source.  You will also need to pass
`--with-clang-dir` to SourceWeb's `configure` script.

Build the software:

    mkdir build
    cd build
    ../configure [--with-clang-dir <path-to-clang-dir>]
    make -j4
    sudo make install


### Building on OS X

SourceWeb is tested with OS X 10.10, but is likely to work with some older
versions.  It will not work with OS X 10.6, because OS X 10.6 lacks libc++,
necessary for C++11.  Satisfy the prerequisites:

1. Install Xcode.

2. Install the Command Line Tools.  Run `xcode-select --install` at the
   command-line.

3. Install Qt.  Qt 5 from qt-project.org should work, but Qt 4 or Qt from other
   sources (e.g. HomeBrew/MacPorts) might also work.

4. Install the Clang library set somewhere on your system.  The easiest way to
   do this is to download the
   [official prebuilt binaries](http://llvm.org/releases/download.html).

Configure and build SourceWeb:

    mkdir build
    cd build
    ../configure --with-clang-dir <path-to-clang-dir> \
                 [--with-qmake <path-to-qt5>/<qt5ver>/clang_64/bin/qmake]
    make -j4
    make install


### Configuration notes

The Clang directory (e.g. `$HOME/clang+llvm-3.X.Y-x86_64-linux-gnu`) is
embedded into the SourceWeb build output, so it must not be moved later.

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
by the CMake project.  Clang has a [page][1] describing the format.

[1]: http://clang.llvm.org/docs/JSONCompilationDatabase.html

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
command that builds the software (e.g. `make`).  `sw-btrace` will load
`libsw-btrace.so` into each subprocess, and when this library is loaded, it
will log that subprocess' command-line (and other details) to a `btrace.log`
file in the working directory of the root `sw-btrace` process.  Once all
commands are logged, run `sw-btrace-to-compiledb` to create a
`compile_commands.json` file from the `btrace.log` file.  This script may need
some customization (e.g. to recognize unusual compiler executable names).

btrace is compatible with `ccache`, but it has not been tested with `distcc`.

btrace works on Linux, OS X, and FreeBSD.  On FreeBSD, however, the default
`cc` and `gcc` executables might be statically linked, limiting btrace's
usefulness.


### Indexing step

Run `sw-clang-indexer --index-project` to index the source code.  It will look
for a `compile_commands.json` file in the working directory and write an `index`
file to the working directory.  This step takes approximately as long as
compiling the code.


### Starting the GUI

Run `sourceweb` passing it the path to an `index` file.  On OS X, run the
`Contents/MacOS/sourceweb` binary; opening the `sourceweb.app` bundle does not
work.


Demo
----

The `demo/demo.sh` script demonstrates use of the `sw-btrace`,
`sw-clang-indexer`, and `sourceweb` commands on [bigint][2], a small C++
library.

[2]: https://mattmccutchen.net/bigint/


License
-------

The project is licensed under the BSD.  The Git repository embeds third-party
components under various permissive licenses.
