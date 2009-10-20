
TEMPLATE = app
TARGET = ut_core 
DEPENDPATH += .
INCLUDEPATH += . ../ut_unittests  ../../src
QMAKE_LIBDIR += ../../src ../ut_unittests

LIBS += -lquill -lquillimagefilter -lunittests
QT += testlib
CONFIG += debug

# Input
HEADERS += ut_core.h
SOURCES += ut_core.cpp

# --- install
target.path = $$(DESTDIR)/usr/lib/libquill-tests/
INSTALLS += target

# --- clean
QMAKE_CLEAN += \
	*.gcov *.gcno *.log

QMAKE_DISTCLEAN += *.gcda *.gcno *.gcov *.log *.xml coverage *.o moc_* Makefile*
