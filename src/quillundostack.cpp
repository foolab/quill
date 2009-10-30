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

#include <QUndoStack>
#include <QuillImage>
#include <QFileInfo>
#include <QBuffer>
#include <QDataStream>
#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include <QuillImageFilterGenerator>
#include <QDebug>

#include "quillundostack.h"
#include "quillundocommand.h"
#include "quillfile.h"
#include "core.h"
#include "tilemap.h"
#include "savemap.h"

QuillUndoStack::QuillUndoStack(Core *parent, QuillFile *file) :
    m_core(parent), m_stack(new QUndoStack()), m_file(file),
    m_sessionId(0), m_nextSessionId(1), m_savedIndex(0),
    m_saveCommand(0), m_saveMap(0)
{
}

QuillUndoStack::~QuillUndoStack()
{
    delete m_stack;
    delete m_saveCommand;
    delete m_saveMap;
}

QuillFile* QuillUndoStack::file()
{
    return m_file;
}

void QuillUndoStack::load()
{
    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("Load");
    QFile loadFile(m_file->originalFileName());
    if (loadFile.exists() && (loadFile.size() > 0))
        filter->setOption(QuillImageFilter::FileName,
                          m_file->originalFileName());
    else
        filter->setOption(QuillImageFilter::FileName,
                          m_file->fileName());
    add(filter);
}

void QuillUndoStack::add(QuillImageFilter *filter)
{
    qDebug() << "Command" << filter->name() << "added to stack.";

    QuillUndoCommand *cmd = new QuillUndoCommand(this, m_core);

    cmd->setFilter(filter);
    cmd->setIndex(index());
    if (cmd->filter()->name() != "Load")
        cmd->setSessionId(m_sessionId);

    // full image size

    QSize previousFullSize;

    if (index() == 0)
        previousFullSize = QSize();
    else
        previousFullSize = command()->fullImageSize();

    QSize fullSize = filter->newFullImageSize(previousFullSize);

    if (fullSize.isEmpty())
        m_file->setError(Quill::ErrorFormatUnsupported);

    cmd->setFullImageSize(fullSize);

    // tile map
    if (!m_core->defaultTileSize().isEmpty()) {
        TileMap *tileMap;
        if (filter->name() == "Load")
            tileMap = new TileMap(filter->newFullImageSize(QSize()),
                                  m_core->defaultTileSize(),m_core->tileCache());
        else
            tileMap = new TileMap(command()->tileMap(), filter);

        cmd->setTileMap(tileMap);
    }

    // add to stack
    m_stack->push(cmd);
}

bool QuillUndoStack::canUndo() const
{
    if (!m_stack->canUndo())
        return false;

    // The initial load cannot be undone
    if (index() <= 1)
        return false;

    // Session mode: cannot undo outside session
    if ((m_sessionId > 0) && (command()->sessionId() != m_sessionId))
        return false;

    return true;
}

void QuillUndoStack::undo()
{
    if (canUndo()) {
        // In case of an intermediate load, we make a double undo
        if ((command()->filter()->name() == "Load") && (m_stack->index() > 2))
            m_stack->undo();

        int referenceSessionId = command()->sessionId();

        // If we are not currently recording a session, an entire
        // session should be undone at once.
        if ((m_sessionId != 0) || (referenceSessionId == 0))
            m_stack->undo();
        else
            do
                m_stack->undo();
            while (command()->sessionId() == referenceSessionId);

        // If we have any stored images in cache, move them to protected
        command()->protectImages();
    }
}

bool QuillUndoStack::canRedo() const
{
    if (!m_stack->canRedo())
        return false;

    // Session mode: cannot redo outside session
    if ((m_sessionId > 0) && (command(index())) &&
        (command(index())->sessionId() != m_sessionId))
        return false;

    return true;
}

void QuillUndoStack::redo()
{
    if (canRedo()) {
        int sessionId = command(index())->sessionId();

        // If we are not currently recording a session, an entire
        // session should be redone at once.
        do
            m_stack->redo();
        while ((m_sessionId == 0) && (sessionId != 0) && command(index()) &&
               (command(index())->sessionId() == sessionId));

        // In case of intermediate load, double redo
        if ((command(index()) && (command(index())->filter()->name() == "Load")))
            m_stack->redo();

        // If we have any stored images in cache, move them to protected
        command()->protectImages();
    }
}

QuillImage QuillUndoStack::image() const
{
    QuillUndoCommand *curr = command();
    if (curr)
        return curr->bestImage();
    else
        return QuillImage();
}

QuillImage QuillUndoStack::image(int level) const
{
    QuillUndoCommand *curr = command();
    if (curr)
        return curr->image(level);
    else
        return QuillImage();
}

QList<QuillImage> QuillUndoStack::allImageLevels(int maxLevel) const
{
    QuillUndoCommand *curr = command();
    if (curr)
        return curr->allImageLevels(maxLevel);
    else
        return QList<QuillImage>();
}

int QuillUndoStack::count() const
{
    return m_stack->count();
}

int QuillUndoStack::index() const
{
    return m_stack->index();
}

QuillUndoCommand *QuillUndoStack::command() const
{
    if (index() > 0)
        return command(index()-1);
    else
        return 0;
}

QuillUndoCommand *QuillUndoStack::command(int index) const
{
    // This can be done since the stack will only hold QuillUndoCommands.

    return (QuillUndoCommand*) m_stack->command(index);
}

QuillUndoCommand *QuillUndoStack::find(int id) const
{
    for (int index=0; index<m_stack->count(); index++) {
        QuillUndoCommand *cmd = command(index);
        if (cmd && (cmd->uniqueId() == id))
            return cmd;
    }

    if ((m_saveCommand && m_saveCommand->uniqueId() == id))
        return m_saveCommand;

    return 0;
}

bool QuillUndoStack::isClean() const
{
    return m_stack->isClean();
}

QSize QuillUndoStack::fullImageSize() const
{
    if (command())
        return command()->fullImageSize();
    else
        return QSize();
}

void QuillUndoStack::setSavedIndex(int index)
{
    m_savedIndex = index;
}

int QuillUndoStack::savedIndex() const
{
    return m_savedIndex;
}

bool QuillUndoStack::isDirty() const
{
    return (command()->index() != savedIndex());
}

void QuillUndoStack::startSession()
{
    if (m_sessionId <= 0) {
        m_sessionId = m_nextSessionId;
        m_nextSessionId++;
    }
}

void QuillUndoStack::endSession()
{
    m_sessionId = 0;
}

bool QuillUndoStack::isSession() const
{
    return (m_sessionId > 0);
}

void QuillUndoStack::prepareSave(const QString &fileName)
{
    m_sessionId = 0;

    delete m_saveCommand;
    delete m_saveMap;

    if (!m_core->defaultTileSize().isEmpty())
        m_saveMap = new SaveMap(command()->fullImageSize(),
                                    m_core->saveBufferSize(),
                                    command()->tileMap());

    QuillImageFilter *saveFilter =
        QuillImageFilterFactory::createImageFilter("Save");

    saveFilter->setOption(QuillImageFilter::FileName,
                          QVariant(fileName));

    saveFilter->setOption(QuillImageFilter::FileFormat,
                          QVariant(m_file->targetFormat()));

    m_saveCommand = new QuillUndoCommand(this, m_core);
    m_saveCommand->setFilter(saveFilter);

    if (!m_core->defaultTileSize().isEmpty()) {
        saveFilter->setOption(QuillImageFilter::TileCount,
                              QVariant(m_saveMap->bufferCount()));
        m_saveCommand->setTileMap(new TileMap(command()->tileMap(),
                                                  saveFilter));
    }
}

void QuillUndoStack::concludeSave()
{
    m_savedIndex = command()->index();

    if (!m_core->defaultTileSize().isEmpty()) {
        delete m_saveCommand;
        m_saveCommand = 0;

        delete m_saveMap;
        m_saveMap = 0;
    }
}

QuillUndoCommand *QuillUndoStack::saveCommand()
{
    return m_saveCommand;
}

SaveMap *QuillUndoStack::saveMap()
{
    return m_saveMap;
}
