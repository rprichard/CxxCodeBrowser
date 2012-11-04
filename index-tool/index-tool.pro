TARGET = index-tool
TEMPLATE = app

SOURCES += \
    main.cc

PRE_TARGETDEPS += $${OUT_PWD}/../libindexdb/libindexdb.a
LIBS +=           $${OUT_PWD}/../libindexdb/libindexdb.a

INCLUDEPATH    += ../third_party/libsnappy
PRE_TARGETDEPS += $${OUT_PWD}/../third_party/libsnappy/libsnappy.a
LIBS           += $${OUT_PWD}/../third_party/libsnappy/libsnappy.a

INCLUDEPATH    += ../third_party/libMurmurHash3
PRE_TARGETDEPS += $${OUT_PWD}/../third_party/libMurmurHash3/libMurmurHash3.a
LIBS           += $${OUT_PWD}/../third_party/libMurmurHash3/libMurmurHash3.a

INCLUDEPATH    += ../third_party/libsha2
PRE_TARGETDEPS += $${OUT_PWD}/../third_party/libsha2/libsha2.a
LIBS           += $${OUT_PWD}/../third_party/libsha2/libsha2.a

QMAKE_CXXFLAGS += -std=c++0x
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
