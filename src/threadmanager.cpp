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

#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <QSemaphore>
#include <QEventLoop>
#include <QDir>

#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include <QuillImageFilterGenerator>

#include "file.h"
#include "core.h"
#include "quillundocommand.h"
#include "quillundostack.h"
#include "threadmanager.h"
#include "tilemap.h"
#include "savemap.h"

ThreadManager::ThreadManager(Core *core, Quill::ThreadingMode mode) :
    core(core), commandId(0), commandLevel(0), tileId(0),
    activeFilter(0), resultImage(0),
    watcher(new QFutureWatcher<QuillImage>),
    threadingMode(mode),
    semaphore(0), eventLoop(0), debugDelay(0)
{
    if (mode == Quill::ThreadingTest)
    {
        semaphore = new QSemaphore();
        eventLoop = new QEventLoop();
    }

    connect(watcher,
            SIGNAL(finished()),
            SLOT(calculationFinished()));
}

ThreadManager::~ThreadManager()
{
    delete watcher;
    delete resultImage;
    delete semaphore;
    delete eventLoop;
}

bool ThreadManager::isRunning() const
{
    return (commandId != 0);
}

QuillUndoCommand *ThreadManager::getTask(QuillUndoStack *stack, int level) const
{
    int index;

    for (index=stack->index()-1; index>=0; index--)
    {
        if (!stack->command(index)->image(level).isNull())
            break;

        // Load filters can be re-executed
        if (stack->command(index)->filter()->role() == QuillImageFilter::Role_Load)
        {
            index--;
            break;
        }
    }

    // ...and start working with the next one
    return stack->command(index + 1);
}

bool ThreadManager::suggestTilingTask(File *file)
{
    QuillUndoStack *stack = file->stack();
    int tileIndex;

    if (stack->saveCommand())
    {
        // If we can run the actual save filter
        if (stack->saveMap()->isBufferComplete())
            return false;

        // If we already have tiles to push to the save buffer
        if (stack->saveMap()->processNext(stack->command()->tileMap()) != -1)
            return false;
        else
            // Ask save map about which tile to fetch.
            tileIndex = stack->saveMap()->prioritize();
    }
    else
    {
        // We already have the maximum number of relevant tiles.
        // Do not calculate any new ones to avoid an infinite loop.
        // Currently, this may prioritize some of the less relevant
        // tiles in favor of more relevant ones.

        if (stack->command()->tileMap()->
            nonEmptyTiles(file->viewPort()).count() >=
            stack->command()->tileMap()->cacheCost())
            return false;

        tileIndex = stack->command()->tileMap()->
            prioritize(file->viewPort());
    }

    // We have all the tiles we want already
    if (tileIndex == -1)
        return false;

    int index;

    for (index=stack->index()-1; index>=0; index--)
    {
        if (stack->command(index)->tileMap()->tile(tileIndex) != QuillImage())
            break;

        // Load filters can be re-executed
        if (stack->command(index)->filter()->role() == QuillImageFilter::Role_Load)
        {
            index--;
            break;
        }
    }

    // ...and start working with the next one
    QuillUndoCommand *command = stack->command(index + 1);
    QuillImage prevImage;

    if (command->filter()->role() == QuillImageFilter::Role_Load)
        prevImage = command->tileMap()->tile(tileIndex);
    else
        prevImage = command->prev()->tileMap()->tile(tileIndex);

    startThread(command->uniqueId(),
                core->previewLevelCount(),
                tileIndex,
                prevImage,
                command->filter());

    return true;
}

bool ThreadManager::suggestTilingSaveTask(File *file)
{
    QuillUndoStack *stack = file->stack();
    if (stack->saveMap()->isBufferComplete())
    {
        startThread(stack->saveCommand()->uniqueId(),
                    core->previewLevelCount(), 0,
                    stack->saveMap()->buffer(),
                    stack->saveCommand()->filter());
        return true;
    }
    else
        return suggestTilingOverlayTask(file);
}

bool ThreadManager::suggestTilingOverlayTask(File *file)
{
    QuillUndoStack *stack = file->stack();
    int tileId = stack->saveMap()->processNext(stack->command()->tileMap());

    if(tileId < 0)
        return false;
    // Should never happen.
    if (stack->command()->tileMap()->tile(tileId) == QuillImage())
        return false;

    QuillImageFilter *filter = stack->saveMap()->addToBuffer(tileId);

    QuillImage prevImage = stack->command()->tileMap()->tile(tileId);

    startThread(stack->saveCommand()->uniqueId(),
                core->previewLevelCount(),
                tileId,
                prevImage,
                filter);

    return true;
}

bool ThreadManager::suggestThumbnailLoadTask(File *file,
                                             int level)
{
    if (!file->stack())
        return false;

    if (file->stack()->image(level) != QuillImage())
        // Image already exists - no need to recalculate
        return false;

    QuillUndoCommand *command = getTask(file->stack(), level);

    if ((command->filter()->role() != QuillImageFilter::Role_Load) ||
        (command->index() != command->stack()->savedIndex()) ||
        (!file->hasThumbnail(level)))
        return false;

    QuillImageFilter *filter = QuillImageFilterFactory::createImageFilter(QuillImageFilter::Role_Load);

    filter->setOption(QuillImageFilter::FileName,
                      file->thumbnailFileName(level));

    startThread(command->uniqueId(),
                level, 0,
                QuillImage(),
                filter);

    return true;
}

bool ThreadManager::suggestThumbnailSaveTask(File *file, int level)
{
    QuillUndoStack *stack = file->stack();

    if ((!stack) || (!stack->command()))
        return false;

    if ((core->thumbnailDirectory(level).isEmpty()) ||
        (file->image(level).isNull()) ||
        (file->stack()->isDirty()) ||
        (file->hasThumbnail(level)))
        return false;

    if (file->image(level).size() !=
        file->stack()->command()->targetPreviewSize(level))
        return false;

    QDir().mkpath(core->thumbnailDirectory(level));

    QuillImageFilter *filter = QuillImageFilterFactory::createImageFilter(QuillImageFilter::Role_Save);

    filter->setOption(QuillImageFilter::FileName,
                      file->thumbnailFileName(level));

    startThread(file->stack()->command()->uniqueId(),
                level, 0,
                file->image(level),
                filter);

    return true;
}

bool ThreadManager::suggestNewTask(File *file, int level)
{
    QuillUndoStack *stack = file->stack();

    if (!stack->command())
        // Empty stack command - should never happen
        return false;

    if (stack->image(level) != QuillImage())
        // Image already exists - no need to recalculate
        return false;

    if ((level == core->previewLevelCount()) &&
        (!core->defaultTileSize().isEmpty()))
        return suggestTilingTask(file);

    // The given resolution level is missing

    QuillUndoCommand *command = getTask(stack, level);

    QuillUndoCommand *prev = 0;
    if (command->filter()->role() != QuillImageFilter::Role_Load)
        prev = command->prev();

    QuillImage prevImage;
    if ((prev == 0) && (level == core->previewLevelCount()))
        prevImage = QuillImage();
    else if (prev == 0)
    {
        prevImage = QuillImage(QImage(command->targetPreviewSize(level),
                                      QImage::Format_RGB32));
        prevImage.setFullImageSize(command->fullImageSize());
        prevImage.setArea(QRect(QPoint(0, 0), command->fullImageSize()));
        prevImage.setZ(level);
    }
    else
    {
        QuillImageFilterGenerator *generator =
            dynamic_cast<QuillImageFilterGenerator*>(command->filter());

        // Red eye detection always uses the best available image
        if (generator && (!generator->isUsedOnPreview()))
            prevImage = prev->bestImage(core->previewLevelCount());
        else
            prevImage = prev->image(level);
    }

    startThread(command->uniqueId(),
                level, 0,
                prevImage,
                command->filter());

    return true;
}

bool ThreadManager::suggestSaveTask(File *file)
{
    QuillUndoStack *stack = file->stack();

    // If the current command has an image which has not been saved before.
    if (!stack->isDirty())
        return false;

    // Tiling save variant
    if (!core->defaultTileSize().isEmpty())
        return suggestTilingSaveTask(file);

    startThread(stack->saveCommand()->uniqueId(),
                core->previewLevelCount(), 0,
                stack->image(core->previewLevelCount()),
                stack->saveCommand()->filter());

    return true;
}

bool ThreadManager::suggestPreviewImprovementTask(File *file)
{
    QuillUndoStack *stack = file->stack();

    // If the current command already has a full image, we see if the
    // previews need re-calculating

    QuillUndoCommand *command = stack->command();

    int level;
    QSize targetSize;

    for (level=0; level<core->previewLevelCount(); level++)
    {
        // Preview images cannot be bigger than full image
        targetSize = command->targetPreviewSize(level);
        if ((!command->image(level).size().isEmpty()) &&
            (command->image(level).size() != targetSize))
            break;
    }

    if (level < core->previewLevelCount())
    {
        QuillImage prevImage = command->fullImage();

        if (prevImage == QuillImage())
        {
            // The case with tiling (base on the largest preview)
            if (level < core->previewLevelCount() - 1)
                prevImage = command->image(core->previewLevelCount() - 1);

            if (prevImage == QuillImage())
                return false;
        }

        // Create ad-hoc filter for preview re-calculation

        QuillImageFilter *scaleFilter =
            QuillImageFilterFactory::createImageFilter(QuillImageFilter::Role_PreviewScale);
        scaleFilter->setOption(QuillImageFilter::SizeAfter,
                               QVariant(targetSize));

        // Treat image as a full image

        startThread(command->uniqueId(),
                    level, 0,
                    QuillImage(QImage(prevImage)),
                    scaleFilter);

        return true;

    }
    // This should never happen
    return false;
}

QuillImage applyFilter(QuillImageFilter *filter, QuillImage image,
                       QSemaphore *semaphore, int debugDelay)
{
    sleep(debugDelay);
    if (semaphore != 0)
        semaphore->acquire();
    return filter->apply(image);
}


void ThreadManager::startThread(int id, int level, int tile,
                                const QuillImage &image,
                                QuillImageFilter *filter)
{
    commandId = id;
    commandLevel = level;
    tileId = tile;
    activeFilter = filter;

    resultImage = new QFuture<QuillImage>;
    *resultImage =
        QtConcurrent::run(applyFilter, filter, image,
                          semaphore, debugDelay);
    watcher->setFuture(*resultImage);
}

void ThreadManager::calculationFinished()
{
    bool raiseFull = false,
        raisePartial = false;

    QuillImage image = resultImage->result();
    delete resultImage;
    resultImage = 0;

    // See if the command is still in the stack.

    QuillUndoCommand *command = core->findInAllStacks(commandId);
    QuillUndoStack *stack = 0;
    if (command != 0)
        stack = command->stack();

    // Set active filter to zero (so that we do not block its deletion)

    QuillImageFilter *filter = activeFilter;
    activeFilter = 0;

    // Mark that we are not currently running a thread

    int previousCommandId = commandId;
    commandId = 0;

    QuillImageFilterGenerator *generator =
        dynamic_cast<QuillImageFilterGenerator*>(filter);

    if (command == 0)
    {
        // The command has been deleted.
        // Do nothing, just delete the filter
        // since it is now orphan and we could not delete it
        // in QuillUndoCommand::~QuillUndoCommand().

        delete filter;
    }
    else if (filter->role() == QuillImageFilter::Role_Overlay)
    {
        // Save buffer overlay has finished - update the buffer.

        delete filter;
        stack->saveMap()->setBuffer(image);
    }
    else if (filter->role() == QuillImageFilter::Role_Save)
    {
        if (!stack->file()->isSaveInProgress()) {
            // Thumbnail saving
            delete filter;

            // Save failed - disabling thumbnailing
            if (image.isNull()) {
                stack->file()->setError(Quill::ErrorThumbnailWriteFailed);
                qDebug() << "Save failed!";
                core->setThumbnailCreationEnabled(false);
            }
        }

        else if (!stack->saveMap() ||
                 stack->saveMap()->isSaveComplete())
        {
            // If full image saving is concluded, rename the file and clean up.

            stack->file()->concludeSave();

            if (stack->file()->allowDelete())
                delete stack->file();
        }
        else
            // Full image saving proceeds
            stack->saveMap()->nextBuffer();
    }
    else if (generator)
    {
        // A detection phase has been passed, replacing the detector
        // with its resulting filter.

        QuillImageFilter *filter = generator->resultingFilter();

        // If null is returned, create a filter which does nothing instead.

        if (filter == 0)
            filter = QuillImageFilterFactory::
                createImageFilter("RedEyeReduction");

        delete generator;
        command->setFilter(filter);
    }
    else if ((commandLevel >= core->previewLevelCount()) &&
             !core->defaultTileSize().isEmpty())
    {
        // A fragment of the full image has been calculated

        command->tileMap()->setTile(tileId, image);
        raisePartial = true;
    }
    else
    {
        // Ad-hoc filters (preview improvement and thumbnail loading)
        if (command->filter() != filter)
        {
            image = QuillImage(image, command->fullImageSize());
            delete filter;
        }

        // Normal case: a better version of an image has been calculated.

        command->setImage(commandLevel, image);
        raiseFull = true;
    }

    // since this might be altered by suggestNewTask
    int previousCommandLevel = commandLevel;
    int previousTileId = tileId;
    core->suggestNewTask();

    // If we just got a better version of the active image, emit signal
    QuillUndoCommand *current = 0;
    if (command != 0)
        current = stack->command();
    if (current && (current->uniqueId() == previousCommandId)) {
        if (raiseFull)
            core->emitImageAvailable(stack->file(), previousCommandLevel);
        if (raisePartial)
            core->emitTileAvailable(stack->file(), previousTileId);
    }

    if (threadingMode == Quill::ThreadingTest)
        eventLoop->exit();
}

bool ThreadManager::allowDelete(QuillImageFilter *filter) const
{
    return (filter != activeFilter);
}

void ThreadManager::setDebugDelay(int delay)
{
    debugDelay = delay;
}

void ThreadManager::releaseAndWait()
{
    if (threadingMode == Quill::ThreadingTest)
    {
        semaphore->release();
        if (isRunning())
            eventLoop->exec();
    }
}
