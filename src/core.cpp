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
#include "task.h"
#include "scheduler.h"
#include "threadmanager.h"
#include "tilemap.h"
#include "tilecache.h"
#include "historyxml.h"
#include "logger.h"
#include "dbus-thumbnailer/dbusthumbnailer.h"

Core *Core::g_instance = 0;

Core::Core(Quill::ThreadingMode threadingMode) :
    m_imageSizeLimit(QSize()),
    m_imagePixelsLimit(0),
    m_nonTiledImagePixelsLimit(0),
    m_editHistoryDirectory(QDir::homePath() + "/.config/quill/history"),
    m_thumbnailCreationEnabled(true),
    m_dBusThumbnailingEnabled(true),
    m_recoveryInProgress(false),
    m_saveBufferSize(65536*16),
    m_tileCache(new TileCache(100)),
    m_scheduler(new Scheduler()),
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

    m_dBusThumbnailer = new DBusThumbnailer;
    connect(m_dBusThumbnailer,
            SIGNAL(thumbnailGenerated(const QString, const QString)),
            SLOT(processDBusThumbnailerGenerated(const QString, const QString)));

    connect(m_dBusThumbnailer,
            SIGNAL(thumbnailError(const QString, uint, const QString)),
            SLOT(processDBusThumbnailerError(const QString, uint, const QString)));
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
    delete m_scheduler;
    delete m_dBusThumbnailer;
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

void Core::setImageSizeLimit(const QSize &size)
{
    m_imageSizeLimit = size;
}

QSize Core::imageSizeLimit() const
{
    return m_imageSizeLimit;
}

void Core::setImagePixelsLimit(int pixels)
{
    m_imagePixelsLimit = pixels;
}

int Core::imagePixelsLimit() const
{
    return m_imagePixelsLimit;
}

void Core::setNonTiledImagePixelsLimit(int pixels)
{
    m_nonTiledImagePixelsLimit = pixels;
}

int Core::nonTiledImagePixelsLimit() const
{
    return m_nonTiledImagePixelsLimit;
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

    QFileInfo fileInfo(fileName);
    QString originalFileName =
        fileInfo.path() + "/.original/" + fileInfo.fileName();
    file = File::readFromEditHistory(fileName, originalFileName, &error);

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

    file->setOriginalFileName(originalFileName);

    file->setFileFormat(fileFormat);
    file->setTargetFormat(fileFormat);

    m_files.insert(fileName, file);
    return file;
}

void Core::attach(File *file)
{
    m_files.insert(file->fileName(), file);
}

void Core::detach(File *file)
{
    m_files.remove(m_files.key(file));
}

QuillUndoCommand *Core::findInAllStacks(int id) const
{
    QuillUndoCommand *command = 0;

    foreach (File *file, m_files)
        if (file->exists()) {
            command = file->stack()->find(id);
            if (command)
                break;
        }
    return command;
}

File *Core::priorityFile() const
{
    QString name;
    int level = -1;

    foreach (File *file, m_files)
        if (file->exists() && file->supported() &&
            (file->displayLevel() > level)) {
            name = file->fileName();
            level = file->displayLevel();
        }

    if (!name.isEmpty())
        return m_files[name];
    else
        return 0;
}

File *Core::prioritySaveFile() const
{
    foreach (File *file, m_files) {
        if (file->isSaveInProgress())
            return file;
    }

    return 0;
}

QList<File*> Core::existingFiles() const
{
    QList<File*> files;

    foreach (File *file, m_files) {
        if (file->exists())
            files += file;
    }
    return files;
}

void Core::suggestNewTask()
{
    // The D-Bus thumbnailer runs on a different process instead of
    // Quill's background thread so it can always be invoked.
    if (m_dBusThumbnailingEnabled)
        activateDBusThumbnailer();

    // Make sure that nothing is already running on the background.

    if (m_threadManager->isRunning())
        return;

    Task *task = m_scheduler->newTask();

    if (task)
        m_threadManager->run(task);
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

    // If crash recovery has been in progress but has ended,
    // clear the recovery flag here
    if (m_recoveryInProgress && !isSaveInProgress())
        m_recoveryInProgress = false;

    // If crash recovery is still in progress, crash dumping is disabled
    QString history;
    if (!fileList.isEmpty() && !m_recoveryInProgress)
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
    else
        m_recoveryInProgress = true;

    // dump an error marker for the duration of the recovery to prevent
    // multiple crashes
    dump();

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

void Core::setDBusThumbnailingEnabled(bool enabled)
{
    m_dBusThumbnailingEnabled = enabled;
}

bool Core::isDBusThumbnailingEnabled() const
{
    return m_dBusThumbnailingEnabled;
}

void Core::insertFile(File *file, const QString &key)
{
    m_files.insert(key, file);
}

void Core::processFinishedTask(Task *task, QuillImage resultImage)
{
    m_scheduler->processFinishedTask(task, resultImage);
    suggestNewTask();
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
    foreach (File *file, m_files)
        if (file->displayLevel() >= level)
            n++;

    return n;
}

bool Core::isCalculationInProgress() const
{
    return (m_threadManager->isRunning());
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

QString Core::flavorFromLevel(int level)
{
    return QDir(thumbnailDirectory(level)).dirName();
}

int Core::levelFromFlavor(QString flavor)
{
    for (int level=0; level<=previewLevelCount()-1; level++)
        if (flavorFromLevel(level) == flavor)
            return level;
    return -1;
}

void Core::setBackgroundRenderingColor(const QColor &color)
{
    m_backgroundRenderingColor = color;
}

QColor Core::backgroundRenderingColor() const
{
    return m_backgroundRenderingColor;
}

void Core::setVectorGraphicsRenderingSize(const QSize &size)
{
    m_vectorGraphicsRenderingSize = size;
}

QSize Core::vectorGraphicsRenderingSize() const
{
    return m_vectorGraphicsRenderingSize;
}

void Core::activateDBusThumbnailer()
{
    Logger::log("[Core]"+QString(Q_FUNC_INFO));
    if (m_dBusThumbnailer->isRunning())
        return;

    for (int level=0; level<=previewLevelCount()-1; level++)
        foreach (File *file, existingFiles()){
            if (file->exists() &&
                !file->supported() &&
                file->thumbnailSupported() &&
                (level <= file->displayLevel()) &&
                !thumbnailDirectory(level).isNull() &&
                file->stack() &&
                file->stack()->image(level).isNull() &&
                !file->hasThumbnail(level) &&
                m_dBusThumbnailer->supports(file->fileFormat())) {

                QString flavor = flavorFromLevel(level);

                Logger::log("[Core] Requesting thumbnail from D-Bus thumbnailer for "+ file->fileName() + " Mime type " + file->fileFormat() + " Flavor " + flavor);

                m_dBusThumbnailer->newThumbnailerTask(file->fileName(),
                                                      file->fileFormat(),
                                                      flavor);
                return;
                }
        }
}

void Core::processDBusThumbnailerGenerated(const QString fileName,
                                           const QString flavor)
{
    Logger::log("[Core] D-Bus thumbnailer finished with "+ fileName);
    int level = levelFromFlavor(flavor);
    if (!file(fileName, "")->hasThumbnail(level))
        processDBusThumbnailerError(fileName, -1, "No thumbnail found");

    suggestNewTask();
}

void Core::processDBusThumbnailerError(const QString fileName,
                                       uint errorCode,
                                       const QString message)
{
    Q_UNUSED(errorCode);
    Q_UNUSED(message);

    Logger::log("[Core] D-Bus thumbnailer error with "+ fileName);

    file(fileName, "")->emitError(QuillError(QuillError::FileFormatUnsupportedError,
                                             QuillError::ImageFileErrorSource,
                                             fileName));

    // Do not try to do any more thumbnailing for this file.
    file(fileName, "")->setThumbnailSupported(false);
}

void Core::emitSaved(QString fileName)
{
    emit saved(fileName);
    Logger::log("[Core] "+QString(Q_FUNC_INFO)+fileName);
}

void Core::emitRemoved(QString fileName)
{
    emit removed(fileName);
    Logger::log("[Core] "+QString(Q_FUNC_INFO)+fileName);
}

void Core::emitError(QuillError quillError)
{
    emit error(quillError);
    Logger::log("[Core] "+QString(Q_FUNC_INFO)+QString(" code")+Logger::intToString((int)(quillError.errorCode()))+QString(" source")+Logger::intToString((int)(quillError.errorSource()))+QString(" data:")+quillError.errorData());
}
