/****************************************************************************
**
** Copyright (C) 2009-11 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Pekka Marjola <pekka.marjola@nokia.com>
**
** This file is part of the Quill package.
**
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
****************************************************************************/

/*!
  \mainpage LibQuill Reference Documentation

  \section intro_sec Introduction

LibQuill is a image viewer/editor library with background processing
capabilities, optimized for mobile devices. It can be used as a
background library for building an image viewing or editing
application with a GUI, or it can as well be used as a back-end for an
application without a GUI, like example a batch job image processor or
an automatic thumbnailer. The current LibQuill does not have rendering
capabilities, instead of that all relevant image data is forwarded to
the GUI application. LibQuill currently does all its operations on
software but it is easily extendable into using special hardware in
its operations.

LibQuill currently has a good basic set of image editing
operations. It supports the Qt image format plugins and thus can use
any format which has such a plugin. There are also some optimizations
specific to the Jpeg format.

  \section features_sec Features

  \subsection undo_sec Unlimited undo/redo

For each image which has been edited, an original copy is
preserved. In addition, all editing operations until the current
situation, called "commands", are saved into a separate file in xml
format. This file, along with the original copy, can be then used to
reconstruct any point in the editing history of the file. Thus, the
complete edit history will be accessible during any later session.

  \subsection bg_sec Background processing

Image editing operations, when done on software, are known to consume
noticeable amounts of time. To avoid freezing the user interface
during this time, LibQuill does all its image processing on a separate
background thread.

  \subsection concurrent_sec Concurrent for many files

LibQuill can have a large number of files open at the same time. This
not only means that editing can proceeds into a next image when saving
a previous one is in progress, it also means that several files can be
loaded, displayed and edited at the same time.

  \subsection incremental_sec Incremental

As image processing operations for large images might take more time
than what is comfortable for the user, we have adopted a policy of
incrementality. This means that any operation is first applied to a
"preview", a downscaled version of the full image. This allows us to
display a good estimate quickly for the user, who will get instant
feedback on his edit and can even proceed with editing when the
background operation for a full image is still in progress. As some
operations are known to produce different results for lower-scale
images than higher-scale ones, careful design is needed for
implementations of such edit operations.

  \subsection tiled_sec Tiled

As complete images in 32-bit format take a lot of memory which is
scarce on a mobile device, they should never be loaded into the memory as
such (unless they are very small). Instead, rectangular tiles are used
to display regions of interest at the request of the user, and also
when processing with saving any modifications to the file system. This
allows for a more efficient memory use and, for some cases, a faster
response time. Again, careful design of image editing operations is
required to produce an end result with good quality.

  \section sturcture_sec Package structure

LibQuill comes in two packages: QuillImageFilters and LibQuill.

QuillImageFilters contain a standard set of image editing operations,
including file loading and saving. They all implement a simple
standard interface. QuillImageFilters can be also used as a
stand-alone package without LibQuill proper.

LibQuill proper is a meta-editing library, responsible for file
management, undo history, image caching and the scheduling and running
of different edit operations. It contains no image editing operations
on its own.
 */

/*!
  \class Quill

  \brief Class containing all global settings and other centralized
  information for LibQuill.
*/

#ifndef QUILL_H
#define QUILL_H

#include <QObject>
#include <QuillImageFilter>
#include "quillerror.h"

class QImage;
class QString;
class QSize;
class QuillImage;
class QuillFilterGenerator;
class QuillFile;

class QuillPrivate;

class Quill : public QObject
{
Q_OBJECT

public:

    /*!
      For testing use only.
    */

    typedef enum _ThreadingMode
    {
        ThreadingNormal,
        ThreadingTest
    } ThreadingMode;

    static QSize defaultViewPortSize;
    static int defaultCacheSize;

    /*!
      Starts Quill with testing mode. This operation will fail if
      Quill is already in use. In testing mode, any background
      operation will require a call to releaseAndWait() before
      completing. This setting should only be used by unit tests.
     */

    static void initTestingMode();

    /*!
      Destroys the Quill core and resets all settings. All
      QuillFiles will have their valid() property set to false. If
      Quill or QuillFiles are used after this command,
      Quill will start with a fresh core.

      There should be no reason to call this function except at
      program cleanup, or cleaning up the state between different unit
      tests.
     */

    static void cleanup();

    /*!
      Modifies the preview level count. If new previews are
      created, both of their dimensions are twice those of the
      previous level by default. Calling this will fail if any files
      are either active or in the progress of saving.

      @param count The number of preview levels. Must be at least 1.
     */

    static void setPreviewLevelCount(int count);

    /*!
      Returns the preview level count set by setPreviewLevelCount().
     */

    static int previewLevelCount();

    /*!
      Sets the maximum number of edit history steps that can be cached
      for a given display level. The cached images are used for fast
      undo and redo.

      This limit is not unique to any QuillFile; edit history images
      from different files occupy the same cache. The expiration
      policy used by the cache is approximately FIFO.

      Regardless of this limit, any point in the edit history of any
      file can be recovered by Quill, it will just be slower.

      @param level preview level

      @param limit The cache size for this level, in number of edit
      history steps (images) saved. The minimum value is 0 (no caching);
      the default value is 0.
    */

    static void setEditHistoryCacheSize(int level, int limit);

    /*!
      Returns the edit history cache size for the given preview
      level. See setEditHistoryCacheSize().
     */

    static int editHistoryCacheSize(int level);

    /*!
      Sets the recommended size for preview images for a certain
      level. Calling this will fail if any files are either active, or
      in the progress of saving.
     */

    static void setPreviewSize(int level, const QSize &size);

    /*!
      Sets the recommended size for preview images for a certain
      level. Calling this will fail if any files are either active, or
      in the progress of saving.
    */

    static QSize previewSize(int level);

    /*!
      Sets the minimum size for preview images of a certain level. The
      default is no size, which is represented by an invalid size
      parameter. If either dimension of this value is valid, libquill
      will use cropped scaling to ensure that the minimum size
      requirement is met. However, libquill will never change the aspect
      ratio of an image, nor upscale an image which is smaller than
      this set minimum size.

      Note that cropped preview levels (ones with minimum size set)
      cannot be automatically substituted to other levels.
     */

    static void setMinimumPreviewSize(int level, const QSize &size);

    /*!
      Gets the minimum size of a preview size for a level. See
      setMinimumPreviewSize().
    */

    static QSize minimumPreviewSize(int level);

    /*!
      Sets the maximum tile size if tiling is in use.
      The default is 0, which disables tiling.
     */

    static void setDefaultTileSize(const QSize &size);

    /*!
      Sets the tile cache size (measured in tiles, not bytes!)
      The default is 20.
    */

    static void setTileCacheSize(int size);

    /*!
      Sets the maximum save buffer size, in pixels (4 bytes per pixel).
    */

    static void setSaveBufferSize(int size);

    /*!
      Sets the maximum allowed dimensions for an image. If either
      dimension of an image overflows its respective limit set here,
      the image will not be handled by Quill.

      This limit will not prevent image filters from resizing an image
      into bigger than the given size.

      The default is no limit, which is represented by an invalid
      size.

      See also setImagePixelsLimit().
     */

    static void setImageSizeLimit(const QSize &size);

    /*!
      Returns the maximum allowed dimensions for an image. See
      setImageSizeLimit().
    */

    static QSize imageSizeLimit();

    /*!
      Sets the maximum number of allowed pixels for an image. Images
      which have more pixels than this number will not be handled by
      Quill. This does not replace imageSizeLimit(); both limits
      are checked separately.

      The default is no limit, which is represented by the value 0.
     */

    static void setImagePixelsLimit(int pixels);

    /*!
      Returns the maximum number of allowed dimensions for an image. See
      setImagePixelsLimit().
    */

    static int imagePixelsLimit();

    /*!
      Sets the maximum number of allowed pixels for image formats which
      do not support tiling. (Currently, this means all formats except Jpeg.)
      Images which have more pixels than this number will not be handled by
      Quill. If tiling is not enabled with setDefaultTileSize(), this
      maximum size applies to all images.

      The default is no limit, which means that the generic image pixels limit
      will be used (see setImagePixelsLimit() ).
     */

    static void setNonTiledImagePixelsLimit(int pixels);

    /*!
      Returns the maximum number of allowed pixels for image formats which
      do not support tiling. See setNonTilingImagePixelsLimit().
    */

    static int nonTiledImagePixelsLimit();

    /*
      Sets the path where Quill will store and retrieve edit histories.
      Default is $HOME/.config/quill/history .
     */

    static void setEditHistoryPath(const QString &path);

    /*!
      Sets the base path under which thumbnails are saved. This
      defaults to the freedesktop standard which is .thumbnails under
      QDir::homePath().
     */

    static void setThumbnailBasePath(const QString &path);

    /*!
      Sets the unique thumbnail flavor name for a given display
      level. This is also the name of the directory under the
      thumbnail base path which is used as the file system cache for
      thumbnails of this level.

      Not setting this value or setting this to empty means that
      thumbnails of this level will not be cached to the file system.
    */

    static void setThumbnailFlavorName(int level, const QString &name);

    /*!
      Sets the file extension which is used in storing and retrieving thumbnail
      files. This is also used to determine the format of the thumbnail files.
      Does not include a full stop. Default is "png".
     */

    static void setThumbnailExtension(const QString &format);

    /*!
      Enables or disables thumbnail creation. Disabling thumbnail
      creation will not abort a creation of an individual thumbnail
      currently in progress.
     */

    static void setThumbnailCreationEnabled(bool enabled);

    /*!
      Returns true if thumbnail creation is enabled. True by
      default, this can be made false by setThumbnailCreationEnabled(), or if
      thumbnail creation fails by some reason.
     */

    static bool isThumbnailCreationEnabled();

    /*!
      Enables or disables thumbnail creation using an external D-Bus
      thumbnailer (in cases of files which are not directly supported
      by Quill). The external thumbnailer must be configured to
      support all the preview levels that are specified using
      setThumbnailDirectory(). This option is true by default.
    */

    static void setDBusThumbnailingEnabled(bool enabled);

    /*!
      Returns true if the external D-Bus thumbnailer has been selected to be
      used.
    */

    static bool isDBusThumbnailingEnabled();

    /*!
      Sets the path where Quill will store its temporary files.
      The temporary files are currently not autocleaned in case of
      crash, so it is recommended to use a temporary partition for
      such files.
    */
    static void setTemporaryFilePath(const QString &path);

    /*!
      The path where Quill will store its temporary files. See
      setTemporaryFilePath().
    */

    static QString temporaryFilePath();

    /*!
      Sets the default color for alpha channel rendering.

      As Quill does not support alpha channel editing, the alpha
      channel is immediately and permanently rendered as the
      background rendering color.
     */

    static void setBackgroundRenderingColor(const QColor &color);

    /*!
      Gets the default color for alpha channel rendering.

      See setBackgroundRenderingColor().
    */

    static QColor backgroundRenderingColor();

    /*!
      Sets the bounding box for vector graphics rendering.

      Vector graphics (mimetype "image/svg+xml") will be
      rendered to this size, preserving aspect ratio. This
      size cannot be bigger than the biggest preview level.

      Default is an invalid QSize, which means that all
      vector graphics is rendered into their intended size.
     */

    static void setVectorGraphicsRenderingSize(const QSize &size);

    /*!
      Returns the bounding box for vector graphics rendering.

      See setVectorGraphicsRenderingSize().
    */

    static QSize vectorGraphicsRenderingSize();

    /*!
      Returns true if there is any calculation in progress.
     */

    static bool isCalculationInProgress();

    /*!
      Returns true if there are any files which are in the progress of
      saving. Useful for example when an application wants to exit.
     */

    static bool isSaveInProgress();

    /*!
      Returns the names list of files which are in the progress of
      saving.
     */

    static QStringList saveInProgressList();

    /*!
      Returns the names list of files that are locked by any process.
     */

    static QStringList lockedFiles();

    /*!
      Puts the calling thread (which must be the thread Quill
      object was created in) to sleep until Quill has synchronized its
      state with the file system and it is safe to exit the
      application. This means that all files in progress of saving
      have finished, however excluding thumbnail saving.

      Alternatively, a timeout value can be set so that after a
      certain time even if the saving has not finished, the call will
      return.

      @param msec Timeout in milliseconds, or 0 for no timeout.

      @return true if saving was finished, false if timeout occurred.
     */

    static bool waitUntilFinished(int msec = 0);

    /*!
      To make background loading tests easier on fast machines

      Only works if Quill has been created with threadingMode =
      Quill::ThreadingTest. This will allow the background thread to
      apply one image operation, and make the foreground thread to
      wait for it. Calling this is required, or the background thread
      will freeze forever.
    */

    static void releaseAndWait();

    /*!
      Returns the singleton instance of Quill. This should only be
      used for signals of the QObject class. The returned object
      should never be deleted, use Quill::cleanup() instead.
    */

    static Quill *instance();

signals:
    /*!
      Edits to a file have been successfully saved.

      @param filePath the path to the file which has been saved.

      Since it may be that all QuillFile objects related to a save in
      progress have been deleted already, a successful save triggers
      both QuillFile::saved() and this signal.
     */

    void saved(QString filePath);

    /*
      A file has been removed.

      This currently applies only if the file has been explicitly removed
      with QuillFile::remove().
     */

    void removed(QString filePath);

    /*!
      Any error not specific to any individual QuillFile is reported
      by this signal.

      Since it may be that all QuillFile objects related to a save in
      progress have been deleted already, errors related to saving
      trigger both QuillFile::error() and this signal.

      @param errorCode The error code enumeration
      @param data Any other information specific to the error
     */

    void error(QuillError error);

private:

    /*!
      This class is a static class, so no constructors should not be invoked
      from the outside.
    */

    Quill();
    ~Quill();
    Q_DISABLE_COPY(Quill);
private:
    QuillPrivate *priv;
    static Quill *g_instance;
};

#endif // __QUILL_PUBLIC_INTERFACE_H_
