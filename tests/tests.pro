
TEMPLATE = subdirs

DEPENDPATH += .
INCLUDEPATH += .

CONFIG += ordered

SUBDIRS += \
           ut_unittests \
           ut_filtergenerator \
           ut_crashrecovery \
           ut_imageloader \
           ut_partialloader \
           ut_tiling \
           ut_quill \
           ut_tilemap \
           ut_savemap \
           ut_imagecache \
           ut_command \
           ut_stack \
           ut_xml \
           ut_core \
           ut_file \
           ut_thumbnail \
           benchmark  \

# --- install
tatam.files = tests.xml
tatam.path  = $$(DESTDIR)/usr/share/libquill-tests/

tatamimages.files += images/16_color_palette.png
tatamimages.files += images/benchmark12.jpg

tatamimages.path  = $$(DESTDIR)/usr/share/libquill-tests/images/

INSTALLS += tatam tatamimages
