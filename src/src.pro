##########
# the project file in src/
##########

TEMPLATE = lib
TARGET = quill

DEPENDPATH += .

# INCLUDEPATH += . /usr/include/qt4/quillimagefilter
INCLUDEPATH += . $$[QT_INSTALL_HEADERS]/quillimagefilter

LIBS += -lgcov
QMAKE_CXXFLAGS += -ftest-coverage -fprofile-arcs -fno-elide-constructors
DEFINES     +=

CONFIG += DEBUG

CONFIG += quillimagefilter

# Generate pkg-config support by default
# Note that we HAVE TO also create prl config as QMake implementation
# mixes both of them together.
CONFIG += create_pc create_prl no_install_prl

QMAKE_PKGCONFIG_REQUIRES = quillimagefilter QtGui
QMAKE_PKGCONFIG_INCDIR = $$[QT_INSTALL_HEADERS]/$$TARGET
QMAKE_PKGCONFIG_LIBDIR = $$[QT_INSTALL_LIBS]

#this is for removing coverage information while doing qmake as "qmake COV_OPTION=off"
for(OPTION,$$list($$lower($$COV_OPTION))){
    isEqual(OPTION, off){
        QMAKE_CXXFLAGS -= -ftest-coverage -fprofile-arcs -fno-elide-constructors
        LIBS -= -lgcov
    }
}

# --- input

HEADERS += quill.h \
           quillfile.h \
           core.h \
           tilecache.h \
           tilemap.h \
           savemap.h \
           threadmanager.h \
           quillundocommand.h \
           quillundostack.h \
           imagecache.h \
           historyxml.h

SOURCES += quill.cpp \
           quillfile.cpp \
           core.cpp \
           tilecache.cpp \
           tilemap.cpp \
           savemap.cpp \
           threadmanager.cpp \
           quillundocommand.cpp \
           quillundostack.cpp \
           imagecache.cpp \
           historyxml.cpp

INSTALL_HEADERS = \
           Quill \
           QuillFile \
           quill.h \
           quillfile.h

# --- install
headers.files = $$INSTALL_HEADERS
headers.path = $$[QT_INSTALL_HEADERS]/$$TARGET
target.path = $$[QT_INSTALL_LIBS]
pkgconfig.files = quill.pc
pkgconfig.path = $$[QT_INSTALL_LIBS]/pkgconfig
prf.files = quill.prf
prf.path = $$[QT_INSTALL_DATA]/mkspecs/features
INSTALLS += target headers pkgconfig prf

# ---clean
QMAKE_CLEAN += \
	*.gcov *.gcno *.log *.moc_* *.gcda
