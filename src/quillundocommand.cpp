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

#include <QImage>
#include <QuillImage>
#include <QuillImageFilter>

#include "quill.h"
#include "quillfile.h"
#include "quillundostack.h"
#include "quillundocommand.h"
#include "imagecache.h"
#include "tilemap.h"
#include "core.h"

int QuillUndoCommand::m_nextId = 1;

QuillUndoCommand::QuillUndoCommand(QuillUndoStack *parent) :
    QUndoCommand(), m_filter(0), m_stack(parent),
    m_index(0), m_belongsToSession(0), m_sessionId(0),
    m_fullImageSize(QSize()), m_tileMap(0)
{
    // Guarantees that the id will always be unique (at least to maxint)
    m_id = m_nextId;
    m_nextId++;
}

QuillUndoCommand::~QuillUndoCommand()
{
    // Deleting a command with the intermediate load filter
    // forces save when closing

    if (m_stack && (m_index > 0) && (m_index == m_stack->savedIndex()))
        m_stack->setSavedIndex(-1);

    // If the background thread is currently running the filter,
    // do not delete it (it would crash the background thread).
    // Instead, Scheduler::processFinishedTask() will handle this
    // after the calculation has finished.

    if (m_filter && Core::instance()->allowDelete(m_filter))
        delete m_filter;

    // Eject any images from the cache

    if (m_stack)
        for (int level=0; level<Core::instance()->previewLevelCount(); level++)
            Core::instance()->cache(level)->remove(m_stack->file(), m_id);

    // The tile map becomes property of the command

    delete m_tileMap;
}

QuillImageFilter* QuillUndoCommand::filter() const
{
    return m_filter;
}

void QuillUndoCommand::setFilter(QuillImageFilter* filter)
{
    m_filter = filter;
}

void QuillUndoCommand::redo()
{
    // no need to do anything here - see QuillUndoStack::redo()
}

void QuillUndoCommand::undo()
{
    // no need to do anything here - see QuillUndoStack::undo()
}

QuillUndoStack *QuillUndoCommand::stack() const
{
    return m_stack;
}

void QuillUndoCommand::setIndex(int index)
{
    m_index = index;
}

int QuillUndoCommand::index() const
{
    return m_index;
}

int QuillUndoCommand::uniqueId() const
{
    return m_id;
}

void QuillUndoCommand::setUniqueId(int id)
{
    m_id = id;
}

QuillUndoCommand *QuillUndoCommand::prev() const
{
    if (!m_stack || !m_index)
        return 0;

    return m_stack->command(m_index-1);
}

bool QuillUndoCommand::hasImage(int level) const
{
    return Core::instance()->cache(level)->hasImage(m_stack->file(), m_id);
}

QuillImage QuillUndoCommand::image(int level) const
{
    return Core::instance()->cache(level)->image(m_stack->file(), m_id);
}

void QuillUndoCommand::setImage(int level, const QuillImage &image)
{
    ImageCache *cache = Core::instance()->cache(level);
    if (!cache)
        return;

    ImageCache::ProtectionStatus status = ImageCache::NotProtected;

    // An image will be protected if it is:
    // 1) not after the current state (i.e. not in redo history)
    // 2) closer to the current state than the currently protected image
    // 3) or, the currently protected image is after the current state

    QuillUndoCommand *previousProtected =
        m_stack->find(cache->protectedId(m_stack->file()));

    if ((m_index < m_stack->index()) &&
        (!previousProtected ||
         (m_index >= previousProtected->index()) ||
         (previousProtected->index() >= m_stack->index())))
        status = ImageCache::Protected;

    cache->insert(m_stack->file(), m_id, image, status);
}

void QuillUndoCommand::protectImages()
{
    for (int level=0; level<=Core::instance()->previewLevelCount(); level++)
        Core::instance()->cache(level)->protect(m_stack->file(), m_id);
}

QuillImage QuillUndoCommand::fullImage() const
{
    return image(Core::instance()->previewLevelCount());
}

int QuillUndoCommand::bestImageLevel(int maxLevel) const
{
    for (int i=maxLevel; i>=0; i--)
        if (Core::instance()->isSubstituteLevel(i, maxLevel) &&
            !image(i).isNull())
            return i;
    return -1;
}

QuillImage QuillUndoCommand::bestImage(int maxLevel) const
{
    int level = bestImageLevel(maxLevel);

    if (level == -1)
        return QuillImage();
    else
        return image(level);
}

QList<QuillImage> QuillUndoCommand::allImageLevels(int maxLevel) const
{
    if (maxLevel > Core::instance()->previewLevelCount())
        maxLevel = Core::instance()->previewLevelCount();
    QList<QuillImage> list;
    for (int i=0; i<=maxLevel; i++)
        if (Core::instance()->isSubstituteLevel(i, maxLevel) &&
            !image(i).isNull())
            list.append(image(i));

    return list;
}

void QuillUndoCommand::setFullImageSize(const QSize &size)
{
    m_fullImageSize = size;
    //We need set the full image size to thumbnails
    int displayLevel = Core::instance()->previewLevelCount();
    for(int i=0;i<=displayLevel;i++){
        QuillImage imageCopy(image(i));
        if(!imageCopy.isNull()){
            imageCopy.setFullImageSize(m_fullImageSize);
            QSize targetSize = Core::instance()->targetSizeForLevel(i, m_fullImageSize);
            imageCopy.setArea(Core::instance()->targetAreaForLevel(i, targetSize, m_fullImageSize));
            setImage(i,imageCopy);
        }
    }
}

QSize QuillUndoCommand::fullImageSize() const
{
    return m_fullImageSize;
}

void QuillUndoCommand::setSessionId(int id)
{
    m_belongsToSession = true;
    m_sessionId = id;
}

int QuillUndoCommand::sessionId() const
{
    return m_sessionId;
}

bool QuillUndoCommand::belongsToSession() const
{
    return m_belongsToSession;
}

bool QuillUndoCommand::belongsToSession(int id) const
{
    return (m_belongsToSession && m_sessionId == id);
}

void QuillUndoCommand::createTileMap()
{
    if (m_tileMap || !m_filter)
        return;

    if (m_filter->role() == QuillImageFilter::Role_Load)
        m_tileMap = new TileMap(m_fullImageSize,
                                Core::instance()->defaultTileSize(),
                                Core::instance()->tileCache());
    else {
        if (!prev()->tileMap())
            prev()->createTileMap();
        m_tileMap = new TileMap(prev()->tileMap(), m_filter);
    }
}

void QuillUndoCommand::setTileMap(TileMap *map)
{
    delete m_tileMap;

    m_tileMap = map;
}

TileMap *QuillUndoCommand::tileMap()
{
    if (!m_tileMap)
        createTileMap();

    return m_tileMap;
}
