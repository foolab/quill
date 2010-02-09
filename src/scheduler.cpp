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

#include <QDir>

#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include <QuillImageFilterGenerator>

#include "scheduler.h"
#include "quillerror.h"
#include "file.h"
#include "core.h"
#include "quillundocommand.h"
#include "quillundostack.h"
#include "threadmanager.h"
#include "tilemap.h"
#include "savemap.h"

Scheduler::Scheduler(ThreadManager *threadManager) :
    m_threadManager(threadManager)
{
}

Scheduler::~Scheduler()
{
}

QuillUndoCommand *Scheduler::getTask(QuillUndoStack *stack, int level) const
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

bool Scheduler::suggestTilingTask(File *file)
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

    m_threadManager->startThread(command->uniqueId(),
                                 Core::instance()->previewLevelCount(),
                                 tileIndex,
                                 prevImage,
                                 command->filter());

    return true;
}

bool Scheduler::suggestTilingSaveTask(File *file)
{
    QuillUndoStack *stack = file->stack();
    if (stack->saveMap()->isBufferComplete())
    {
        m_threadManager->startThread(stack->saveCommand()->uniqueId(),
                                     Core::instance()->previewLevelCount(), 0,
                                     stack->saveMap()->buffer(),
                                     stack->saveCommand()->filter());
        return true;
    }
    else
        return suggestTilingOverlayTask(file);
}

bool Scheduler::suggestTilingOverlayTask(File *file)
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

    m_threadManager->startThread(stack->saveCommand()->uniqueId(),
                                 Core::instance()->previewLevelCount(),
                                 tileId,
                                 prevImage,
                                 filter);

    return true;
}

bool Scheduler::suggestThumbnailLoadTask(File *file,
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

    m_threadManager->startThread(command->uniqueId(),
                                 level, 0,
                                 QuillImage(),
                                 filter);

    return true;
}

bool Scheduler::suggestThumbnailSaveTask(File *file, int level)
{
    QuillUndoStack *stack = file->stack();

    if ((!stack) || (!stack->command()))
        return false;

    if ((Core::instance()->thumbnailDirectory(level).isEmpty()) ||
        (file->image(level).isNull()) ||
        (file->isDirty()) ||
        (file->hasThumbnail(level)))
        return false;

    if (file->image(level).size() !=
        file->stack()->command()->targetPreviewSize(level))
        return false;

    if(!QDir().mkpath(Core::instance()->thumbnailDirectory(level)))
        file->emitError(QuillError(QuillError::DirCreateError,
                                   QuillError::ThumbnailErrorSource,
                                   Core::instance()->thumbnailDirectory(level)));

    QuillImageFilter *filter = QuillImageFilterFactory::createImageFilter(QuillImageFilter::Role_Save);

    filter->setOption(QuillImageFilter::FileName,
                      file->thumbnailFileName(level));

    m_threadManager->startThread(file->stack()->command()->uniqueId(),
                                 level, 0,
                                 file->image(level),
                                 filter);

    return true;
}

bool Scheduler::suggestNewTask(File *file, int level)
{
    QuillUndoStack *stack = file->stack();

    if (!stack->command())
        // Empty stack command - should never happen
        return false;

    if (stack->image(level) != QuillImage())
        // Image already exists - no need to recalculate
        return false;

    if ((level == Core::instance()->previewLevelCount()) &&
        (!Core::instance()->defaultTileSize().isEmpty()))
        return suggestTilingTask(file);

    // The given resolution level is missing

    QuillUndoCommand *command = getTask(stack, level);

    // If a file is currently waiting for data, load should not be tried
    if ((command->filter()->role() == QuillImageFilter::Role_Load) &&
        (file->isWaitingForData()))
        return false;

    QuillUndoCommand *prev = 0;
    if (command->filter()->role() != QuillImageFilter::Role_Load)
        prev = command->prev();

    QuillImage prevImage;
    if ((prev == 0) && (level == Core::instance()->previewLevelCount()))
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
            prevImage = prev->bestImage(Core::instance()->previewLevelCount());
        else
            prevImage = prev->image(level);
    }

    // Commands with errors should be ignored
    if (command->fullImageSize().isEmpty())
        return false;

    m_threadManager->startThread(command->uniqueId(),
                                 level, 0,
                                 prevImage,
                                 command->filter());

    return true;
}

bool Scheduler::suggestSaveTask(File *file)
{
    QuillUndoStack *stack = file->stack();

    // If the current command has an image which has not been saved before.
    if (!stack->isDirty())
        return false;

    // Tiling save variant
    if (!Core::instance()->defaultTileSize().isEmpty())
        return suggestTilingSaveTask(file);

    m_threadManager->startThread(stack->saveCommand()->uniqueId(),
                                 Core::instance()->previewLevelCount(), 0,
                                 stack->image(Core::instance()->previewLevelCount()),
                                 stack->saveCommand()->filter());

    return true;
}

bool Scheduler::suggestPreviewImprovementTask(File *file)
{
    QuillUndoStack *stack = file->stack();

    // If the current command already has a full image, we see if the
    // previews need re-calculating

    QuillUndoCommand *command = stack->command();

    int level;
    QSize targetSize;

    for (level=0; level<Core::instance()->previewLevelCount(); level++)
    {
        // Preview images cannot be bigger than full image
        targetSize = command->targetPreviewSize(level);
        if ((!command->image(level).size().isEmpty()) &&
            (command->image(level).size() != targetSize))
            break;
    }

    if (level < Core::instance()->previewLevelCount())
    {
        QuillImage prevImage = command->fullImage();

        if (prevImage == QuillImage())
        {
            // The case with tiling (base on the largest preview)
            if (level < Core::instance()->previewLevelCount() - 1)
                prevImage = command->image(Core::instance()->previewLevelCount() - 1);

            if (prevImage == QuillImage())
                return false;
        }

        // Create ad-hoc filter for preview re-calculation

        QuillImageFilter *scaleFilter =
            QuillImageFilterFactory::createImageFilter(QuillImageFilter::Role_PreviewScale);
        scaleFilter->setOption(QuillImageFilter::SizeAfter,
                               QVariant(targetSize));

        // Treat image as a full image

        m_threadManager->startThread(command->uniqueId(),
                                     level, 0,
                                     QuillImage(QImage(prevImage)),
                                     scaleFilter);

        return true;

    }
    // This should never happen
    return false;
}

void Scheduler::processFinishedTask(int commandId, int commandLevel, int tileId,
                                    QuillImage image,
                                    QuillImageFilter *activeFilter)
{
    bool imageUpdated = false;

    // See if the command is still in the stack.

    QuillUndoCommand *command = Core::instance()->findInAllStacks(commandId);
    QuillUndoStack *stack = 0;
    if (command != 0)
        stack = command->stack();

    // Set active filter to zero (so that we do not block its deletion)

    QuillImageFilter *filter = activeFilter;
    activeFilter = 0;

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
            // Save failed - disabling thumbnailing
            if (image.isNull()) {
                stack->file()->emitError(QuillError(QuillError::FileWriteError,
                                                    QuillError::ThumbnailErrorSource,
                                                    filter->option(QuillImageFilter::FileName).toString()));
                qDebug() << "Thumbnail save failed!";
                Core::instance()->setThumbnailCreationEnabled(false);
            }

            // Thumbnail saving - delete temporary filter
            delete filter;
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
    else if ((commandLevel >= Core::instance()->previewLevelCount()) &&
             !Core::instance()->defaultTileSize().isEmpty())
    {
        // A fragment of the full image has been calculated

        command->tileMap()->setTile(tileId, image);
        imageUpdated = true;
    }
    else
    {
        // Ad-hoc filters (preview improvement and thumbnail loading)
        if (command->filter() != filter)
        {
            // Thumbnail load failed
            if ((image.isNull()) &&
                (filter->role() == QuillImageFilter::Role_Load)) {
                QuillError quillError =
                    QuillError(QuillError::translateFilterError(filter->error()),
                               QuillError::ThumbnailErrorSource,
                               filter->option(QuillImageFilter::FileName).toString());
                stack->file()->emitError(quillError);
                stack->file()->removeThumbnails();
                stack->file()->setThumbnailError(true);
            }

            image = QuillImage(image, command->fullImageSize());
            delete filter;
        } else if ((image.isNull()) &&
                   (command->filter()->role() == QuillImageFilter::Role_Load)) {
            File *file = stack->file();

            // Prevent repeating the faulty operation
            command->setFullImageSize(QSize());

            // Normal load failed
            QuillError::ErrorCode errorCode =
                QuillError::translateFilterError(filter->error());

            QString fileName = filter->option(QuillImageFilter::FileName).toString();
            QuillError::ErrorSource errorSource;
            qDebug() << "Normal load failed!" << fileName << file->fileName();

            if (fileName == file->fileName()) {
                errorSource = QuillError::ImageFileErrorSource;
                if ((errorCode == QuillError::FileFormatUnsupportedError) ||
                    (errorCode == QuillError::FileCorruptError))
                    file->setSupported(false);
                else
                    file->setExists(false);
            }
            else {
                errorSource = QuillError::ImageOriginalErrorSource;
            }

            file->emitError(QuillError(errorCode, errorSource,
                                       fileName));

            Core::instance()->suggestNewTask();
        }

        // Normal case: a better version of an image has been calculated.

        command->setImage(commandLevel, image);
        imageUpdated = true;
    }

    // since this might be altered by suggestNewTask
    int previousCommandLevel = commandLevel;
    Core::instance()->suggestNewTask();

    // If we just got a better version of the active image, emit signal

    if (imageUpdated) {
        QuillUndoCommand *current = 0;
        if (command != 0)
            current = stack->command();

        if (current && (current->uniqueId() == commandId)) {
            image.setZ(previousCommandLevel);
            stack->file()->emitSingleImage(image, previousCommandLevel);
        }
    }
}