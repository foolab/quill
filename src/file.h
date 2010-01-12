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

#ifndef FILE_H
#define FILE_H

#include <QMetaType>
#include <QList>
#include <QFile>
#include <QuillImageFilter>

#include "quill.h"
#include "quillfile.h"

class QuillUndoStack;
class FilePrivate;

class File : public QObject
{
Q_OBJECT

    friend class ut_thumbnail;
    friend class ut_file;
    friend class ut_error;
public:
    File();
    virtual ~File();

    /*!
      Adds the file to the reference list of this file.
     */

    void addReference(QuillFile *file);

    /*!
      Removes the file from the reference list of this file.
     */

    void removeReference(QuillFile *file);

    /*!
      This must return true before the file can be deleted.
     */

    bool allowDelete();

    /*!
      Removes the file from the file lists of Core; invalidates all
      related QuillFile objects.
     */

    void detach();

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
      Sets the file name.

      Internal/testing use only.
    */

    void setFileName(const QString &fileName);

    /*!
      Sets the original file name.

      Internal/testing use only.
    */

    void setOriginalFileName(const QString &originalFileName);

    /*!
      Sets the file format.

      Internal/testing use only.
    */

    void setFileFormat(const QString &fileFormat);

    /*!
      Sets the target format.

      Internal/testing use only.
    */

    void setTargetFormat(const QString &targetFormat);

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
      Exports the file into a new name and format.

      @param format The file format. This parameter must be given if
      the format is not evident from the file extension.

    virtual QuillFile *exportFile(const QString &newFileName,
    const QString &fileFormat = "");*/

    /*!
      If the file is in the progress of saving.
    */

    virtual bool isSaveInProgress() const;

    /*!
      Returns true if the file has unsaved modifications.
    */

    virtual bool isDirty() const;

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
      which is available. This will not exceed the display level of
      the image. This function will not return tiles.
    */

    virtual QuillImage bestImage(int displayLevel) const;

    /*!
      Returns a representation of the current state of the file.

      @param level The resolution level to be used.

      @result a representation of the full image using exactly the
      given resolution level. This will not exceed the display level
      of the image. This function will not return tiles.
    */

    virtual QuillImage image(int level) const;

    /*!
      Sets the image representation of the current state of the
      file. The image is not protected; it might be overwritten or
      dropped at any time. The specific parameters (full image size)
      in QuillImage are currently ignored. This can currently only modify
      a full image or a preview, not tiles.
    */

    virtual void setImage(int level, const QuillImage &image);

    /*!
      Returns all image levels for the current state of the file.

      @result all representations of the full image which are
      currently cached, including all resolution levels and
      tiles. Anything over the display level of the file will be
      filtered out.
    */

    virtual QList<QuillImage> allImageLevels(int displayLevel) const;

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
      Returns the associated undo stack.

      Internal/testing use only.
     */

    QuillUndoStack *stack() const;

    /*!
      If the file exists in the file system.
     */

    virtual bool exists() const;

    /*!
      Sets file existence status
     */

    virtual void setExists(bool exists);

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
      Reads a complete file object from edit history.

      @param parent a Quill Core object.

      Internal/testing use only.
     */

    static File *readFromEditHistory(const QString &fileName,
                                     QuillError *error);

    /*!
      Copies a file over another file in the file system.

      Qt does not have an overwriting copy for platform reasons,
      this has been is implemented for efficiency reasons.
     */

    QuillError overwritingCopy(const QString &fileName,
                               const QString &targetName);

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
      Immediately concludes any save operation. Internal use only.
     */

    void concludeSave();

    /*!
      If the file has an original registered in core.
    */

    virtual bool hasOriginal();

    /*!
      Returns the original, read-only copy of this file instance.
    */

    virtual File *original();

    /*
      Sets the waiting-for-data status for this file.

      Set this to true if you want to start editing an image which is not
      completely available yet, for example if it is currently being
      transferred by the network. Use setImage() to immediately set a preview
      level. Set this to false when the full image has arrived.

      Warning: the status must be set before raising the preview level, or the
      image will get an "unsupported" status.
    */

    void setWaitingForData(bool status);

    /*!
      Returns the waiting-for-data status for the file. See setWaitingForData().
    */

    bool isWaitingForData() const;

    /*!
      A single image is available, instructs all referring QuillFile
      objects with a high enough display level to emit a signal.
     */

    void emitSingleImage(QuillImage image, int level);

    /*!
      A set of new tiles is available, instructs all referring QuillFile
      objects with a high enough display level to emit a signal.
     */

    void emitTiles(QList<QuillImage> tiles);

    /*!
      A set of pre-calculated images and tiles is possibly available,
      instructs all referring QuillFile objects with a high enough
      display level to emit a signal.
     */

    void emitAllImages();

    /*!
      Triggers an error.
     */

    virtual void emitError(QuillError error);

    // QString errorString() const;

    Quill::Error errorMessage() const;


signals:

    /*!
      Saving a file has been concluded on the background.
    */
    void saved();

    /*!
      The associated file has been removed.
     */
    void removed();

    /*
      There was an error in the file.
     */
    void error(QuillError error);

private:
    void prepareSave();

    static QString fileNameHash(const QString &fileName);

    static QString editHistoryFileName(const QString &fileName,
                                       const QString &editHistoryDirectory);

    /*!
      Writes the edit history.
     */

    void writeEditHistory(const QString &history, QuillError *error);

    FilePrivate *priv;

    QList<QuillFile*> m_references;
};

#endif // QUILLFILE_H
