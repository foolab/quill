TEMPLATE = app
DEPENDPATH += .
# Please do not delete the INCLUDEDPATH
INCLUDEPATH += . ../ut_unittests ../../src  $$[QT_INSTALL_HEADERS]/quillimagefilter
QMAKE_LIBDIR += ../../src ../bin ../ut_unittests

CONFIG += quillimagefilter
CONFIG += debug

LIBS += -lquill -lunittests
QT += testlib

# --- install
target.path = $$[QT_INSTALL_LIBS]/libquill-tests/
INSTALLS += target

# --- clean
QMAKE_CLEAN += \
	*.gcov *.gcno *.log

QMAKE_DISTCLEAN += *.gcda *.gcno *.gcov *.log *.xml coverage *.o moc_* Makefile*
