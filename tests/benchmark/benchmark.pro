TEMPLATE = app
TARGET =
DEPENDPATH += .
INCLUDEPATH += . ../../src
LIBS += -L../../src -lquill

CONFIG += quill
CONFIG += quillimagefilter

include(../../common.pri)

# Input
SOURCES += benchmark.cpp batchrotate.cpp generatethumbs.cpp loadthumbs.cpp tiling.cpp autofix.cpp straighten.cpp redeye.cpp
HEADERS += batchrotate.h generatethumbs.h generatethumbs.h tiling.h autofix.h straighten.h redeye.h
