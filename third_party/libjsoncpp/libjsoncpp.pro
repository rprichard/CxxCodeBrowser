# jsoncpp 0.6.0-rc2 amalgamation

QT -= core gui
CONFIG += static

TARGET = jsoncpp
TEMPLATE = lib

DEFINES += JSON_IS_AMALGAMATION

SOURCES += \
    jsoncpp.cpp

HEADERS += \
    json/json.h \
    json/json-forwards.h

include(../../enable-cxx11.pri)

QMAKE_CXXFLAGS_WARN_ON += \
    -Wno-type-limits \
    -Wno-unused-parameter \
    -Wno-extra
