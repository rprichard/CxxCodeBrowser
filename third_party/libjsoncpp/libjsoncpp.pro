# jsoncpp 0.6.0-rc2 amalgamation

CONFIG += static

TARGET = jsoncpp
TEMPLATE = lib

DEFINES += JSON_IS_AMALGAMATION

SOURCES += \
    jsoncpp.cpp

HEADERS += \
    json/json-forwards.h \
    json/json.h

QMAKE_CXXFLAGS_WARN_ON += \
    -Wno-type-limits \
    -Wno-unused-parameter \
    -Wno-extra
