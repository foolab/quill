TEMPLATE = app
TARGET = quill-autoclean
DEPENDPATH += .
INCLUDEPATH += .
LIBS += -lquill
QMAKE_LIBDIR += ../src/

include(../common.pri)

# Input
HEADERS += autoclean.h
SOURCES += autoclean.cpp main.cpp

target.path = /usr/lib/quill-utils
INSTALLS += target
