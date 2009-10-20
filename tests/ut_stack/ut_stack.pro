#include (../tests.pri)

TEMPLATE = app
TARGET = ut_stack
DEPENDPATH += .
INCLUDEPATH += . ../ut_unittests ../../src
QMAKE_LIBDIR += ../../src ../ut_unittests

LIBS += -lquill -lquillimagefilter -lunittests
QT += testlib
CONFIG += debug

# Input
HEADERS += ut_stack.h
SOURCES += ut_stack.cpp

# --- install
target.path = $$(DESTDIR)/usr/lib/libquill-tests/
INSTALLS += target

# --- clean
QMAKE_CLEAN += \
	*.gcov *.gcno *.log

QMAKE_DISTCLEAN += *.gcda *.gcno *.gcov *.log *.xml coverage *.o moc_* Makefile*
