include(../tests.pri)

TARGET = ../bin/ut_autoclean
INCLUDEPATH += ../../utils/
QMAKE_LIBDIR += ../../src/

# Input
HEADERS += ../../utils/autoclean.h ut_autoclean.h
SOURCES += ../../utils/autoclean.cpp ut_autoclean.cpp
