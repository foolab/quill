TEMPLATE = app
TARGET = quill-autoclean
DEPENDPATH += .
INCLUDEPATH += .

equals(QT_MAJOR_VERSION, 4): LIBS += -lquill
equals(QT_MAJOR_VERSION, 5): LIBS += -lquill-qt5

QMAKE_LIBDIR += ../src/

# Avoid automatic casts from QString to QUrl. Dangerous!!!
DEFINES += QT_NO_URL_CAST_FROM_STRING

include(../common.pri)

# Input
HEADERS += autoclean.h
SOURCES += autoclean.cpp main.cpp

target.path = /usr/lib/quill-utils
INSTALLS += target
