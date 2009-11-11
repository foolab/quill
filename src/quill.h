/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Alexander Bokovoy <alexander.bokovoy@nokia.com>
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

#ifndef QUILL_H
#define QUILL_H

#include <QObject>
#include <QuillImageFilter>

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

    typedef enum _Error
    {
        ErrorUnspecified = -1,
        NoError = 0,
        ErrorFileNonexistent = 1,
        ErrorFormatUnsupported,
        ErrorFileCorrupt,
        ErrorDirectoryCannotCreate,
        ErrorFileLimitExceeded,
        ErrorThumbnailWriteFailed
    } Error;

    static QSize defaultViewPortSize;
    static int defaultCacheSize;

    /*!
      @param viewPortSize maximum viewport size
      @param threadingMode use for testing only!
    */
    Quill(const QSize &viewPortSize = Quill::defaultViewPortSize,
          Quill::ThreadingMode threadingMode = Quill::ThreadingNormal);
    virtual ~Quill();

    /*!
      Used to create a singleton pattern.
    */
    static Quill *instance();

    /*!
      Provides access to a file object. The object will be created
      if necessary. It will stay as property of the Quill object and
      kept in memory indefinitely.

      @param fileName The name of the file where changes are
      made. Must be non-empty. The file may be non-existing, in which
      case the file will be created. If creating the file fails, this
      operation will also fail. Will create directories if necessary.

      @param fileFormat The file format, if not evident from the file
      name. As in QImageReader::setFormat().
    */

    QuillFile *file(const QString &fileName,
                    const QString &fileFormat = "");

    /*!
      Modifies the preview level count. If new previews are
      created, both of their dimensions are twice those of the
      previous level by default. Calling this will fail if any files
      are either active or in the progress of saving.

      @param count The number of preview levels. Must be at least 1.
     */

    void setPreviewLevelCount(int count);

    /*!
      Returns the preview level count set by setPreviewLevelCount().
     */

    int previewLevelCount() const;

    /*!
      Sets the maximum number of files that can have their display level
      equal or bigger than the given level at one time.

      Normally, an application should make sure that file display
      levels are set to -1 whenever those files are not needed any
      more. The file limit can be considered an extra safeguard to
      make sure that there is a hard limit on memory used which is not
      exceeded at any time.

      If a setPreviewLevel() command violates the constraints given by
      this command, it will fail, and a QuillFile::error() of type
      Quill::FileLimitExceeded will be emitted.

      This limit can be modified at any time. Lowering the limit under
      the number of files on the level is allowed; in this case, any
      files already on the level will function normally.

      @param level Display level. Trying to set this limit on the full
      image level will fail, due to the limitations of the tile cache
      the limit for full images is always 1.

      @param limit Number of files allowed on the given level. The minimum
      value is 1; the default value is 1.
     */

    void setFileLimit(int level, int limit);

    /*!
      Returns the limit for the number of files that can be open at the
      given level at one time. See setFileLimit().
     */

    int fileLimit(int level) const;

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

    void setEditHistoryCacheSize(int level, int limit);

    /*!
      Returns the edit history cache size for the given preview
      level. See setEditHistoryCacheSize().
     */

    int editHistoryCacheSize(int level) const;

    /*!
      Sets the recommended size for preview images for a certain
      level. Calling this will fail if any files are either active, or
      in the progress of saving.
     */

    void setPreviewSize(int level, QSize size);

    /*!
      Sets the recommended size for preview images for a certain
      level. Calling this will fail if any files are either active, or
      in the progress of saving.
    */

    QSize previewSize(int level) const;

    /*!
      Sets the maximum tile size if tiling is in use.
      The default is 0, which disables tiling.
     */

    void setDefaultTileSize(const QSize &size);

    /*!
      Sets the tile cache size (measured in tiles, not bytes!)
      The default is 20.
    */

    void setTileCacheSize(int size);

    /*!
      Sets the maximum save buffer size, in pixels (4 bytes per pixel).
    */

    void setSaveBufferSize(int size);

    /*!
      Dumps the image editor state into a byte array which can be
      saved to the file system.
     */

    QByteArray dump();

    /*!
      Recovers the state of the image editor after a crash.
      All files start as closed.
    */

    void recover(QByteArray history);

    /*
      Sets the default directory where to look for edit histories.
      Default is $HOME/.config/quill/history.
     */

    void setEditHistoryDirectory(const QString &directory);

    /*!
      Sets the directory where to look for ready-made thumbnails
      for a given preview level. Default is empty, meaning that all
      preview images for the level are generated from the full image.
     */

    void setThumbnailDirectory(int level, const QString &directory);

    /*!
      Sets the file extension which is used in storing and retrieving thumbnail
      files. This is also used to determine the format of the thumbnail files.
      Does not include a full stop. Default is "png".
     */

    void setThumbnailExtension(const QString &format);

    /*!
      Enables or disables thumbnail creation. Disabling thumbnail
      creation will not abort a creation of an individual thumbnail
      currently in progress.
     */

    void setThumbnailCreationEnabled(bool enabled);

    /*!
      Returns true if thumbnail creation is enabled. True by
      default, this can be made false by setThumbnailCreationEnabled(), or if
      thumbnail creation fails by some reason.
     */

    bool isThumbnailCreationEnabled() const;

    /*!
      To make background loading tests easier on fast machines

      Only works if Quill has been created with threadingMode =
      Quill::ThreadingTest. This will allow the background thread to
      apply one image operation, and make the foreground thread to
      wait for it. Calling this is required, or the background thread
      will freeze forever.
    */

    void releaseAndWait();

    /*!
      To make background loading tests easier on fast machines
      @param delay extra delay per background operation, in seconds
    */

    void setDebugDelay(int delay);

 signals:

    /*!
      There was a generic error in Quill.
     */

    void error(Quill::Error errorCode);

private:

    QuillPrivate *priv;
};

#endif // __QUILL_PUBLIC_INTERFACE_H_
