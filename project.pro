##########
# main libquill project file
##########

TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS = src \
          utils \
          tests

include(doc/doc.pri)
