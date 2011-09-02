TEMPLATE = app
TARGET =
DEPENDPATH += .
INCLUDEPATH += . ../../src
LIBS += -L../../src -lquill

# Avoid automatic casts from QString to QUrl. Dangerous!!!
DEFINES += QT_NO_URL_CAST_FROM_STRING

CONFIG += quill
CONFIG += quillimagefilter

include(../../common.pri)

# Input
SOURCES += benchmark.cpp batchrotate.cpp generatethumbs.cpp loadthumbs.cpp tiling.cpp autofix.cpp straighten.cpp redeye.cpp
HEADERS += batchrotate.h generatethumbs.h generatethumbs.h tiling.h autofix.h straighten.h redeye.h
