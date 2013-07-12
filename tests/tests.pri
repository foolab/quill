# This is not automatically created pro file. Please do not modify it without consideration.

TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += . ../ut_unittests ../../src ../../src/dbus-thumbnailer
QMAKE_LIBDIR += ../../src ../bin ../ut_unittests

QMAKEFEATURES += ../../src
CONFIG += link_pkgconfig
equals(QT_MAJOR_VERSION, 4): PKGCONFIG += quillmetadata quillimagefilter
equals(QT_MAJOR_VERSION, 5): PKGCONFIG += quillmetadata-qt5 quillimagefilter-qt5

equals(QT_MAJOR_VERSION, 5): QT += widgets

include(../common.pri)

# Generate pkg-config support by default
# Note that we HAVE TO also create prl config as QMake implementation
# mixes both of them together.
CONFIG += create_pc create_prl no_install_prl

QMAKE_PKGCONFIG_REQUIRES = quill quillimagefilter QtGui
QMAKE_PKGCONFIG_LIBDIR = $$[QT_INSTALL_LIBS]

equals(QT_MAJOR_VERSION, 4): LIBS += -lquill
equals(QT_MAJOR_VERSION, 5): LIBS += -lquill-qt5

LIBS += -lunittests-quill
QT += testlib

# --- install
target.path = $$[QT_INSTALL_LIBS]/libquill-tests/
INSTALLS += target

# --- clean
QMAKE_CLEAN += \
	*.gcov *.gcno *.log

QMAKE_DISTCLEAN += *.gcda *.gcno *.gcov *.log *.xml coverage *.o moc_* Makefile*
