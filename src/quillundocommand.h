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
  \class QuillUndoCommand

  \brief Represents one image editing operation in the edit history.

QuillUndoCommand extends QUndoCommand by containing cache references
to preview and full images (in ImageCache, accessible via image()) and
tiles (in TileCache, accessible via tileMap()).
 */

#ifndef QUILL_UNDO_COMMAND
#define QUILL_UNDO_COMMAND

#include <QUndoCommand>
#include <QuillImageFilter>

class QuillUndoStack;
class QString;
class QuillImage;
class Quill;
class TileMap;

class QuillUndoCommandPrivate;

class QuillUndoCommand : public QUndoCommand
{

public:
    QuillUndoCommand(QuillUndoStack *parent);
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
      Returns the stack of the command.
     */

    QuillUndoStack *stack() const;

    int index() const;
    void setIndex(int index);

    /*!
      A guaranteedly unique identifier for this command.
     */

    int uniqueId() const;

    /*!
      Redefines the unique identifier for this command. Usable if the
      original command has been destroyed and any possible ongoing
      progress (directed by the command id) by the background thread
      needs to be kept. Use in any other case will probably break everything.
     */

    void setUniqueId(int id);

    /*!
      Get the previous command in stack.
      Returns null if command has not been added to a stack or
      if it is the first one in the stack.
    */

    QuillUndoCommand *prev() const;

    /*!
      Only querying for the existence of an image, not interested
      in the image itself
     */

    bool hasImage(int level) const;

    /*!
      The preview-size result image of the command
     */

    QuillImage image(int level) const;

    /*!
      Internal use only
     */

    void setImage(int level, const QuillImage &image);

    /*!
      Protects all images. Useful when this command is set to current.
     */

    void protectImages();

    /*!
      The full-size result image of the command
     */
    QuillImage fullImage() const;

    /*!
      Setting the full image size of the command
     */
    void setFullImageSize(const QSize &size);

    /*!
      The full image size - possibly pre-calculated
     */
    QSize fullImageSize() const;

    /*!
      Gets the resolution level of the best image available in the command.
     */

    int bestImageLevel(int maxLevel) const;

    /*!
      Gets the best image available in the command.
    */

    QuillImage bestImage(int maxLevel) const;

    /*!
      Gets all image levels available in the command.
    */

    QList<QuillImage> allImageLevels(int maxLevel) const;

    /*!
      Makes the command part of a session, and sets the session id
      for the command. Commands with the same session id will undo and
      redo together.
     */

    void setSessionId(int id);

    /*!
      Gets the session id for the command. Commands with the same
      session id will undo and redo together. If the command is not
      part of any session, returns 0; see also belongsToSession().
     */

    int sessionId() const;

    /*!
      Returns true if the command is part of any session.
     */

    bool belongsToSession() const;

    /*!
      Returns true if the command is part of the given session.
     */

    bool belongsToSession(int id) const;

    /*!
      Automatically creates a new tile map for the command.
    */

    void createTileMap();

    /*!
      Sets the tile map for the command.
      The tile map becomes property of the command.
    */

    void setTileMap(TileMap *map);

    /*!
      Gets the tile map for the command. Modifications to the tile
      map will be reflected to the command.
     */

    TileMap *tileMap();

private:
    int m_id;
    QuillImageFilter* m_filter;
    QuillUndoStack* m_stack;

    /*!
      Position of the command in the stack.
     */

    int m_index;

    int m_belongsToSession;

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
