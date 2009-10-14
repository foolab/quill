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

class QuillUndoStackPrivate
{
public:
    Core *core;
    QUndoStack *stack;
    QuillFile *file;
    int sessionId, nextSessionId;
    int savedIndex;
    QuillUndoCommand *saveCommand;
    SaveMap *saveMap;
};

QuillUndoStack::QuillUndoStack(Core *parent, QuillFile *file)
{
    priv = new QuillUndoStackPrivate;
    priv->core = parent;
    priv->file = file;
    priv->stack = new QUndoStack();
    priv->sessionId = 0;
    priv->nextSessionId = 1;
    priv->savedIndex = 0;
    priv->saveCommand = 0;
    priv->saveMap = 0;
}

QuillUndoStack::~QuillUndoStack()
{
    delete priv->stack;
    delete priv->saveCommand;
    delete priv;
}

QuillFile* QuillUndoStack::file()
{
    return priv->file;
}

void QuillUndoStack::load()
{
    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("Load");
    QFile loadFile(priv->file->originalFileName());
    if (loadFile.exists() && (loadFile.size() > 0))
        filter->setOption(QuillImageFilter::FileName,
                          priv->file->originalFileName());
    else
        filter->setOption(QuillImageFilter::FileName,
                          priv->file->fileName());
    add(filter);
}

void QuillUndoStack::add(QuillImageFilter *filter)
{
    qDebug() << "Command" << filter->name() << "added to stack.";

    QuillUndoCommand *cmd = new QuillUndoCommand(filter);

    cmd->setStack(this);
    cmd->setIndex(index());
    cmd->setSessionId(priv->sessionId);
    cmd->setCore(priv->core);

    // tile map
    if (priv->core->defaultTileSize() != QSize())
    {
        TileMap *tileMap;
        if (filter->name() == "Load")
            tileMap = new TileMap(filter->newFullImageSize(QSize()),
                                  priv->core->defaultTileSize(),priv->core->tileCache());
        else
            tileMap = new TileMap(command()->tileMap(), filter);

        cmd->setTileMap(tileMap);
    }

    // add to stack
    priv->stack->push(cmd);
}

bool QuillUndoStack::canUndo() const
{
    if (!priv->stack->canUndo())
        return false;

    // The initial load cannot be undone
    if (index() <= 1)
        return false;

    // Session mode: cannot undo outside session
    if ((priv->sessionId > 0) && (command()->sessionId() != priv->sessionId))
        return false;

    return true;
}

void QuillUndoStack::undo()
{
    if (canUndo()) {
        // In case of an intermediate load, we make a double undo
        if ((command()->filter()->name() == "Load") &&
            (priv->stack->index() > 2))
            priv->stack->undo();

        int sessionId = command()->sessionId();

        // If we are not currently recording a session, an entire
        // session should be undone at once.
        do
            priv->stack->undo();
        while ((priv->sessionId == 0) && (sessionId != 0) &&
               (command()->sessionId() == sessionId));
    }
}

bool QuillUndoStack::canRedo() const
{
    if (!priv->stack->canRedo())
        return false;

    // Session mode: cannot redo outside session
    if ((priv->sessionId > 0) && (command(index())) &&
        (command(index())->sessionId() != priv->sessionId))
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
            priv->stack->redo();
        while ((priv->sessionId == 0) && (sessionId != 0) && command(index()) &&
               (command(index())->sessionId() == sessionId));

        // In case of intermediate load, double redo
        if ((command(index()) && (command(index())->filter()->name() == "Load")))
            priv->stack->redo();
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
    return priv->stack->count();
}

int QuillUndoStack::index() const
{
    return priv->stack->index();
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

    return (QuillUndoCommand*) priv->stack->command(index);
}

QuillUndoCommand *QuillUndoStack::find(int id) const
{
    for (int index=0; index<priv->stack->count(); index++)
    {
        QuillUndoCommand *cmd = command(index);
        if (cmd && (cmd->uniqueId() == id))
            return cmd;
    }

    if ((priv->saveCommand && priv->saveCommand->uniqueId() == id))
        return priv->saveCommand;

    return 0;
}

bool QuillUndoStack::isClean() const
{
    return priv->stack->isClean();
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
    priv->savedIndex = index;
}

int QuillUndoStack::savedIndex() const
{
    return priv->savedIndex;
}

bool QuillUndoStack::isDirty() const
{
    return (command()->index() != savedIndex());
}

void QuillUndoStack::startSession()
{
    if (priv->sessionId <= 0) {
        priv->sessionId = priv->nextSessionId;
        priv->nextSessionId++;
    }
}

void QuillUndoStack::endSession()
{
    priv->sessionId = 0;
}

bool QuillUndoStack::isSession() const
{
    return (priv->sessionId > 0);
}

void QuillUndoStack::prepareSave(const QString &fileName)
{
    priv->sessionId = 0;

    // Should never happen.
    if (priv->saveCommand)
        delete priv->saveCommand;

    if (priv->saveMap)
        delete priv->saveMap;

    if (priv->core->defaultTileSize() != QSize())
        priv->saveMap = new SaveMap(command()->fullImageSize(),
                                    priv->core->saveBufferSize(),
                                    command()->tileMap());

    QuillImageFilter *saveFilter =
        QuillImageFilterFactory::createImageFilter("Save");

    saveFilter->setOption(QuillImageFilter::FileName,
                          QVariant(fileName));

    saveFilter->setOption(QuillImageFilter::FileFormat,
                          QVariant(priv->file->targetFormat()));

    priv->saveCommand = new QuillUndoCommand(saveFilter);
    priv->saveCommand->setStack(this);

    if (priv->core->defaultTileSize() != QSize())
    {
        saveFilter->setOption(QuillImageFilter::TileCount,
                              QVariant(priv->saveMap->bufferCount()));
        priv->saveCommand->setTileMap(new TileMap(command()->tileMap(),
                                                  saveFilter));
    }
}

void QuillUndoStack::concludeSave()
{
    priv->savedIndex = command()->index();

    if (priv->core->defaultTileSize() != QSize())
    {
        delete priv->saveCommand;
        priv->saveCommand = 0;

        delete priv->saveMap;
        priv->saveMap = 0;
    }
}

QuillUndoCommand *QuillUndoStack::saveCommand()
{
    return priv->saveCommand;
}

SaveMap *QuillUndoStack::saveMap()
{
    return priv->saveMap;
}
