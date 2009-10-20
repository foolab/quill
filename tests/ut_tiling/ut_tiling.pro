
TEMPLATE = app
TARGET = ut_tiling
DEPENDPATH += .
INCLUDEPATH += . ../ut_unittests  ../../src
QMAKE_LIBDIR += ../../src ../ut_unittests

LIBS += -lquill -lquillimagefilter -lunittests
QT += testlib
CONFIG += debug

# Input
HEADERS += ut_tiling.h
SOURCES += ut_tiling.cpp


# --- install
target.path = $$(DESTDIR)/usr/lib/libquill-tests/
INSTALLS += target

# --- clean
QMAKE_CLEAN += \
	*.gcov *.gcno *.log

QMAKE_DISTCLEAN += *.gcda *.gcno *.gcov *.log *.xml coverage *.o moc_* Makefile*


