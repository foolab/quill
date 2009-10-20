TEMPLATE = lib
TARGET = unittests
DEPENDPATH += .
INCLUDEPATH += .

QT += testlib
DEFINES     += 
CONFIG += debug

# Input
HEADERS += unittests.h
SOURCES += unittests.cpp

# --- install
target.path = $$[QT_INSTALL_LIBS]
INSTALLS += target

# --- clean
QMAKE_CLEAN += \
	*.gcov *.gcno *.log

QMAKE_DISTCLEAN += *.gcda *.gcno *.gcov *.log *.xml coverage *.o moc_* Makefile*
