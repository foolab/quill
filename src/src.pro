##########
# the project file in src/
##########

TEMPLATE = lib
TARGET = quill
# Please do not remove this INCLUDEPATH in any case
INCLUDEPATH += . $$[QT_INSTALL_HEADERS]/quillimagefilter $$[QT_INSTALL_HEADERS]/quillmetadata
DEPENDPATH += .

# Avoid automatic casts from QString to QUrl. Dangerous!!!
DEFINES += QT_NO_URL_CAST_FROM_STRING
MOC_DIR = .moc
QT += dbus

include(../common.pri)

CONFIG += quillimagefilter quillmetadata

LIBS += -lexif -lexempi
# Generate pkg-config support by default
# Note that we HAVE TO also create prl config as QMake implementation
# mixes both of them together.
CONFIG += create_pc create_prl no_install_prl

QMAKE_PKGCONFIG_REQUIRES = quillimagefilter quillmetadata QtGui
QMAKE_PKGCONFIG_INCDIR = $$[QT_INSTALL_HEADERS]/$$TARGET
QMAKE_PKGCONFIG_LIBDIR = $$[QT_INSTALL_LIBS]

QMAKE_CXXFLAGS += -Werror
# this is for adding coverage information while doing qmake as "qmake COV_OPTION=on"
# message is shown when 'make' is executed
for(OPTION,$$list($$lower($$COV_OPTION))){
    isEqual(OPTION, on){
        message("TEST COVERAGE IS ENABLED")
        QMAKE_CXXFLAGS += -ftest-coverage -fprofile-arcs -fno-elide-constructors
        LIBS += -lgcov
    }
}

# --- input

HEADERS += quill.h \
           quillfile.h \
           quillerror.h \
           file.h \
           core.h \
           displaylevel.h \
           tilecache.h \
           tilemap.h \
           savemap.h \
           task.h \
           scheduler.h \
           threadmanager.h \
           quillundocommand.h \
           quillundostack.h \
           imagecache.h \
           historyxml.h \
           unix_platform.h \
           logger.h \
           dbus-thumbnailer/dbusthumbnailer.h \
           dbus-thumbnailer/thumbnailer_generic.h \
           backgroundthread.h \
           regionsofinterest.h

SOURCES += quill.cpp \
           quillfile.cpp \
           quillerror.cpp \
           file.cpp \
           core.cpp \
           displaylevel.cpp \
           tilecache.cpp \
           tilemap.cpp \
           savemap.cpp \
           task.cpp \
           scheduler.cpp \
           threadmanager.cpp \
           quillundocommand.cpp \
           quillundostack.cpp \
           imagecache.cpp \
           historyxml.cpp \
           unix_platform.cpp \
           dbus-thumbnailer/dbusthumbnailer.cpp \
           dbus-thumbnailer/thumbnailer_generic.cpp \
           backgroundthread.cpp \
           regionsofinterest.cpp

debug {
    SOURCES += logger.cpp
}

INSTALL_HEADERS = \
           Quill \
           QuillFile \
           QuillError \
           quill.h \
           quillfile.h \
           quillerror.h

# --- install
headers.files = $$INSTALL_HEADERS
headers.path = $$[QT_INSTALL_HEADERS]/$$TARGET
target.path = $$[QT_INSTALL_LIBS]
target.depends += generateproxy
pkgconfig.files = quill.pc
pkgconfig.path = $$[QT_INSTALL_LIBS]/pkgconfig
prf.files = quill.prf
prf.path = $$[QMAKE_MKSPECS]/features
INSTALLS += target headers pkgconfig prf

generateproxy.target = dbus-thumbnailer/thumbnailer_generic.h
generateproxy.depends = dbus-thumbnailer/tumbler-service-dbus.xml
generateproxy.commands = qdbusxml2cpp -c ThumbnailerGenericProxy -p dbus-thumbnailer/thumbnailer_generic.h:dbus-thumbnailer/thumbnailer_generic.cpp dbus-thumbnailer/tumbler-service-dbus.xml org.freedesktop.thumbnails.Thumbnailer1

# ---clean
QMAKE_CLEAN += *.gcov *.gcno *.log *.moc_* *.gcda
#               dbus-thumbnailer/thumbnailer_generic.h \
#               dbus-thumbnailer/thumbnailer_generic.cpp

QMAKE_EXTRA_TARGETS += generateproxy

QMAKE_LFLAGS += -Wl,--as-needed
