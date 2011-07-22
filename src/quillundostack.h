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
  \class QuillUndoStack

  \brief Contains a QUndoStack specialized for QuillUndoCommands,
  also stores related non-undoable commands like saving and loading.

In addition to normal stack operations, QuillUndoStack can be used
used for querying for images from several commands in the undo stack.

Edit sessions are a generic way to merge groups of operations
together. These can be used if an image editor has a hierarchical
structure of edit modes (for example, a "red eye removal mode"). See
startSession() and endSession() for more details.

QuillUndoStack also supports structures speficicly related to saving
images, like the SaveMap, and the commands prepareSave() and
concludeSave() which are used to help with these operations.
 */

#ifndef __QUILL_UNDO_STACK_H__
#define __QUILL_UNDO_STACK_H__

#include "quill.h"

class QuillImage;
class File;
class QuillUndoCommand;
class QuillImageFilter;
class SaveMap;
class QuillImageFilterGenerator;
class QUndoStack;
class Logger;

class QuillUndoStack : public QObject
{
Q_OBJECT

public:
    QuillUndoStack(File *file);
    ~QuillUndoStack();

    File *file();

    void load();

    /*
     Refresh the load filter by detecting the format of the given file again.
     Typically used together with File::refresh().
     */
    void refresh();

    void add(QuillImageFilter *filter);

    /*!
      Starts an undo session. When an undo session is in progress,
      no undo/redo outside the session is permitted. A closed undo
      session will be treated as one operation by undo() and
      redo(). Undo sessions can be thought as a more generic
      replacement of QUndoCommand::mergeWith(). Unlike it, a
      session can also combine different kinds of filters and requires
      no support from the QUndoCommand subclass implementation.
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
      Check if it is possible to undo the previous command.
      Some commands (e.g. load, save) cannot be undone.
    */

    bool canUndo() const;

    /*!
      Undo the previous command if possible, otherwise do nothing.
      Some commands (e.g. load, save) cannot be undone.
    */

    void undo();

    /*!
      Check if it is possible to redo a command.
    */

    bool canRedo() const;

    /*!
      Redo a command if possible, otherwise do nothing.
    */

    void redo();

    /*!
      Drops any redo history, not recoverable.
     */

    void dropRedoHistory();

    /*!
      Gets the current image, best resolution available.
    */

    QuillImage bestImage(int maxLevel) const;

    /*!
      Query if the current image exists with the given resolution.
    */

    bool hasImage(int level) const;

    /*!
      Gets the current image, given resolution.
    */

    QuillImage image(int level) const;

    /*!
      Allows forcibly setting an image value from outside.
    */

    void setImage(int level, const QuillImage &image);

    /*!
      Gets the current image in all available image levels.
    */
    QList<QuillImage> allImageLevels(int maxLevel) const;

    /*!
      Gets the pre-calculated full image size for the current image.
    */

    QSize fullImageSize();

    /*!
      Pre-calculates the full image size for the current image.
    */

    void calculateFullImageSize(QuillUndoCommand *command);

    /*!
      Count of the elements in the stack (same as QUndoStack)
    */

    int count() const;

    /*!
      Index of the next command to be added (same as QUndoStack)
     */

    int index() const;

    /*!
      The current command of the stack (with index()-1)
    */

    QuillUndoCommand *command() const;

    /*!
      Should be called by QuillUndoCommand only
     */
    QuillUndoCommand *command(int index) const;

    /*!
      Find a command with a given id (not index!)

      Used for background thread calculations.
    */
    QuillUndoCommand *find(int id) const;

    /*!
      If the stack is clean (has no elements at all).

      Normally, the stack should always have elements, at least an
      initial load command.
    */

    bool isClean() const;

    /*!
      Sets the index of the stack command whose image is currently saved
      into the file fileName().
    */

    void setSavedIndex(int index);

    /*!
      Gets the index of the stack command whose image is currently saved
      into the file fileName().
     */

    int savedIndex() const;

    /*!
      If the current stack is dirty (ie. a new saving is needed)
    */

    bool isDirty() const;

    /*!
      Prepares the stack for saving.
      This is currently used by non-tiled saving only - tiled
      saving uses ad-hoc save filters.
    */

    void prepareSave(const QString &fileName,
                     const QByteArray &rawExifDump);

    /*!
      Concludes saving for the file and cleans up the stack.
      This is currently used by non-tiled saving only - tiled
      saving uses ad-hoc save filters.
    */

    void concludeSave();

    /*!
      Aborts saving, immediately freeing resources.
    */

    void abortSave();

    /*!
      Frees resources related to saving.
    */

    void cleanupAfterSave();

    /*!
      Returns pointer to the save command, to be used by thread
      manager.
     */

    QuillUndoCommand *saveCommand();

    /*!
      Returns pointer to the load command, creates it on demand
      if it does not exist.
    */

    QuillUndoCommand *loadCommand();

    /*!
      Returns pointer to the save map, to be used by thread
      manager.
     */

    SaveMap *saveMap();
    /*!
      Sets the index of the stack command that is currently used before reverting.
    */
    void setRevertIndex(int index);

    /*!
      Gets the stack command id before it is reverted.
     */
    int revertIndex() const;
    /*!
      Reverts the editing operations to the beginning
     */
    void revert();

    /*!
      Restores the editing operations to the state before reverting
     */
    void restore();

    /*!
      Checks if the operations are reverted.
     */
    bool canRevert() const;

    /*!
      Checks if the operations are restored
     */
    bool canRestore() const;

    void clear();
private:

    /*!
      Updates the initial load filter to load from a correct file.
     */

    void setInitialLoadFilter(QuillImageFilter *filter);

private:
    QUndoStack *m_stack;
    File *m_file;
    bool m_isSessionRecording;
    int m_recordingSessionId, m_nextSessionId;
    int m_savedIndex;
    QuillUndoCommand *m_saveCommand;
    QuillUndoCommand *m_loadCommand;
    SaveMap *m_saveMap;
    int m_revertIndex;
};

#endif // __QUILL_UNDO_STACK_H__
