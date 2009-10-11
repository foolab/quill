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

#ifndef __QUILL_CORE_H_
#define __QUILL_CORE_H_

#include <QObject>
#include <QtImageFilter>

#include "quill.h"

class QuillUndoCommand;
class QuillUndoStack;
class ImageCache;
class TileCache;
class CorePrivate;

class Core : public QObject
{
Q_OBJECT

friend class ut_stack;
friend class ut_core;


public:

    Core(const QSize &viewPortSize = Quill::defaultViewPortSize,
         Quill::ThreadingMode threadingMode = Quill::ThreadingNormal);
    ~Core();

    /*!
      Opens new file for viewing and editing.
    */

    QuillFile *file(const QString &fileName,
                    const QString &fileFormat);

    /*!
      Inserts a new file.
     */

    void insertFile(QuillFile *file, const QString &key);

    /*!
      Setting the limit for the undo cache(s).
     */

    void setCacheLimit(int level, int limit);

    /*!
      Modifies the preview level count. If new previews are created,
      both of their dimensions are twice those of the previous level
      by default. Calling this function after any files have been
      opened will fail.

      @param count The number of preview levels. Must be at least 1.
     */

    void setPreviewLevelCount(int count);

    /*!
      Gets the number of preview levels in use.
     */

    int previewLevelCount() const;

    /*!
      Gets the preview size of the given level.
    */

    QSize previewSize(int level) const;

    /*!
      Gets access to the undo cache of the given level.
    */

    ImageCache *cache(int level) const;

    /*!
      Sets the size of a preview-level image. Calling this function will fail
      if any files are open.
     */

    void setPreviewSize(int level, const QSize &size);

    /*!
      Sets the default tile size if tiling is in use.
      The default is 0, which disables tiling.
     */

    void setDefaultTileSize(const QSize &defaultTileSize);

    /*!
      Gets the default tile size if tiling is in use.
      Returns QSize() if tiling is not in use.
    */

    QSize defaultTileSize() const;

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
      The maximum save buffer size, in pixels (4 bytes per pixel).
    */

    int saveBufferSize() const;

    /*!
      Dumps the image editor state into a byte array which can be
      saved to the file system.
    */

    QByteArray dump() const;

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

    QString editHistoryDirectory() const;

    /*!
      Sets the directory where to look for ready-made thumbnails
      for a given preview level. Default is empty, meaning that all
      preview images for the level are generated from the full image.
     */

    void setThumbnailDirectory(int level, const QString &directory);

    QString thumbnailDirectory(int level) const;

    /*!
      Sets the file extension which is used in storing and retrieving thumbnail
      files. This is also used to determine the format of the thumbnail files.
      Does not include a full stop. Default is "png".
     */

    void setThumbnailExtension(const QString &format);

    QString thumbnailExtension() const;

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
      @param delay extra delay per operation in seconds
    */

    void setDebugDelay(int delay);

    /*
      From all stacks open and closed, finds one command by its unique id.
    */

    QuillUndoCommand *findInAllStacks(int id);

    /*
      Used to check if the background thread could be activated to do
      a task. If successful, will also start to do that task.
    */

    void suggestNewTask();

    /*!
      @return if the thread manager allows the deletion of a
      filter (so that it is not running on the background).
    */

    bool allowDelete(QuillImageFilter *filter) const;

    /*!
      Access to the tile cache of the core.
     */

    TileCache *tileCache() const;

    /*!
      The core has received an update of an image.
     */

    void emitImageAvailable(QuillFile *file, int level);

    /*!
      The core has received a partial update of an image.
    */

    void emitTileAvailable(QuillFile *file, int tileId);

private:

    /*
      Return one file.
    */

    QuillFile *priorityFile() const;

    /*
      Return one file.
    */

    QuillFile *prioritySaveFile() const;

    /*
      Return all existing files.
    */

    QList<QuillFile*> existingFiles() const;

    CorePrivate *priv;
};

#endif
