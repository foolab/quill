##########
# main libquill project file
##########

TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS = src \
          tests

# --- install
pkgconfig.files = libquill.pc
pkgconfig.path = $$(DESTDIR)/usr/lib/pkgconfig
INSTALLS += pkgconfig

include(doc/doc.pri)
