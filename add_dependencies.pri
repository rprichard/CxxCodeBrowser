# This file uses undocumented qmake functionality.  Some is documented at
# http://www.qtcentre.org/wiki/index.php?title=Undocumented_qmake.  Some other
# things, like for(ever), are not documented at all.  See the qmake source.

DEPS_NAME = dependencies.cfg

defineTest(addDependency) {
    equals(2, direct) {
        # For direct dependencies only, add the dependency directory to the
        # include path.  DEPENDPATH is a list of paths that qmake searches to
        # find included header files.
        INCLUDEPATH -= $${ROOT_DIR}/$${1}
        INCLUDEPATH += $${ROOT_DIR}/$${1}
        DEPENDPATH -= $${ROOT_DIR}/$${1}
        DEPENDPATH += $${ROOT_DIR}/$${1}
        export (INCLUDEPATH)
        export (DEPENDPATH)
    }

    # Add the dependency to the LIBS variable.
    win32:CONFIG(release, debug|release) {
        lib = $${ROOT_DIR}/$${1}/release/$$basename(1).a
    } else:win32:CONFIG(debug, debug|release) {
        lib = $${ROOT_DIR}/$${1}/debug/$$basename(1).a
    } else:unix {
        lib = $${ROOT_DIR}/$${1}/$$basename(1).a
    }
    # Ensure that the new LIB exists exactly once in the list, at the end.
    # If a program links against libraries A and B that both need library
    # C, then libC.a must be on the linker command-line after libA.a and
    # after libB.a.  Thanks to the qmake Unix generator's
    # "smart-library-merging" feature, we must also ensure that libC.a
    # never appears before either libA.a or libB.a in LIBS, because qmake
    # may delete the later instance thinking it is redundant.  (Note that
    # the MinGW generator is *not* a subclass of the Unix generator in
    # qmake.)
    LIBS -= $$lib
    LIBS += $$lib
    export (LIBS)

    # Setting PRE_TARGETDEPS is needed so that a change to a dependency
    # static library causes the dependents to relink.
    PRE_TARGETDEPS -= $$lib
    PRE_TARGETDEPS += $$lib
    export (PRE_TARGETDEPS)
}

# defineTest(depCfgPath).  depCfgPath is an absolute path to a dependencies.cfg
# file.
defineTest(addDependencies) {
    directDeps = $$cat($$1)
    depFiles =
    for(directDep, directDeps) {
        # Each directDep is a relative path from the root to the directory
        # containing the library.
        addDependency ($$directDep, direct)
        directDepFile = \
            $${_PRO_FILE_PWD_}/$${ROOT_DIR}/$${directDep}/$${DEPS_NAME}
        depFiles -= $$directDepFile
        depFiles += $$directDepFile
    }
    for(ever) {
        isEmpty(depFiles): break ()
        depFile = $$first(depFiles)
        depFiles -= $$depFile
        !exists($$depFile): next ()
        indirectDeps = $$cat($$depFile)
        for(indirectDep, indirectDeps) {
            addDependency ($$indirectDep, indirect)
            indirectDepFile = \
                $${_PRO_FILE_PWD_}/$${ROOT_DIR}/$${indirectDep}/$${DEPS_NAME}
            depFiles -= $$indirectDepFile
            depFiles += $$indirectDepFile
        }
    }
}

exists($${_PRO_FILE_PWD_}/$${DEPS_NAME}) {
    addDependencies($${_PRO_FILE_PWD_}/$${DEPS_NAME})
}
