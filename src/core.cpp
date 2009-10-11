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

#include <QuillImage>
#include <QuillImageFilter>
#include <QuillImageFilterGenerator>
#include <QDir>
#include "quill.h"
#include "core.h"
#include "quillfile.h"
#include "quillundostack.h"
#include "quillundocommand.h"
#include "imagecache.h"
#include "threadmanager.h"
#include "tilemap.h"
#include "tilecache.h"
#include "historyxml.h"
class CorePrivate
{
public:
    QSize viewPortSize;

    QList<QSize> previewSize;
    QList<ImageCache*> cache;

    QString editHistoryDirectory;
    QList<QString> thumbnailDirectory;
    QString thumbnailExtension;

    QMap<QString, QuillFile*> files;

    QSize defaultTileSize;
    int saveBufferSize;

    ThreadManager *threadManager;

    TileCache *tileCache;
};

Core::Core(const QSize &viewPortSize,
           Quill::ThreadingMode threadingMode)
{
    priv = new CorePrivate();
    priv->viewPortSize = viewPortSize;
    priv->files = QMap<QString, QuillFile*>();
    priv->threadManager = new ThreadManager(this, threadingMode);

    priv->previewSize.append(viewPortSize);
    priv->editHistoryDirectory = QDir::homePath() + "/.config/quill/history";
    priv->thumbnailDirectory.append(QString());
    priv->cache.append(new ImageCache(Quill::defaultCacheSize));
    priv->cache.append(new ImageCache(Quill::defaultCacheSize));
    priv->tileCache = new TileCache(100);

    // Current default is no tiling
    priv->defaultTileSize = QSize();
    priv->saveBufferSize = 65536*16;
}

Core::~Core()
{
    while (!priv->files.isEmpty()) {
        delete *(priv->files.begin());
        priv->files.erase(priv->files.begin());
    }

    while (!priv->cache.isEmpty()) {
        delete priv->cache.first();
        priv->cache.removeFirst();
    }
    delete priv->tileCache;
    delete priv->threadManager;
    delete priv;
}

void Core::setPreviewLevelCount(int count)
{
    // Only works with empty stack and count >= 1
    if (!priv->files.isEmpty() || (count <= 0))
        return;

    int oldCount = priv->previewSize.count();
    if (count > oldCount)
        for (int i = oldCount; i < count; i++) {
            priv->previewSize.append(priv->previewSize.last() * 2);
            priv->cache.append(new ImageCache(Quill::defaultCacheSize));
            priv->thumbnailDirectory.append(QString());
        }
    else if (count < oldCount)
        for (int i = oldCount; i > count; i--) {
            priv->previewSize.removeLast();
            delete priv->cache.last();
            priv->cache.removeLast();
            priv->thumbnailDirectory.removeLast();
        }
}

int Core::previewLevelCount() const
{
    return priv->previewSize.count();
}

ImageCache *Core::cache(int level) const
{
    return priv->cache[level];
}

void Core::setPreviewSize(int level, const QSize &size)
{
    if ((level < 0) || (level >= priv->previewSize.count()))
        return;

    priv->previewSize.replace(level, size);
}

QSize Core::previewSize(int level) const
{
    if ((level >=0 ) && (level < priv->previewSize.count()))
        return priv->previewSize[level];
    else
        return QSize();
}

void Core::setCacheLimit(int level, int limit)
{
    if ((level < 0) || (level >= priv->cache.count()))
        return;

    priv->cache[level]->setMaxCost(limit);
}

QuillFile *Core::file(const QString &fileName,
                      const QString &fileFormat)
{
    QuillFile *file = priv->files[fileName];

    if (file)
        return file;

    file = QuillFile::readFromEditHistory(fileName, this);

    if (file)
    {
        priv->files.insert(fileName, file);
        return file;
    }

    file = new QuillFile(this);
    file->setFileName(fileName);

    QFileInfo fileInfo(fileName);
    file->setOriginalFileName(fileInfo.path() + "/.original/" +
                              fileInfo.fileName());

    file->setFileFormat(fileFormat);
    file->setTargetFormat(fileFormat);

    priv->files.insert(fileName, file);
    return file;
}

QuillUndoCommand *Core::findInAllStacks(int id)
{
    QuillUndoCommand *command = 0;

    for (QMap<QString, QuillFile*>::iterator file = priv->files.begin();
         file != priv->files.end(); file++)
        if ((*file)->exists())
        {
            command = (*file)->stack()->find(id);
            if (command)
                break;
        }
    return command;
}

QuillFile *Core::priorityFile() const
{
    QString name;
    int level = -1;

    for (QMap<QString, QuillFile*>::iterator file = priv->files.begin();
         file != priv->files.end(); file++)
    {
        if ((*file)->exists() && (*file)->supported() &&
            ((*file)->displayLevel() > level))
        {
            name = (*file)->fileName();
            level = (*file)->displayLevel();
        }
    }

    if (name != "")
        return priv->files[name];
    else
        return 0;
}

QuillFile *Core::prioritySaveFile() const
{
    for (QMap<QString, QuillFile*>::iterator file = priv->files.begin();
         file != priv->files.end(); file++)
    {
        if ((*file)->isSaveInProgress())
        {
            return *file;
        }
    }

    return 0;
}

QList<QuillFile*> Core::existingFiles() const
{
    QList<QuillFile*> files;

    for (QMap<QString, QuillFile*>::iterator file = priv->files.begin();
         file != priv->files.end(); file++)
    {
        if ((*file)->exists())
            files += *file;
    }
    return files;
}

void Core::suggestNewTask()
{
    // Make sure that nothing is already running on the background.

    if (priv->threadManager->isRunning())
        return;

    // No files means no operation

    if (priv->files.isEmpty())
        return;

    QList<QuillFile*> allFiles = existingFiles();

    // First priority (all files): loading any pre-generated thumbnails
    for (QList<QuillFile*>::iterator file = allFiles.begin();
         file != allFiles.end(); file++) {

        int maxLevel = (*file)->displayLevel();
        if (maxLevel >= previewLevelCount())
            maxLevel = previewLevelCount()-1;

        for (int level=0; level<=maxLevel; level++) {
            if (priv->threadManager->suggestThumbnailLoadTask((*file), level))
                return;
        }
    }

    // Second priority (highest display level): all preview levels

    QuillFile *priorityFile = this->priorityFile();

    if (priorityFile) {

        int maxLevel = priorityFile->displayLevel();
        if (maxLevel >= previewLevelCount())
            maxLevel = previewLevelCount()-1;

        for (int level=0; level<=maxLevel; level++)
            if (priv->threadManager->suggestNewTask(priorityFile, level))
                return;
    }

    // Third priority (save in progress): getting final full image/tiles

    QuillFile *prioritySaveFile = this->prioritySaveFile();

    if (prioritySaveFile) {

        if (priv->threadManager->suggestNewTask(prioritySaveFile,
                                                previewLevelCount()))
            return;

        // Fourth priority (save in progress): saving image

        if (priv->threadManager->suggestSaveTask(prioritySaveFile))
            return;
    }

    // Fifth priority (highest display level): full image/tiles

    if ((priorityFile != 0) &&
        (priorityFile->displayLevel() >= previewLevelCount())) {

        if (priv->threadManager->suggestNewTask(priorityFile,
                                                previewLevelCount()))
            return;
    }

    // Sixth priority (highest display level):
    // regenerating lower-resolution preview images to
    // better match their respective higher-resolution previews or
    // full images

    if (priorityFile != 0)
        priv->threadManager->suggestPreviewImprovementTask(priorityFile);

    // Seventh priority (all others): all preview levels

    for (QList<QuillFile*>::iterator file = allFiles.begin();
         file != allFiles.end(); file++)
        if ((*file)->supported()) {
            int maxLevel = (*file)->displayLevel();
            if (maxLevel >= previewLevelCount())
                maxLevel = previewLevelCount()-1;

            for (int level=0; level<=maxLevel; level++)
                if (priv->threadManager->suggestNewTask((*file), level))
                    return;
        }

    // Seventh priority (any): saving thumbnails

    for (QList<QuillFile*>::iterator file = allFiles.begin();
         file != allFiles.end(); file++)
        if ((*file)->supported() && (!(*file)->isReadOnly()))
            for (int level=0; level<=previewLevelCount()-1; level++)
                if (priv->threadManager->suggestThumbnailSaveTask((*file), level))
                    return;
}

bool Core::allowDelete(QuillImageFilter *filter) const
{
    return priv->threadManager->allowDelete(filter);
}

void Core::setDefaultTileSize(const QSize &size)
{
    priv->defaultTileSize = size;
}

QSize Core::defaultTileSize() const
{
    return priv->defaultTileSize;
}

void Core::setTileCacheSize(int size)
{
    priv->tileCache->resizeCache(size);
}

void Core::setSaveBufferSize(int size)
{
    priv->saveBufferSize = size;
}

int Core::saveBufferSize() const
{
    return priv->saveBufferSize;
}

TileCache* Core::tileCache() const
{
    return priv->tileCache;
}

QByteArray Core::dump() const
{
    return HistoryXml::encode(priv->files.values());
}

void Core::recover(QByteArray history)
{
    if (!priv->files.isEmpty())
        return;

    QList<QuillFile*> fileList = HistoryXml::decode(history, this);

    for (QMap<QString, QuillFile*>::iterator file = priv->files.begin();
         file != priv->files.end(); file++)
    {
        priv->files.insert((*file)->fileName(), *file);
    }

    suggestNewTask();
}

void Core::setEditHistoryDirectory(const QString &directory)
{
    priv->editHistoryDirectory = directory;
}

QString Core::editHistoryDirectory() const
{
    return priv->editHistoryDirectory;
}

void Core::setThumbnailDirectory(int level, const QString &directory)
{
    priv->thumbnailDirectory[level] = directory;
}

QString Core::thumbnailDirectory(int level) const
{
    if (level >= previewLevelCount())
        return QString();
    else
        return priv->thumbnailDirectory[level];
}

void Core::setThumbnailExtension(const QString &extension)
{
    priv->thumbnailExtension = extension;
}

QString Core::thumbnailExtension() const
{
    return priv->thumbnailExtension;
}

void Core::insertFile(QuillFile *file, const QString &key)
{
    // insertMulti() instead of insert() here since original copies
    // will be using empty strings as keys.
    priv->files.insertMulti(key, file);
}

void Core::releaseAndWait()
{
    priv->threadManager->releaseAndWait();
}

void Core::setDebugDelay(int delay)
{
    priv->threadManager->setDebugDelay(delay);
}

void Core::emitImageAvailable(QuillFile *file, int level)
{
    if (level > file->displayLevel())
        return;

    QuillImage image = file->stack()->image(level);
    image.setZ(level);

    QList<QuillImage> imageList = QList<QuillImage>();
    imageList.append(image);

    file->emitImageAvailable(imageList);
}

void Core::emitTileAvailable(QuillFile *file, int tileId)
{
    if (file->displayLevel() < previewLevelCount())
        return;

    QuillImage image = file->stack()->command()->tileMap()->tile(tileId);

    QList<QuillImage> imageList = QList<QuillImage>();
    imageList.append(image);

    file->emitImageAvailable(imageList);
}
