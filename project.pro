##########
# main libquill project file
##########

TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS = src \
          utils \
          tests \

contains( doc, no ) {
    message( "Not building the documentation ..." )
}
else {
    include(doc/doc.pri)
    }
