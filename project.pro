##########
# main libquill project file
##########

TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS = src \
          tests

include(doc/doc.pri)
