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

#ifndef FILE_H
#define FILE_H

#include <QMetaType>
#include <QList>
#include <QFile>
#include <QuillImageFilter>

#include "quill.h"
#include "quillfile.h"
#include "quillundostack.h"

class QTemporaryFile;
class QuillMetadata;

class File : public QObject
{
Q_OBJECT

    friend class ut_thumbnail;
    friend class ut_dbusthumbnail;
    friend class ut_file;
    friend class ut_error;
    friend class loadthumbs;

public:

    typedef enum {
        State_Initial,     //! Initial, currently not in use
        State_Normal,      //! File is usable, no problems
        State_NonExistent, //! File does not exist
        State_ExternallySupportedFormat, //! File format is only supported via
                                         //! D-Bus thumbnailer
        State_UnsupportedFormat, //! File format is not supported at all
        State_ReadOnly,          //! File is otherwise usable but cannot be
                                 //! written
        State_Placeholder,       //! File is a placeholder for coming data
        State_Saving             //! File is being saved
    } State;

    File();
    ~File();

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

    QString fileName() const;

    /*!
      Returns the original file name associated with the QuillFile
      object. The original is used to save the unmodified backup of
      the file.
    */

    QString originalFileName() const;

    /*!
      Returns format associated with the QuillFile object.
    */

    QString fileFormat() const;

    /*!
      Returns the target format associated with the QuillFile object.
      This is different from fileFormat() if the file has been
      exported into a different format.
    */

    QString targetFormat() const;

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

    void setReadOnly();

    bool isDisplayLevelEnabled(int level) const;

    /*!
      Sets the display level of the file.
      -1 = no display, 0 = first level only, ...
      previewlevelcount = full image or tiles.

      Default is -1. This can be modified on the fly.

      If Quill::fileLimit() of the target level, or any level below
      it, has been exceeded, this command does nothing, returns false,
      and an error() signal is emitted.
    */

    bool setDisplayLevel(int level);

    /*!
      Returns the current display level of the file.
    */

    int displayLevel() const;

    /*!
      File priority is always the highest of the priorities of
      QuillFiles referring to the File object. Any time there is a new
      file reference, one is deleted, or its priority is changed, the
      priority is recalculated.
    */

    void calculatePriority();

    /*!
      Returns file priority.
    */

    int priority() const;

    /*!
      Starts to asynchronously save any changes made to the file (if
      any). If there were any changes, the saved() signal is emitted
      when the changes are finished.
    */

    void save();

    /*!
      Exports the file into a new name and format.

      @param format The file format. This parameter must be given if
      the format is not evident from the file extension.
    */

    /*!
      If the file is in the progress of saving.
    */

    bool isSaveInProgress() const;

    /*!
      Returns true if the file has unsaved modifications.
    */

    bool isDirty() const;

    /*!
      Starts to asynchronously run an operation.
     */

    void runFilter(QuillImageFilter *filter);

    /*!
      Starts an undo session. When an undo session is in progress,
      no undo/redo outside the session is permitted. A closed
      undo session will be treated as one operation by undo() and
      redo().
     */

    void startSession();

    /*!
      Concludes an undo session started by startSession(). A closed
      undo session will be treated as one operation by undo() and
      redo().
     */

    void endSession();

    /*!
      If an undo session is in progress. See startSession() and
      endSession().
    */

    bool isSession() const;

    /*!
      If the previous operation can be undone.
     */

    bool canUndo() const;

    /*!
      Undoes the previous operation.
     */

    void undo();

    /*!
      If an undone operation can be redone.
     */

    bool canRedo() const;

    /*!
      Redoes a previously undone operation.
     */

    void redo();

    /*!
      Drops the redo history so it cannot be recovered anymore.
    */

    void dropRedoHistory();

    /*!
      Returns a representation of the current state of the file.

      @result the highest-resolution representation of the full image
      which is available. This will not exceed the display level of
      the image. This function will not return tiles.
    */

    QuillImage bestImage(int displayLevel) const;

    /*!
      Returns a representation of the current state of the file.

      @param level The resolution level to be used.

      @result a representation of the full image using exactly the
      given resolution level. This will not exceed the display level
      of the image. This function will not return tiles.
    */

    QuillImage image(int level) const;

    /*!
      Sets the image representation of the current state of the
      file. The image is not protected; it might be overwritten or
      dropped at any time. The specific parameters (full image size)
      in QuillImage are currently ignored. This can currently only modify
      a full image or a preview, not tiles.
    */

    void setImage(int level, const QuillImage &image);

    /*!
      Returns all image levels for the current state of the file.

      @result all representations of the full image which are
      currently cached, including all resolution levels and
      tiles. Anything over the display level of the file will be
      filtered out.
    */

    QList<QuillImage> allImageLevels(int displayLevel) const;

    /*!
      Returns the full image size, in pixels, of the current state of the file.
    */

    QSize fullImageSize() const;

    /*!
      Sets the viewport area (in full-image coordinates), only
      takes effect if tiling is in use. Only tiles within the viewport
      will be processed and returned. The default is no viewport
      (e.g. no tiles will be returned).
    */

    void setViewPort(const QRect &viewPort);

    /*!
      Returns the viewport in full-image coordinates. Only tiles
      within the viewport will be processed and returned.
     */

    QRect viewPort() const;

    /*!
      Check if an image size passes the maximum image size constraints.
     */

    bool checkImageSize(const QSize &fullImageSize);

    /*!
      If two timestamps match so closely that they can be
      considered the same.  Currently, since to FAT file system
      inaccuracies, the tolerance is 1 second.
     */

    static bool isMatchingTimestamp(QDateTime stamp1, QDateTime stamp2);

    /*!
      If a related thumbnail has been cached to the file system.
     */

    bool hasThumbnail(int level);

    /*!
      If a related "failed" thumbnail has been cached to the file system.
     */

    bool hasFailedThumbnail();

    /*!
      Adds a "failed" thumbnail for the file to the file system,
      preventing further thumbnailing or loading attempts.
     */

    void addFailedThumbnail();

    /*!
      Gets the file name associated with the given preview level.

      @return the file name associated with the given preview level,
      or an empty string if the given level is not cached. The file is
      not guaranteed to exist.
     */

    QString thumbnailFileName(int level);

    QString failedThumbnailFileName();

    /*!
      If there are unsaved thumbnails available
     */

    bool hasUnsavedThumbnails();

    /*!
      Returns the associated undo stack.

      Internal/testing use only.
     */

    QuillUndoStack *stack() const;

    /*!
      Returns the state of the file.
    */

    State state() const;

    /*!
      Changes the state of the file.
     */

    void setState(State state);

    /*!
      Returns true if the file supports viewing thumbnails.
    */
    bool supportsThumbnails() const;

    /*!
      Returns true if the file supports viewing.
    */
    bool supportsViewing() const;

    /*!
      Returns true if the file supports editing.
    */
    bool supportsEditing() const;

    /*!
      If the file exists in the file system.
     */

    bool exists() const;

    /*!
      Gets the (cached) last modification time of a file.
    */

    QDateTime lastModified() const;

    /*!
      Refreshes (reads from file) the last modification time of a file.
    */

    void refreshLastModified();

    /*!
      Sets file existence status
     */

    void setExists(bool exists);

    /*!
      Tries to force Quill to acknowledge the file as supported or
      unsupported. This should only be used after the contents of a
      file have been changed. If this is set to true with a file not
      recognizable by Quill, it will revert to unsupported.
     */

    void setSupported(bool supported);

    /*!
      Marks the file as being supported by (at least) the D-Bus thumbnailer.
     */

    void setThumbnailSupported(bool format);

    /*!
      Reads a complete file object from edit history.

      @param parent a Quill Core object.

      Internal/testing use only.
     */

    void readFromEditHistory(const QString &fileName,
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
    void remove();

    /*!
      Removes the thumbnails for a file. This operation cannot be undone.
     */
    void removeThumbnails();

    /*!
      Immediately concludes any save operation. Internal use only.
     */

    void concludeSave();

    /*!
      Aborts a save operation in progress, freeing any resources.
    */

    void abortSave();

    /*!
      Concludes a thumbnail save operation (internally registers
      thumbnails as saved).
     */

    void registerThumbnail(int level);

    /*!
      Internally removes registration thumbnail as being saved
      */
    void unregisterThumbnail(int level);

    /*!
      If the file has an original registered in core.
    */

    bool hasOriginal();

    /*!
      Returns the original, read-only copy of this file instance.
    */

    File *original();

    /*!
      Processes any filter I/O errors related to the file.
    */

    void processFilterError(QuillImageFilter *filter);

    /*!
      Trigger an error if a full image size violates the set limits.
    */

    void imageSizeError();

    /*!
      Reloads changes from the file system.
     */

    void refresh();

    /*!
      Returns true for images that are in Placeholder state, false otherwise.
    */

    bool isWaitingForData() const;

    /*!
      There are errors in the thumbnail cache for this file,
      thumbnail reading should be disabled.
     */

    void setThumbnailError(bool status);

    /*!
      Returns true if there are any errors in the thumbnails for this file
     */

    bool hasThumbnailError() const;

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

    void emitError(QuillError error);

     /*!
      If we can revert to the original operation
     */
    bool canRevert() const;

    /*!
      Reverts to the original operation
     */
    void revert();

    /*!
      If we can restore to the pre-revert operation
     */
    bool canRestore() const;

    /*!
      Restors to the pre-revert operation
     */
    void restore();

    static QByteArray filePathToUri(const QString &filePath);

    static QString filePathHash(const QString &filePath);

    /*!
      If the file is an original one
     */
    bool isOriginal() const;

    /*!
      Sets a flag for the original file
     */
    void setOriginal(bool flag);

    /*!
      If the edit history is read
    */
    bool readEditHistory() const;

    /*!
      Sets the file index name for original files
    */
    void setFileIndexName(const QString indexName);

    /*!
      Gets the file index name for original files, if the file is
      non-original file, the file name is returned.
    */
    QString fileIndexName() const;

    /*!
      Touches one thumbnail for a file. This needs to be called after
      a thumbnail has been created so that the thumbnail date can be set
      to the same date as the main file since without it, the thumbnail
      will be considered invalid.
     */
    void touchThumbnail(int level);

     /*!
      If the file is a SVG file
     */
    bool isSvg() const;

    /*!
      Returns the current error state
     */
    QuillError error() const;

signals:

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
    void error(QuillError error);

private:
    bool isJpeg() const;

    void prepareSave();

    static QString editHistoryFileName(const QString &fileName,
                                       const QString &editHistoryDirectory);

    /*!
      Writes the edit history.
     */

    void writeEditHistory(const QString &history, QuillError *error);

    /*!
      Touches all thumbnails for a file. This is usable if the main
      file has been changed and thumbnails don't need regenerating.
     */
    void touchThumbnails();

    /*!
      Internal implementation of setting the display level without scheduling
      a new task. It is used by \ref setImage function that implicates no loading
      from file system should happen.
      */
    void setDisplayLevelInternal(int level);

private:

    static const int timestampTolerance;

    /*!
      Enumeration for thumbnail existence: yes, no, or not yet known
      */
    enum ThumbnailExistenceState {
        Thumbnail_UnknownExists = 0,
        Thumbnail_Exists,
        Thumbnail_NotExists,
    };

    QList<QuillFile*> m_references;

    State m_state;

    bool m_hasThumbnailError;
    QDateTime m_lastModified;

    QuillUndoStack *m_stack;

    int m_displayLevel;
    int m_priority;
    QMap<int, ThumbnailExistenceState> m_hasThumbnail; //! Caches information of thumbnail existence in the file system. Absent values are interpreted as Thumbnail_UnknownExists.

    QString m_fileName;
    QString m_originalFileName;
    QString m_fileFormat;
    QString m_targetFormat;
    QString m_fileNameHash;

    QRect m_viewPort;

    QTemporaryFile *m_temporaryFile;
    //one flag for the original file
    bool m_original;
    //the flag for the stack when we read the edit history xml file
    bool m_hasReadEditHistory;
    //the index name is different from file name, for orignial file, the index name starts with "\",
    QString m_fileIndexName;

    QuillError m_error;


    void ResetOrientationTag(QuillMetadata &metadata);

    Q_DISABLE_COPY(File)
};

#endif // QUILLFILE_H
