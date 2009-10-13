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

#ifndef QUILL_UNDO_COMMAND
#define QUILL_UNDO_COMMAND

#include <QUndoCommand>
#include <QuillImageFilter>

class QuillUndoStack;
class QString;
class QuillImage;
class Core;
class Quill;
class TileMap;

class QuillUndoCommandPrivate;

class QuillUndoCommand : public QUndoCommand
{

public:
    QuillUndoCommand(QuillImageFilter *filter);
    ~QuillUndoCommand();

    QuillImageFilter *filter() const;

    /*!
      Used for on-the-fly updating of the filter.
     */

    void setFilter(QuillImageFilter *filter);

    /*!
      Called from QUndoStack::push() and redo().
    */

    void redo();

    /*!
      Called from QUndoStack::undo().
    */

    void undo();

    /*!
      To be used by QuillUndoStack only!
     */

    QuillUndoStack *stack() const;
    void setStack(QuillUndoStack *stack);
    void setCore(Core *core);

    int index() const;
    void setIndex(int index);

    /*!
      A guaranteedly unique identifier for this command.
     */

    int uniqueId() const;

    /*!
      Get the previous command in stack.
      Returns null if command has not been added to a stack or
      if it is the first one in the stack.
    */

    QuillUndoCommand *prev() const;

    /*!
      The preview-size result image of the command
     */

    QuillImage image(int level) const;

    /*!
      Internal use only
     */

    void setImage(int level, const QuillImage &image);

    /*!
      The full-size result image of the command
     */
    QuillImage fullImage() const;

    /*!
      The full image size - possibly pre-calculated
     */
    QSize fullImageSize() const;

    /*!
      Gets the resolution level of the best image available in the command.
     */

    int bestImageLevel() const;

    /*!
      Gets the best image available in the command.
    */

    QuillImage bestImage() const;

    /*!
      Gets all image levels available in the command.
    */

    QList<QuillImage> allImageLevels(int maxLevel) const;

    /*!
      Gets the target preview size for the level, with a correct
      aspect ratio.
     */

    QSize targetPreviewSize(int level) const;

    /*!
      Sets the session id for the command. Commands with the same
      session id will undo and redo together.
     */

    void setSessionId(int id);

    /*!
      Gets the session id for the command. Commands with the same
      session id will undo and redo together.
     */

    int sessionId() const;

    /*!
      Sets the tile map for the command.
      The tile map becomes property of the command.
    */

    void setTileMap(TileMap *map);

    /*!
      Gets the tile map for the command. Modifications to the tile
      map will be reflected to the command.
     */

    TileMap *tileMap() const;

private:
    int m_id;
    QuillImageFilter* m_filter;
    QuillUndoStack* m_stack;
    Core *m_core;

    /*!
      Position of the command in the stack.
     */

    int m_index;

    /*!
      Session id (see QuillUndoStack::startSession())
     */

    int m_sessionId;


    /*!
      The size of the full image after this operation.
     */

    QSize m_fullImageSize;

    /*!
      The tile map for this command.
     */

    TileMap *m_tileMap;

    /*!
      Guarantees the uniqueness of a command in a multi-threaded system.
      0 is used to denote an invalid Id.
    */
    static int m_nextId;
};

#endif