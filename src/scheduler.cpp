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

#include <limits.h>
#include <QDir>
#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include <QuillImageFilterGenerator>
#include "scheduler.h"
#include "quillerror.h"
#include "task.h"
#include "file.h"
#include "core.h"
#include "quillundocommand.h"
#include "quillundostack.h"
#include "tilemap.h"
#include "savemap.h"
#include "imagecache.h"
#include "logger.h"
#include "strings.h"

Scheduler::Scheduler()
{
}

Scheduler::~Scheduler()
{
}

Task *Scheduler::newTask()
{
    const QList<File*> fileList = Core::instance()->fileList();
    // No files means no operation

    if(fileList.isEmpty())
        return 0;

    const int previewLevelCount = Core::instance()->previewLevelCount();

    // First priority (high priority files): loading any
    // pre-generated thumbnails (lowest levels first)

    {
        Task *task = newThumbnailLoadTask(fileList,
                                          QuillFile::Priority_Normal);
        if (task)
            return task;
    }

    // Second priority (any): saving thumbnails
    // Saving a thumbnail is very fast compared to generating one,
    // it should be done whenever possible.

    if (Core::instance()->isThumbnailCreationEnabled())

        foreach(File* file, fileList){
            for (int level=0; level<=previewLevelCount-1; level++) {
                Task *task = newThumbnailSaveTask(file, level);

                if (task)
                    return task;
            }
        }
    // Third priority (high priority files): all preview levels
    // (lowest first)

    {
        Task *task = newNormalTask(fileList,
                                   QuillFile::Priority_Normal);
        if (task)
            return task;
    }

    // Fourth priority (low priority files): pre-generated thumbnails

    {
        Task *task = newThumbnailLoadTask(fileList, INT_MIN);
        if (task)
            return task;
    }

    // Fifth priority (low priority files): all preview levels
    // (lowest levels first)

    {
        Task *task = newNormalTask(fileList, INT_MIN);
        if (task)
            return task;
    }

    // Sixth priority (all images):
    // regenerating lower-resolution preview images to
    // better match their respective higher-resolution previews or
    // full images

    foreach(File* file, fileList){
        Task *task = newPreviewImprovementTask(file);

        if (task)
            return task;
    }

    // Seventh priority (save in progress): getting final full image/tiles

    File *prioritySaveFile = Core::instance()->prioritySaveFile();

    if (prioritySaveFile) {

        Task *task = newNormalTask(prioritySaveFile, previewLevelCount);
        if (task)
            return task;

        // Eighth priority (save in progress): saving image

        task = newSaveTask(prioritySaveFile);

        if (task)
            return task;
    }

    // Last priority (highest display level): full image/tiles

    File *priorityFile = Core::instance()->priorityFile();

    if ((priorityFile != 0) &&
        (priorityFile->displayLevel() >= previewLevelCount)) {

        Task *task = newNormalTask(priorityFile, previewLevelCount);

        if (task)
            return task;
    }

    return 0;
}


QuillUndoCommand *Scheduler::getTask(QuillUndoStack *stack, int level) const
{
    int index;

    for (index=stack->index()-1; index>=0; index--)
    {
        if (!stack->command(index)->image(level).isNull())
            break;

        // Check if there's any loadCommand associated with this index
        if ((stack->savedIndex() > 0) && (stack->savedIndex() == index))
            return stack->loadCommand();
    }

    // If nothing else, this will return the initial load command.
    return stack->command(index + 1);
}

Task *Scheduler::newTilingTask(File *file)
{
    QuillUndoStack *stack = file->stack();
    int tileIndex;

    if (stack->saveCommand())
    {
        // If we can run the actual save filter
        if (stack->saveMap()->isBufferComplete())
            return 0;

        // If we already have tiles to push to the save buffer
        if (stack->saveMap()->processNext(stack->command()->tileMap()) != -1)
            return 0;
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
            return 0;

        tileIndex = stack->command()->tileMap()->
            prioritize(file->viewPort());
    }

    // We have all the tiles we want already
    if (tileIndex == -1)
        return 0;

    int index;

    for (index=stack->index()-1; index>=0; index--)
    {
        if (!stack->command(index)->tileMap()->tile(tileIndex).isNull())
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

    Task *task = new Task();
    task->setCommandId(command->uniqueId());
    task->setDisplayLevel(Core::instance()->previewLevelCount());
    task->setTileId(tileIndex);
    task->setFilter(command->filter());
    task->setInputImage(prevImage);

    return task;
}

Task *Scheduler::newTilingSaveTask(File *file)
{
    QuillUndoStack *stack = file->stack();
    //Make sure the m_saveMap is not null pointer from stack.
    if(!stack->saveMap())
        return 0;
    if (stack->saveMap()->isBufferComplete())
    {
        Task *task = new Task();
        task->setCommandId(stack->saveCommand()->uniqueId());
        task->setDisplayLevel(Core::instance()->previewLevelCount());
        task->setFilter(stack->saveCommand()->filter());
        task->setInputImage(stack->saveMap()->buffer());
        return task;
    }
    else
        return newTilingOverlayTask(file);
}

Task *Scheduler::newTilingOverlayTask(File *file)
{
    QuillUndoStack *stack = file->stack();
    int tileId = stack->saveMap()->processNext(stack->command()->tileMap());

    if(tileId < 0)
        return 0;
    // Should never happen.
    if (stack->command()->tileMap()->tile(tileId).isNull())
        return 0;

    QuillImageFilter *filter = stack->saveMap()->addToBuffer(tileId);

    QuillImage prevImage = stack->command()->tileMap()->tile(tileId);

    Task *task = new Task();
    task->setCommandId(stack->saveCommand()->uniqueId());
    task->setDisplayLevel(Core::instance()->previewLevelCount());
    task->setTileId(tileId);
    task->setFilter(filter);
    task->setInputImage(prevImage);

    return task;
}

Task *Scheduler::newThumbnailLoadTask(const QList<File*> &fileList, int minPriority)
{
    int previewLevelCount = Core::instance()->previewLevelCount();
    for (int level=0; level<=previewLevelCount-1; level++)
        foreach(File* file, fileList){
            if (level <= file->displayLevel() &&
                (file->priority() >= minPriority)) {
                Task *task = 0;
                if (file->hasThumbnail(level)){
                    task = newThumbnailLoadTask(file, level);
                }
                if (task)
                    return task;
            }
        }
    return 0;
}

Task *Scheduler::newThumbnailLoadTask(File *file, int level) const
{
    QuillUndoStack *stack = file->stack();
    if (!stack || stack->isClean())
        return 0;

    if (stack->hasImage(level))
        // Image already exists - no need to recalculate
        return 0;

    QuillUndoCommand *command = getTask(stack, level);

    if (command->index() != stack->savedIndex())
        return 0;

    QuillImageFilter *filter = QuillImageFilterFactory::createImageFilter(QuillImageFilter::Role_Load);
    filter->setOption(QuillImageFilter::FileName,
                      file->thumbnailFileName(level));
    filter->setOption(QuillImageFilter::BackgroundColor,
                      Core::instance()->backgroundRenderingColor());
    filter->setOption(QuillImageFilter::FileFormat,
                      Core::instance()->thumbnailExtension());
    filter->setOption(QuillImageFilter::IgnoreExifOrientation,
                      QVariant(true));

    Task *task = new Task();
    task->setCommandId(command->uniqueId());
    task->setDisplayLevel(level);
    task->setFilter(filter);
    return task;
}

Task *Scheduler::newThumbnailSaveTask(File *file, int level)
{
    if (file->isOriginal() || (file->isWaitingForData()))
        return 0;
    QuillUndoStack *stack = file->stack();

    if ((!stack) || (!stack->command()))
        return 0;

    if ((Core::instance()->thumbnailFlavorName(level).isEmpty()) ||
        (file->image(level).isNull()) ||
        (file->isDirty()) ||
        (file->hasThumbnail(level)))
        return 0;

    if (file->image(level).size() !=
        Core::instance()->targetSizeForLevel(level, fullSizeForAspectRatio(file)))
        return 0;

    if(!QDir().mkpath(Core::instance()->thumbnailPath(level)))
        file->emitError(QuillError(QuillError::DirCreateError,
                                   QuillError::ThumbnailErrorSource,
                                   Core::instance()->thumbnailPath(level)));

    QuillImageFilter *filter = QuillImageFilterFactory::createImageFilter(QuillImageFilter::Role_Save);

    filter->setOption(QuillImageFilter::FileName,
                      file->thumbnailFileName(level));
    filter->setOption(QuillImageFilter::Timestamp,
                      file->lastModified());

    Task *task = new Task();
    task->setCommandId(file->stack()->command()->uniqueId());
    task->setDisplayLevel(level);
    task->setFilter(filter);
    task->setInputImage(QImage(file->image(level)));

    return task;
}

Task *Scheduler::newNormalTask(const QList<File*> &fileList, int priority)
{
    int previewLevelCount = Core::instance()->previewLevelCount();
    for (int level=0; level<=previewLevelCount-1; level++)
        foreach(File* file, fileList){
            if (file->supportsViewing() && (level <= file->displayLevel()) &&
                (file->priority() >= priority)) {
                Task *task = newNormalTask(file, level);

                if (task)
                    return task;
            }
        }

    return 0;
}

Task *Scheduler::newNormalTask(File *file, int level)
{
    QuillUndoStack *stack = file->stack();

    if (!stack || !stack->command())
        // Empty stack command - should never happen
        return 0;

    if (!stack->image(level).isNull())
        // Image already exists - no need to recalculate
        return 0;

    // For read-only images, we stop loading if we already have
    // an equivalent of the full image
    if ((file->state() == File::State_ReadOnly) &&
        (level > Core::instance()->smallestNonCroppedLevel()) &&
        (stack->image(level-1).size() == stack->fullImageSize()))
        return 0;

    if ((level == Core::instance()->previewLevelCount()) &&
        (!Core::instance()->defaultTileSize().isEmpty()))
        return newTilingTask(file);

    // The given resolution level is missing

    QuillUndoCommand *command = getTask(stack, level);

    // Full image size has not been calculated

    if (command->fullImageSize().isEmpty() &&
        file->supportsViewing() &&
        !file->isWaitingForData())
        stack->calculateFullImageSize(command);

    // If a file is currently waiting for data, load should not be tried
    if ((command->filter()->role() == QuillImageFilter::Role_Load) &&
        (file->isWaitingForData())) {
        return 0;
    }

    QuillUndoCommand *prev = 0;
    if (command->filter()->role() != QuillImageFilter::Role_Load)
        prev = command->prev();
    else {
        // Stop loading if file is known to be corrupt
        if (file->hasFailedThumbnail()) {
            file->setSupported(false);
            file->emitError(QuillError(QuillError::FileCorruptError,
                                       QuillError::ImageFileErrorSource,
                                       file->fileName()));
            return 0;
        }
    }

    QuillImage prevImage;
    if ((prev == 0) && (level == Core::instance()->previewLevelCount()))
        prevImage = QuillImage();
    else if (prev == 0)
    {
        QSize fullSize = command->fullImageSize();
        //We check if the image size exceeds image size limit
        //if exceeds, we will not create a task at all.
        if (!file->checkImageSize(fullSize)){
            return 0;
        }
        QSize targetSize = Core::instance()->targetSizeForLevel(level, fullSize);
        prevImage = QuillImage();
        prevImage.setFullImageSize(fullSize);
        prevImage.setTargetSize(targetSize);
        prevImage.setArea(Core::instance()->targetAreaForLevel(level, targetSize, fullSize));
        prevImage.setZ(level);
    }
    else
    {
        QuillImageFilterGenerator *generator =
            dynamic_cast<QuillImageFilterGenerator*>(command->filter());

        // Red eye detection always uses the best available image
        if (generator) {
            if (!generator->isUsedOnPreview())
                prevImage = prev->bestImage(Core::instance()->previewLevelCount());
            else{
            //Becuase grid thumbnail is cropped, we can not use it to generate the color histogram for
            //autofix and other detecter as well. We use thumbnail that is not croppped if it exists
                int nonCroppedLevel = Core::instance()->smallestNonCroppedLevel();
                if(prev->hasImage(nonCroppedLevel))
                    level = nonCroppedLevel;
                prevImage = prev->image(level);
            }
        }
        else
            prevImage = prev->image(level);
    }

    // Commands with errors should be ignored
    if (command->fullImageSize().isEmpty())
        return 0;

    Task *task = new Task();
    task->setCommandId(command->uniqueId());
    task->setDisplayLevel(level);
    task->setFilter(command->filter());
    task->setInputImage(prevImage);
    return task;
}

Task *Scheduler::newSaveTask(File *file)
{
    QuillUndoStack *stack = file->stack();

    // Tiling save variant
    if (!Core::instance()->defaultTileSize().isEmpty())
        return newTilingSaveTask(file);

    Task *task = new Task();
    task->setCommandId(stack->saveCommand()->uniqueId());
    task->setDisplayLevel(Core::instance()->previewLevelCount());
    task->setFilter(stack->saveCommand()->filter());
    task->setInputImage(stack->image(Core::instance()->previewLevelCount()));
    return task;
}

Task *Scheduler::newPreviewImprovementTask(File *file)
{
    if (!file->exists() ||
        (file->state() == File::State_ExternallySupportedFormat) ||
        (!file->stack()->fullImageSize().isValid()))
        return 0;

    QuillUndoStack *stack = file->stack();

    // If the current command already has a better preview image, we try
    // to regenerate the lower level images accordingly

    QuillUndoCommand *command = stack->command();
    if (!command)
        return 0;

    int level;
    QSize targetSize;
    QSize fullImageSize = fullSizeForAspectRatio(file);

    if (fullImageSize.isEmpty())
        return 0;

    for (level=0; level<=file->displayLevel(); level++)
    {
        // Preview images cannot be bigger than full image
        targetSize = Core::instance()->targetSizeForLevel(level, fullImageSize);

        if (command->image(level).size() != targetSize)
            break;
    }

    if (level <= file->displayLevel())
    {
        QuillImage prevImage;

        // Based on the next available non-cropped preview
        // In placeholder mode, a level may also use itself as the base
        int startingLevel;
        if (file->state() == File::State_Placeholder)
            startingLevel = level;
        else
            startingLevel = level+1;

        for (int sourceLevel = startingLevel;
             sourceLevel <= file->displayLevel();
             sourceLevel++)
            if (!Core::instance()->minimumPreviewSize(sourceLevel).isValid()
                && !command->image(sourceLevel).isNull()) {
                prevImage = command->image(sourceLevel);
                break;
            }

        if (prevImage.isNull())
            return 0;

        // Create ad-hoc filter for preview re-calculation

        QuillImageFilter *scaleCropFilter =
            QuillImageFilterFactory::createImageFilter(QuillImageFilter::Role_PreviewScale);
        scaleCropFilter->setOption(QuillImageFilter::SizeAfter,
                                   QVariant(targetSize));
        scaleCropFilter->setOption(QuillImageFilter::CropRectangle,
                                   QVariant(Core::instance()->targetAreaForLevel(level, targetSize, fullImageSize)));

        Task *task = new Task();
        task->setCommandId(command->uniqueId());
        task->setDisplayLevel(level);
        task->setFilter(scaleCropFilter);

        task->setInputImage(prevImage);
        return task;
    }
    // This should never happen
    return 0;
}

QSize Scheduler::fullSizeForAspectRatio(const File *file)
{
    QSize fullImageSize = file->stack()->fullImageSize();

    // If the full image size is not valid (unsupported formats), assume that
    // the biggest available preview has the correct aspect ratio
    if (fullImageSize.isEmpty())
        for (int level=0; level<=file->displayLevel(); level++) {
            QSize levelSize = file->image(level).size();
            if (!levelSize.isEmpty())
                fullImageSize = levelSize;
        }

    return fullImageSize;
}

void Scheduler::processFinishedTask(Task *task, QuillImage image)
{
    bool fileDeletionAllowed = false;
    bool imageUpdated = false;
    image.setZ(task->displayLevel());

    // See if the command is still in the stack.

    QuillUndoCommand *command =
        Core::instance()->findInAllStacks(task->commandId());
    QuillUndoStack *stack = 0;
    File *file = 0;
    if (command != 0) {
        stack = command->stack();
        file = stack->file();
    }

    // Set active filter to zero (so that we do not block its deletion)

    QuillImageFilter *filter = task->filter();
    task->setFilter(0);

    QuillImageFilterGenerator *generator =
        dynamic_cast<QuillImageFilterGenerator*>(filter);

    QuillError error;

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
        if (!file->isSaveInProgress()) {
            // Save failed - disabling thumbnailing
            if (image.isNull()) {
                error = QuillError(QuillError::FileWriteError,
                                   QuillError::ThumbnailErrorSource,
                                   filter->option(QuillImageFilter::FileName).toString());
                QUILL_LOG(Logger::Module_Scheduler, "Thumbnail save failed!");
                Core::instance()->setThumbnailCreationEnabled(false);
            }
            else
                file->registerThumbnail(task->displayLevel());

            // if we can release stored thumbnails now
            if ((file->displayLevel() < 0) && (file->stack()->hasImage(0)))
                Core::instance()->cache(0)->purge(file);

            // if the whole file can be deleted now
            if(file->allowDelete())
                fileDeletionAllowed = true;

            // Thumbnail saving - delete temporary filter
            delete filter;
        }

        else if (!stack->saveMap() ||
                 stack->saveMap()->isSaveComplete())
        {
            // If full image saving is concluded, rename the file and clean up.

            file->concludeSave();

            if (file->allowDelete())
                fileDeletionAllowed = true;
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
                createImageFilter(QuillImageFilter::Name_RedEyeReduction);

        delete generator;
        command->setFilter(filter);
    }
    else if ((task->displayLevel() >= Core::instance()->previewLevelCount()) &&
             !Core::instance()->defaultTileSize().isEmpty())
    {
        // A fragment of the full image has been calculated

        command->tileMap()->setTile(task->tileId(), image);
        imageUpdated = true;
    }
    else
    {
        // Ad-hoc filters (preview improvement and thumbnail loading)
        // Uses the Exif orientation option to differentiate from load command
        if ((command->filter() != filter) &&
            ((filter->role() != QuillImageFilter::Role_Load) ||
             filter->option(QuillImageFilter::IgnoreExifOrientation).toBool()))
        {
            // Thumbnail load failed
            if ((image.isNull()) &&
                (filter->role() == QuillImageFilter::Role_Load)) {

                QuillError::ErrorCode errorCode = QuillError::translateFilterError(filter->error());

                // Corner case: thumbnail is not found anymore;
                // e.g. removed by another process. Instead of emitting an error signal,
                // unregister this thumbnail and fallback to normal thumbnail creation
                if (errorCode == QuillError::FileNotFoundError)
                    file->unregisterThumbnail(task->displayLevel());
                else
                {
                    error = QuillError(errorCode,
                                        QuillError::ThumbnailErrorSource,
                                        filter->option(QuillImageFilter::FileName).toString());
                    file->removeThumbnails();
                    file->setThumbnailError(true);
                }
            }

            image = QuillImage(image, command->fullImageSize());
            image.setZ(task->displayLevel());

            delete filter;

        } else if ((image.isNull()) &&
                   (command->filter()->role() == QuillImageFilter::Role_Load)) {

            // Prevent repeating the faulty operation
            command->setFullImageSize(QSize());

            // Normal load failed
            QuillError::ErrorCode errorCode =
                QuillError::translateFilterError(filter->error());

            QString fileName = filter->option(QuillImageFilter::FileName).toString();
            QuillError::ErrorSource errorSource;
            QUILL_LOG(Logger::Module_Scheduler,
                        "Normal load failed from file " + file->fileName());

            if (fileName == file->fileName()) {
                errorSource = QuillError::ImageFileErrorSource;
                if ((errorCode == QuillError::FileFormatUnsupportedError) ||
                    (errorCode == QuillError::FileCorruptError)) {
                    file->setSupported(false);
                    if (Core::instance()->isDBusThumbnailingEnabled() &&
                        Core::instance()->isExternallySupportedFormat(file->fileFormat()))
                        file->setThumbnailSupported(true);
                    else
                        file->addFailedThumbnail();
                }
                else
                    file->setExists(false);
            }
            else {
                errorSource = QuillError::ImageOriginalErrorSource;
            }

            error = QuillError(errorCode, errorSource, fileName);
        }

        // Detect whether the task results to updating the image data to given display level:
        // loaded image must be discarded if display level has been reduced while loading.
        // If there's still unsaved thumbnails or saving is in progress, cleanup
        // will be done automatically after these operations are finished.
        bool skipUpdate = false;
        if (task->displayLevel() > file->displayLevel())
        {
            bool isThumbnailUnSaved = task->displayLevel() == 0 &&
                                      file->hasUnsavedThumbnails();
            bool isRelatedToSaving = file->isSaveInProgress() &&
                                     task->displayLevel() == Core::instance()->previewLevelCount();

            if (!isThumbnailUnSaved && !isRelatedToSaving)
                skipUpdate = true;
        }
        if (!skipUpdate)
        {
            // Normal case: a better version of an image has been calculated.
            command->setImage(task->displayLevel(), image);
            imageUpdated = true;
        }
    }

    // If we just got a better version of the active image, emit signal

    if (imageUpdated) {
        QuillUndoCommand *current = 0;
        if (command != 0)
            current = stack->command();

        if (current && (current->uniqueId() == task->commandId())) {
            file->emitSingleImage(image, task->displayLevel());
        }
    }
    delete task;

    if (file) {
        if (error.errorCode() != QuillError::NoError)
            file->emitError(error);

        if (fileDeletionAllowed)
            delete file;
    }
}
