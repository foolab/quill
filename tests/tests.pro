
TEMPLATE = subdirs

DEPENDPATH += .
INCLUDEPATH += .

# Avoid automatic casts from QString to QUrl. Dangerous!!!
DEFINES += QT_NO_URL_CAST_FROM_STRING

CONFIG += ordered

SUBDIRS += \
           ut_unittests \
           ut_filtergenerator \
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
           ut_tileloading \
           ut_error \
           ut_format \
           ut_croppedthumbnail \
           ut_scheduler \
           ut_quillmetadata \
           ut_regions \
           ut_dbusthumbnailer \
           ut_autoclean \
           ut_filtering \
           benchmark  \

# --- install
tatam.files = tests.xml
tatam.path  = $$(DESTDIR)/usr/share/libquill-tests/

tatamimages.files += images/image_16x4.jpg
tatamimages.files += images/image_16x4.png
tatamimages.files += images/image_16x4.gif
tatamimages.files += images/exif.jpg
tatamimages.files += images/exif_orientation.jpg
tatamimages.files += images/xmp.jpg
tatamimages.files += images/iptc.jpg
tatamimages.files += images/redeye.jpg
tatamimages.files += images/redeye01.JPG
tatamvideo.files += video/Alvin_2.mp4

tatamimages.path  = $$(DESTDIR)/usr/share/libquill-tests/images/
tatamvideo.path  = $$(DESTDIR)/usr/share/libquill-tests/video/

INSTALLS += tatam tatamimages tatamvideo
