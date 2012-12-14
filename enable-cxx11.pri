macx {
    QMAKE_CXXFLAGS += -std=c++11 -stdlib=libc++
    QMAKE_LFLAGS += -std=c++11 -stdlib=libc++
    QMAKE_MACOSX_DEPLOYMENT_TARGET=10.7
} else {
    QMAKE_CXXFLAGS += -std=c++0x
    QMAKE_LFLAGS += -std=c++0x
}
