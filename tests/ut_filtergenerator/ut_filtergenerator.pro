#include (../tests.pri)

TEMPLATE = app
TARGET = ut_filtergenerator
DEPENDPATH += .
INCLUDEPATH += . ../ut_unittests  ../../src
QMAKE_LIBDIR += ../../src ../ut_unittests

LIBS += -lquill -lqtimagefilter -lquillimagefilter -lunittests
QT += testlib
CONFIG += debug

# Input
HEADERS += ut_filtergenerator.h
SOURCES += ut_filtergenerator.cpp

# --- install
target.path = $$(DESTDIR)/usr/lib/libquill-tests/
INSTALLS += target

# --- clean
QMAKE_CLEAN += \
	*.gcov *.gcno *.log

QMAKE_DISTCLEAN += *.gcda *.gcno *.gcov *.log *.xml coverage *.o moc_* Makefile*
