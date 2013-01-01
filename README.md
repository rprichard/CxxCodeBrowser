SourceWeb
=========

SourceWeb is a source code indexer and code navigation tool for C/C++ code.

Installation
------------

### Prerequisites

SourceWeb currently runs only on Linux.  Satisfying the prerequisites is easier
on Ubuntu 12.04 or a similarly recent Linux distribution.  The project has these
prerequisites:

 * *Linux.*  The project has been tested with both x86 and x86-64 Linux.

 * *Qt 4.8.*  The project has not been tested with Qt5.

 * *G++ 4.6 or later (build compiler).*  The project is written in C++11, so a
   recent compiler and libstdc++ are needed.

 * *Clang 3.2 (index compiler).*  The project needs a complete Clang
   installation.  It must be version 3.2 exactly, because Clang's C++ ABIs are
   not stable between releases.

On sufficiently recent distributions, the Qt and build compiler dependencies can
be satisfied using the package repository.  On Ubuntu 12.04 and later, run:

    sudo apt-get install g++ libqt4-dev

The index compiler requirement can be satisfied using the [official Clang 3.2
binaries][1], which are available for Gentoo and Ubuntu 12.04.  Alternatively,
[Clang can be built from source](#building-clang-from-source).

Clang can be installed anywhere, but it must not be moved after building the
SourceWeb project -- the indexer path is embedded in the `sw-clang-indexer`
binary.

[1]: http://llvm.org/releases/download.html#3.2


### Building Clang from source<a id="building-clang-from-source"></a>

If the official Clang binaries are not suitable, Clang can be built from source.
Run these commands (feel free to alter the build and install directories):

    BUILD_ROOT=$PWD/clang-3.2
    mkdir -p $BUILD_ROOT && cd $BUILD_ROOT
    wget http://llvm.org/releases/3.2/llvm-3.2.src.tar.gz
    wget http://llvm.org/releases/3.2/clang-3.2.src.tar.gz
    wget http://llvm.org/releases/3.2/compiler-rt-3.2.src.tar.gz
    tar xfz llvm-3.2.src.tar.gz
    mv llvm-3.2.src llvm-src
    cd $BUILD_ROOT/llvm-src/tools
    tar xfz ../../clang-3.2.src.tar.gz
    mv clang-3.2.src clang
    cd $BUILD_ROOT/llvm-src/projects
    tar xfz ../../compiler-rt-3.2.src.tar.gz
    mv compiler-rt-3.2.src compiler-rt
    mkdir $BUILD_ROOT/llvm-build
    cd $BUILD_ROOT/llvm-build
    ../llvm-src/configure --disable-assertions --enable-optimized \
        --enable-shared --prefix=$BUILD_ROOT/llvm-install
    make -j$(nproc) && make install
    echo "Pass --with-clang-dir=$BUILD_ROOT/llvm-install to configure."

The build takes about six minutes on the author's system (quad-core Core i7 w/SSD).


### Building SourceWeb

To build SourceWeb, use the conventional `configure` / `make` / `make install`
process.  The `configure` script must be invoked with a `--with-clang-dir`
option pointing to a 3.2 Clang installation.


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

The project is licensed under the MIT.  The Git repository embeds third-party
components under various permissive licenses.
