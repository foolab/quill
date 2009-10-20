
TEMPLATE = app
TARGET = ut_partialloader
DEPENDPATH += .
INCLUDEPATH += . ../ut_unittests  ../../src
QMAKE_LIBDIR += ../../src ../ut_unittests

LIBS += -lquill -lquillimagefilter -lunittests
QT += testlib
DEFINES += QT_NO_DEBUG_OUTPUT QT_NO_WARNING_OUTPUT
CONFIG += debug

# Input
HEADERS += ut_partialloader.h
SOURCES += ut_partialloader.cpp

# --- install
target.path = $$(DESTDIR)/usr/lib/libquill-tests/
INSTALLS += target

# --- clean
QMAKE_CLEAN += \
	*.gcov *.gcno *.log

QMAKE_DISTCLEAN += *.gcda *.gcno *.gcov *.log *.xml coverage *.o moc_* Makefile*


