TARGET = index-tool
TEMPLATE = app

SOURCES += \
    main.cc

PRE_TARGETDEPS += $${OUT_PWD}/../libindexdb/libindexdb.a
LIBS +=           $${OUT_PWD}/../libindexdb/libindexdb.a

QMAKE_CXXFLAGS += -Wall -Wno-unused-parameter -std=c++0x
