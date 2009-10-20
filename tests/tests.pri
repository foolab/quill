TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += . ../ut_unittests ../../src
QMAKE_LIBDIR += ../../src ../bin

CONFIG += link_pkgconfig
PKGCONFIG += quillimagefilter

LIBS += -lquill -lunittests
QT += testlib

CONFIG += debug

# --- install
target.path = $$[QT_INSTALL_LIBS]/libquill-tests/
INSTALLS += target

# --- clean
QMAKE_CLEAN += \
	*.gcov *.gcno *.log

QMAKE_DISTCLEAN += *.gcda *.gcno *.gcov *.log *.xml coverage *.o moc_* Makefile*
