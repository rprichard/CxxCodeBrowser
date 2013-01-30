include(./config.pri)

TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    third_party \
    libindexdb \
    clang-indexer \
    index-tool \
    navigator

# HACK: Stop qmake from attempting to strip the scripts.
QMAKE_STRIP = /bin/echo

# Unix btrace.
linux-*|freebsd-*|darwin-*|macx-* {
    SUBDIRS += btrace
    btrace_script.path = $$BIN_DIR
    btrace_script.files += \
        btrace/sw-btrace \
        btrace/sw-btrace-to-compiledb
    INSTALLS += btrace_script
}

# Linux executable wrapper scripts.
linux-* {
    SUBDIRS += linux-elfvinfo
    libCfgFile = $(INSTALL_ROOT)/$$LIBEXEC_DIR/sw-libstdcxx-path
    libPath = $$CLANG_DIR/lib
    lib64Path = $$CLANG_DIR/lib64
    libstdcxx =
    libstdcxxSep =
    exists($$libPath/libstdc++.so.6) {
        libstdcxx = $$libPath$$libstdcxxSep$$libstdcxx
        libstdcxxSep = :
    }
    exists($$lib64Path/libstdc++.so.6) {
        libstdcxx = $$lib64Path$$libstdcxxSep$$libstdcxx
        libstdcxxSep = :
    }
    wrapper = $$_PRO_FILE_PWD_/linux-wrapper.sh
    linux_wrapper_scripts.path = $$PREFIX
    linux_wrapper_scripts.extra += rm -f $$libCfgFile;
    !equals(libstdcxx, "") {
        linux_wrapper_scripts.extra += echo $$libstdcxx > $$libCfgFile;
    }
    linux_wrapper_scripts.extra += \
        cp $$wrapper $(INSTALL_ROOT)/$$BIN_DIR/sourceweb; \
        cp $$wrapper $(INSTALL_ROOT)/$$BIN_DIR/sw-clang-indexer; \
        cp $$wrapper $(INSTALL_ROOT)/$$BIN_DIR/sw-index-tool;
    INSTALLS += linux_wrapper_scripts
}

include(./check-clang.pri)
