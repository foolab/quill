TEMPLATE = app
TARGET = quill-autoclean
DEPENDPATH += .
INCLUDEPATH += .
LIBS += -lquill
QMAKE_LIBDIR += ../src/

# Avoid automatic casts from QString to QUrl. Dangerous!!!
DEFINES += QT_NO_URL_CAST_FROM_STRING

include(../common.pri)

# Input
HEADERS += autoclean.h
SOURCES += autoclean.cpp main.cpp

target.path = /usr/lib/quill-utils
INSTALLS += target
