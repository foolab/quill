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
#include <QDebug>
#include "quill.h"
#include "quillerror.h"
#include "core.h"
#include "file.h"
#include "quillundostack.h"
#include "quillundocommand.h"
#include "imagecache.h"
#include "threadmanager.h"
#include "tilemap.h"
#include "tilecache.h"
#include "historyxml.h"

Core *Core::g_instance = 0;

Core::Core(Quill::ThreadingMode threadingMode) :
    m_editHistoryDirectory(QDir::homePath() + "/.config/quill/history"),
    m_thumbnailCreationEnabled(true),
    m_saveBufferSize(65536*16),
    m_tileCache(new TileCache(100)),
    m_threadManager(new ThreadManager(threadingMode)),
    m_temporaryFileDirectory(QString()),
    m_crashDumpPath(QString())
{
    m_previewSize.append(Quill::defaultViewPortSize);
    m_thumbnailDirectory.append(QString());
    m_fileLimit.append(1);
    m_fileLimit.append(1);
    m_cache.append(new ImageCache(0));
    m_cache.append(new ImageCache(0));

    qRegisterMetaType<QuillImageList>("QuillImageList");
    qRegisterMetaType<QuillError>("QuillError");
}

Core::~Core()
{
    foreach(File *file, m_files)
        delete file;

    while (!m_cache.isEmpty()) {
        delete m_cache.first();
        m_cache.removeFirst();
    }
    delete m_tileCache;
    delete m_threadManager;
}

void Core::init()
{
    if (!g_instance)
        g_instance = new Core();
}

void Core::initTestingMode()
{
    if (!g_instance)
        g_instance = new Core(Quill::ThreadingTest);
}

void Core::cleanup()
{
    delete g_instance;
    g_instance = 0;
}

Core *Core::instance()
{
    if (!g_instance)
        init();
    return g_instance;
}

void Core::setPreviewLevelCount(int count)
{
    // Only works with empty stack and count >= 1
    if (!m_files.isEmpty() || (count <= 0))
        return;

    int oldCount = m_previewSize.count();
    if (count > oldCount)
        for (int i = oldCount; i < count; i++) {
            m_previewSize.append(m_previewSize.last() * 2);
            m_fileLimit.append(1);
            m_cache.append(new ImageCache(0));
            m_thumbnailDirectory.append(QString());
        }
    else if (count < oldCount)
        for (int i = oldCount; i > count; i--) {
            m_previewSize.removeLast();
            m_fileLimit.removeLast();
            delete m_cache.last();
            m_cache.removeLast();
            m_thumbnailDirectory.removeLast();
        }
}

int Core::previewLevelCount() const
{
    return m_previewSize.count();
}

ImageCache *Core::cache(int level) const
{
    return m_cache[level];
}

void Core::setPreviewSize(int level, const QSize &size)
{
    if ((level < 0) || (level >= m_previewSize.count()))
        return;

    m_previewSize.replace(level, size);
}

QSize Core::previewSize(int level) const
{
    if ((level >=0 ) && (level < m_previewSize.count()))
        return m_previewSize[level];
    else
        return QSize();
}

void Core::setFileLimit(int level, int limit)
{
    if ((level < 0) || (level >= m_fileLimit.count()))
        return;

    m_fileLimit[level] = limit;
}

int Core::fileLimit(int level) const
{
    if ((level >=0 ) && (level < m_fileLimit.count()))
        return m_fileLimit[level];
    else
        return 0;
}

void Core::setEditHistoryCacheSize(int level, int limit)
{
    if ((level < 0) || (level >= m_cache.count()))
        return;

    m_cache[level]->setMaxSize(limit);
}

int Core::editHistoryCacheSize(int level)
{
    return m_cache[level]->maxSize();
}

bool Core::fileExists(const QString &fileName)
{
    return (m_files.value(fileName) != 0);
}

File *Core::file(const QString &fileName,
                 const QString &fileFormat)
{
    File *file = m_files.value(fileName);

    if (file)
        return file;

    QuillError error;
    file = File::readFromEditHistory(fileName, &error);

    // Any errors in reading the edit history will be reported,
    // however they are never fatal so we can always continue
    if ((error.errorCode() != QuillError::NoError) &&
        (error.errorCode() != QuillError::FileNotFoundError))
        emitError(error);

    if (file) {
        m_files.insert(fileName, file);
        return file;
    }

    // Even if there are errors, we can create an empty edit history
    // and try to continue
    file = new File();
    file->setFileName(fileName);

    QFileInfo fileInfo(fileName);
    file->setOriginalFileName(fileInfo.path() + "/.original/" +
                              fileInfo.fileName());

    file->setFileFormat(fileFormat);
    file->setTargetFormat(fileFormat);

    m_files.insert(fileName, file);
    return file;
}

void Core::detach(File *file)
{
    m_files.remove(m_files.key(file));
}

QuillUndoCommand *Core::findInAllStacks(int id) const
{
    QuillUndoCommand *command = 0;

    for (QMap<QString, File*>::const_iterator file = m_files.begin();
         file != m_files.end(); file++)
        if ((*file)->exists()) {
            command = (*file)->stack()->find(id);
            if (command)
                break;
        }
    return command;
}

File *Core::priorityFile() const
{
    QString name;
    int level = -1;

    for (QMap<QString, File*>::const_iterator file = m_files.begin();
         file != m_files.end(); file++) {
        if ((*file)->exists() && (*file)->supported() &&
            ((*file)->displayLevel() > level)) {
            name = (*file)->fileName();
            level = (*file)->displayLevel();
        }
    }

    if (!name.isEmpty())
        return m_files[name];
    else
        return 0;
}

File *Core::prioritySaveFile() const
{
    for (QMap<QString, File*>::const_iterator file = m_files.begin();
         file != m_files.end(); file++) {
        if ((*file)->isSaveInProgress())
            return *file;
    }

    return 0;
}

QList<File*> Core::existingFiles() const
{
    QList<File*> files;

    for (QMap<QString, File*>::const_iterator file = m_files.begin();
         file != m_files.end(); file++)
    {
        if ((*file)->exists())
            files += *file;
    }
    return files;
}

void Core::suggestNewTask()
{
    // Make sure that nothing is already running on the background.

    if (m_threadManager->isRunning())
        return;

    // No files means no operation

    if (m_files.isEmpty())
        return;

    QList<File*> allFiles = existingFiles();

    // First priority (all files): loading any pre-generated thumbnails
    for (QList<File*>::iterator file = allFiles.begin();
         file != allFiles.end(); file++) {

        int maxLevel = (*file)->displayLevel();
        if (maxLevel >= previewLevelCount())
            maxLevel = previewLevelCount()-1;

        for (int level=0; level<=maxLevel; level++) {
            if (m_threadManager->suggestThumbnailLoadTask((*file), level))
                return;
        }
    }

    // Second priority (highest display level): all preview levels

    File *priorityFile = this->priorityFile();

    if (priorityFile) {

        int maxLevel = priorityFile->displayLevel();
        if (maxLevel >= previewLevelCount())
            maxLevel = previewLevelCount()-1;

        for (int level=0; level<=maxLevel; level++)
            if (m_threadManager->suggestNewTask(priorityFile, level))
                return;
    }

    // Third priority (save in progress): getting final full image/tiles

    File *prioritySaveFile = this->prioritySaveFile();

    if (prioritySaveFile) {

        if (m_threadManager->suggestNewTask(prioritySaveFile,
                                            previewLevelCount()))
            return;

        // Fourth priority (save in progress): saving image

        if (m_threadManager->suggestSaveTask(prioritySaveFile))
            return;
    }

    // Fifth priority (highest display level): full image/tiles

    if ((priorityFile != 0) &&
        (priorityFile->displayLevel() >= previewLevelCount())) {

        if (m_threadManager->suggestNewTask(priorityFile,
                                                previewLevelCount()))
            return;
    }

    // Sixth priority (highest display level):
    // regenerating lower-resolution preview images to
    // better match their respective higher-resolution previews or
    // full images

    if (priorityFile != 0)
        m_threadManager->suggestPreviewImprovementTask(priorityFile);

    // Seventh priority (all others): all preview levels

    for (QList<File*>::iterator file = allFiles.begin();
         file != allFiles.end(); file++)
        if ((*file)->supported()) {
            int maxLevel = (*file)->displayLevel();
            if (maxLevel >= previewLevelCount())
                maxLevel = previewLevelCount()-1;

            for (int level=0; level<=maxLevel; level++)
                if (m_threadManager->suggestNewTask((*file), level))
                    return;
        }

    // Seventh priority (any): saving thumbnails

    if (m_thumbnailCreationEnabled)
        foreach(File *file, allFiles)
            if (file->supported() && !file->isReadOnly())
                for (int level=0; level<=previewLevelCount()-1; level++)
                    if (m_threadManager->suggestThumbnailSaveTask(file, level))
                        return;
}

bool Core::allowDelete(QuillImageFilter *filter) const
{
    return m_threadManager->allowDelete(filter);
}

void Core::setDefaultTileSize(const QSize &size)
{
    m_defaultTileSize = size;
}

QSize Core::defaultTileSize() const
{
    return m_defaultTileSize;
}

void Core::setTileCacheSize(int size)
{
    m_tileCache->resizeCache(size);
}

void Core::setSaveBufferSize(int size)
{
    m_saveBufferSize = size;
}

int Core::saveBufferSize() const
{
    return m_saveBufferSize;
}

TileCache* Core::tileCache() const
{
    return m_tileCache;
}

void Core::dump()
{
    if (m_crashDumpPath.isEmpty())
        return;

    QList<File*> fileList;

    foreach (File *file, m_files)
        if (file->isDirty() || file->isSaveInProgress())
            fileList.append(file);

    QString history;
    if (!fileList.isEmpty())
        history = HistoryXml::encode(fileList);

    if (!QDir().mkpath(m_crashDumpPath)) {
        emitError(QuillError(QuillError::DirCreateError,
                             QuillError::CrashDumpErrorSource,
                             m_crashDumpPath));
        return;
    }
    const QString fileName = m_crashDumpPath + QDir::separator() + "dump.xml";
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        emitError(QuillError(QuillError::FileOpenForWriteError,
                             QuillError::CrashDumpErrorSource,
                             fileName));
        return;
    }
    qint64 fileSize = file.write(history.toAscii());
    if (fileSize == -1)
        emitError(QuillError(QuillError::FileWriteError,
                             QuillError::CrashDumpErrorSource,
                             fileName));
    file.close();
}

bool Core::canRecover()
{
    if (!m_files.isEmpty() || m_crashDumpPath.isEmpty())
        return false;

    QFile file(m_crashDumpPath + QDir::separator() + "dump.xml");
    if (file.exists() && file.size() > 0)
        return true;
    else
        return false;
}

void Core::recover()
{
    if (!canRecover())
        return;

    QFile file(m_crashDumpPath + QDir::separator() + "dump.xml");
    if (!file.open(QIODevice::ReadOnly)) {
        emitError(QuillError(QuillError::FileOpenForReadError,
                             QuillError::CrashDumpErrorSource,
                             file.fileName()));
        return;
    }
    const QByteArray history = file.readAll();
    if (history.isEmpty()) {
        emitError(QuillError(QuillError::FileReadError,
                             QuillError::CrashDumpErrorSource,
                             file.fileName()));
    }
    QList<File*> fileList = HistoryXml::decode(history);
    if (fileList.count() == 0) {
        emitError(QuillError(QuillError::FileCorruptError,
                             QuillError::CrashDumpErrorSource,
                             file.fileName()));
    }

    foreach (File *file, fileList) {
        m_files.insert(file->fileName(), file);
        file->save();
    }

    suggestNewTask();
}

QString Core::crashDumpPath() const
{
    return m_crashDumpPath;
}

void Core::setCrashDumpPath(const QString &fileName)
{
    m_crashDumpPath = fileName;
}

void Core::setEditHistoryDirectory(const QString &directory)
{
    m_editHistoryDirectory = directory;
}

QString Core::editHistoryDirectory() const
{
    return m_editHistoryDirectory;
}

void Core::setThumbnailDirectory(int level, const QString &directory)
{
    m_thumbnailDirectory[level] = directory;
}

QString Core::thumbnailDirectory(int level) const
{
    if (level >= previewLevelCount())
        return QString();
    else
        return m_thumbnailDirectory[level];
}

void Core::setThumbnailExtension(const QString &extension)
{
    m_thumbnailExtension = extension;
}

QString Core::thumbnailExtension() const
{
    return m_thumbnailExtension;
}

void Core::setThumbnailCreationEnabled(bool enabled)
{
    m_thumbnailCreationEnabled = enabled;
    if (enabled)
        suggestNewTask();
}

bool Core::isThumbnailCreationEnabled() const
{
    return m_thumbnailCreationEnabled;
}

void Core::insertFile(File *file, const QString &key)
{
    m_files.insert(key, file);
}

void Core::releaseAndWait()
{
    m_threadManager->releaseAndWait();
}

void Core::setDebugDelay(int delay)
{
    m_threadManager->setDebugDelay(delay);
}

int Core::numFilesAtLevel(int level) const
{
    int n = 0;
    for (QMap<QString, File*>::const_iterator file = m_files.begin();
         file != m_files.end(); file++) {
        if ((*file)->displayLevel() >= level)
            n++;
    }
    return n;
}

bool Core::isSaveInProgress() const
{
    return (prioritySaveFile() != 0);
}

void Core::setTemporaryFileDirectory(const QString &fileDir)
{
    m_temporaryFileDirectory = fileDir;
}

QString Core::temporaryFileDirectory() const
{
    return m_temporaryFileDirectory;
}

void Core::emitSaved(QString fileName)
{
    emit saved(fileName);
}

void Core::emitError(QuillError quillError)
{
    emit error(quillError);
}
