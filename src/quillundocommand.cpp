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

#include <QImage>
#include <QDebug>
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

QuillUndoCommand::QuillUndoCommand(QuillImageFilter *filter) :
    QUndoCommand(), m_filter(filter), m_stack(0), m_core(0), m_index(0),
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

    if (m_stack && (m_filter->name() == "Load"))
        m_stack->setSavedIndex(-1);

    // If the background thread is currently running the filter,
    // do not delete it (it would crash the background thread).
    // Instead, ThreadManager::calculationFinished() will handle this
    // after the calculation has finished.

    if (!m_core || (m_core->allowDelete(m_filter)))
        delete m_filter;

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
    // calculate target full image size

    QuillImage previous;
    QSize previousFullSize, fullSize;

    if (m_stack->index() == 0) {
        // default preview image size defined here

        previous = QuillImage(QImage(m_core->previewSize(0),
                                     QuillImage::Format_RGB32));
        previousFullSize = m_filter->newFullImageSize(previousFullSize);
    }
    else {
        previous = prev()->image(0);
        previousFullSize = prev()->fullImageSize();
    }

    if (previousFullSize.isEmpty())
        m_stack->file()->setError(Quill::ErrorFormatUnsupported);

    previous.setFullImageSize(previousFullSize);
    previous.setArea(QRect(QPoint(0, 0), previousFullSize));

    // Update full image size
    m_fullImageSize = m_filter->newFullImageSize(previousFullSize);
}

void QuillUndoCommand::undo()
{
    // no need to do anything here
}

void QuillUndoCommand::setStack(QuillUndoStack *stack)
{
    m_stack = stack;
}

void QuillUndoCommand::setCore(Core *core)
{
    m_core = core;
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

QuillUndoCommand *QuillUndoCommand::prev() const
{
    if (!m_stack || !m_index)
        return 0;

    return m_stack->command(m_index-1);
}

QuillImage QuillUndoCommand::image(int level) const
{
    return m_core->cache(level)->image(m_id);
}

void QuillUndoCommand::setImage(int level, const QuillImage &image)
{
    m_core->cache(level)->insertImage(m_id, image, ImageCache::ProtectLast);
}

QuillImage QuillUndoCommand::fullImage() const
{
    return image(m_core->previewLevelCount());
}

int QuillUndoCommand::bestImageLevel() const
{
    for (int i=m_core->previewLevelCount(); i>=0; i--)
        if (!image(i).isNull())
            return i;
    return -1;
}

QuillImage QuillUndoCommand::bestImage() const
{
    int level = bestImageLevel();

    if (level == -1)
        return QuillImage();
    else
        return image(level);
}

QList<QuillImage> QuillUndoCommand::allImageLevels(int maxLevel) const
{
    if (maxLevel > m_core->previewLevelCount())
        maxLevel = m_core->previewLevelCount();
    QList<QuillImage> list;
    for (int i=0; i<=maxLevel; i++)
        if (!image(i).isNull())
            list.append(image(i));

    return list;
}

QSize QuillUndoCommand::fullImageSize() const
{
    return m_fullImageSize;
}

QSize QuillUndoCommand::targetPreviewSize(int level) const
{
    QSize previewSize = m_core->previewSize(level);
    QSize targetSize;

    // keep aspect ratio, always round fractions up
    int targetWidth = (previewSize.height() * m_fullImageSize.width()
        + m_fullImageSize.height() - 1) / m_fullImageSize.height();

    if (targetWidth <= previewSize.width())
        targetSize = QSize(targetWidth, previewSize.height());
    else {
        // keep aspect ratio, always round fractions up
        int targetHeight = (previewSize.width() * m_fullImageSize.height()
        + m_fullImageSize.width() - 1) / m_fullImageSize.width();
        targetSize = QSize(previewSize.width(), targetHeight);
    }

    return targetSize.boundedTo(m_fullImageSize);
}

void QuillUndoCommand::setSessionId(int id)
{
    m_sessionId = id;
}

int QuillUndoCommand::sessionId() const
{
    return m_sessionId;
}

void QuillUndoCommand::setTileMap(TileMap *map)
{
    delete m_tileMap;

    m_tileMap = map;
}

TileMap *QuillUndoCommand::tileMap() const
{
    return m_tileMap;
}
