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

#ifndef QUILLFILE_H
#define QUILLFILE_H

#include <QObject>
#include "quill.h"

class QuillImage;
class QuillImageFilter;
class QuillFilePrivate;
class QSize;
class QRect;
class File;

typedef QList<QuillImage> QuillImageList;
Q_DECLARE_METATYPE(QuillImageList)

class QuillFile : public QObject
{
Q_OBJECT

    friend class File;

public:
    /*!
      Provides access to a file object. The object will be created
      if necessary. The object becomes property of the caller and can
      be destroyed at any time.

      @param fileName The name of the file where changes are
      made. Must be non-empty. The file may be non-existing, in which
      case the file will be created. If creating the file fails,
      this operation will also fail, returning a QuillFile with isValid()
      set to false. Will create directories if necessary.

      @param fileFormat The file format, if not evident from the file
      name. As in QImageReader::setFormat().
    */

    QuillFile(const QString &fileName,
              const QString &fileFormat = "");


    virtual ~QuillFile();

    /*!
      Returns the actual file name associated with the QuillFile object.
    */

    virtual QString fileName() const;

    /*!
      Returns the original file name associated with the QuillFile
      object. The original is used to save the unmodified backup of
      the file.
    */

    virtual QString originalFileName() const;

    /*!
      Returns format associated with the QuillFile object.
    */

    virtual QString fileFormat() const;

    /*!
      Returns the target format associated with the QuillFile object.
      This is different from fileFormat() if the file has been
      exported into a different format.
    */

    virtual QString targetFormat() const;

    /*!
      Sets file as read only, disabling all edits. Only makes sense
      with originals.
    */

    virtual void setReadOnly();

    /*!
      If the file is read only.
    */

    virtual bool isReadOnly() const;

    /*!
      Sets the display level of the file.
      -1 = no display, 0 = first level only, ...
      previewlevelcount = full image or tiles.

      Default is -1. This can be modified on the fly.

      If Quill::fileLimit() of the target level, or any level below
      it, has been exceeded, this command does nothing, returns false,
      and an error() signal is emitted.
    */

    virtual bool setDisplayLevel(int level);

    /*!
      Returns the current display level of the file.
    */

    virtual int displayLevel() const;

    /*!
      Starts to asynchronously save any changes made to the file (if
      any). If there were any changes, the saved() signal is emitted
      when the changes are finished.
    */

    virtual void save();

    /*!
      If the file is in the progress of saving.
    */

    virtual bool isSaveInProgress() const;

    /*!
      Starts to asynchronously run an operation.
     */

    virtual void runFilter(QuillImageFilter *filter);

    /*!
      Starts an undo session. When an undo session is in progress,
      no undo/redo outside the session is permitted. A closed
      undo session will be treated as one operation by undo() and
      redo().
     */

    virtual void startSession();

    /*!
      Concludes an undo session started by startSession(). A closed
      undo session will be treated as one operation by undo() and
      redo().
     */

    virtual void endSession();

    /*!
      If an undo session is in progress. See startSession() and
      endSession().
    */

    virtual bool isSession() const;

    /*!
      If the previous operation can be undone.
     */

    virtual bool canUndo() const;

    /*!
      Undoes the previous operation.
     */

    virtual void undo();

    /*!
      If an undone operation can be redone.
     */

    virtual bool canRedo() const;

    /*!
      Redoes a previously undone operation.
     */

    virtual void redo();

    /*!
      Returns a representation of the current state of the file.

      @result the highest-resolution representation of the full image
      which is available. This can exceed the display level of the
      image. This function will not return tiles.
    */

    virtual QuillImage image() const;

    /*!
      Returns a representation of the current state of the file.

      @param level The resolution level to be used.

      @result a representation of the full image using exactly the
      given resolution level. This can exceed the display level of the
      image. This function will not return tiles.
    */

    virtual QuillImage image(int level) const;

    /*!
      Returns all image levels of the current state of the file.

      @result all representations of the full image which are currently cached,
      including all resolution levels and tiles.
    */

    virtual QList<QuillImage> allImageLevels() const;

    /*!
      Returns the full image size, in pixels, of the current state of the file.
    */

    virtual QSize fullImageSize() const;

    /*!
      Sets the viewport area (in full-image coordinates), only
      takes effect if tiling is in use. Only tiles within the viewport
      will be processed and returned. The default is no viewport
      (e.g. no tiles will be returned).
    */

    virtual void setViewPort(const QRect &viewPort);

    /*!
      Returns the viewport in full-image coordinates. Only tiles
      within the viewport will be processed and returned.
     */

    virtual QRect viewPort() const;

    /*!
      If a related thumbnail has been cached to the file system.
     */

    virtual bool hasThumbnail(int level) const;

    /*!
      Gets the file name associated with the given preview level.

      @return the file name associated with the given preview level,
      or an empty string if the given level is not cached. The file is
      not guaranteed to exist.
     */

    virtual QString thumbnailFileName(int level) const;

    /*!
      If the file exists in the file system.
     */

    virtual bool exists() const;

    /*!
      If the file is supported and has no errors.
     */

    virtual bool supported() const;

    /*!
      Tries to force Quill to acknowledge the file as supported or
      unsupported. This should only be used after the contents of a
      file have been changed. If this is set to true with a file not
      recognizable by Quill, it will revert to unsupported.
     */

    virtual void setSupported(bool supported);

    /*!
      Completely removes a file along with its associated original
      backup, edit history, and any thumbnails. Does not remove the
      associated file object. This operation cannot be undone.
     */
    virtual void remove();

    /*!
      Removes the thumbnails for a file. This operation cannot be undone.
     */
    virtual void removeThumbnails();

    /*!
      Returns the original, read-only copy of this file instance.
    */

    virtual QuillFile *original();

    /*!
      Returns true if Quill Core holds an internal file object
      related to this one.
     */

    virtual bool isValid();

signals:
    /*!
      Triggered when there is a new image representation available on
      the active state of the file.

      @result a set of preview levels, the full version, or a tile
      representing the active state of the file.
     */

    void imageAvailable(const QuillImageList images);

    /*!
      Saving a file has been concluded on the background.
    */
    void saved();

    /*!
      The associated file has been removed.
     */
    void removed();

    /*!
      There was an error in the file.
     */
    void error(Quill::Error errorCode);

private:

    /*!
      Constructor creating an invalid QuillFile object.
     */
    QuillFile();

    /*!
      Attaches an internal file object to the QuillFile object.
     */
    void attach(File *file);

    /*
      The referred file object has been destroyed.
     */
    void invalidate();

private:
    QuillFilePrivate *priv;
};

#endif
