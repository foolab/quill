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

#include <QuillImage>
#include <QuillImageFilter>
#include <QuillImageFilterGenerator>
#include <QImageWriter>
#include <QDir>
#include <QEventLoop>
#include <QTimer>
#include "quill.h"
#include "quillerror.h"
#include "core.h"
#include "displaylevel.h"
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
#include "strings.h"
#include "unix_platform.h"

Core *Core::g_instance = 0;

Core::Core(Quill::ThreadingMode threadingMode) :
    m_imageSizeLimit(QSize()),
    m_imagePixelsLimit(0),
    m_nonTiledImagePixelsLimit(0),
    m_editHistoryPath(QDir::homePath() + Strings::historyPath),
    m_thumbnailBasePath(QDir::homePath() + Strings::thumbsBasePath),
    m_thumbnailCreationEnabled(true),
    m_dBusThumbnailingEnabled(true),
    m_saveBufferSize(65536*16),
    m_tileCache(new TileCache(100)),
    m_scheduler(new Scheduler()),
    m_threadManager(new ThreadManager(threadingMode)),
    m_temporaryFilePath(QString())
{
    DisplayLevel *previewLevel = new DisplayLevel(Quill::defaultViewPortSize);
    m_displayLevel.append(previewLevel);

    DisplayLevel *fullLevel = new DisplayLevel(Quill::defaultViewPortSize * 2);
    m_displayLevel.append(fullLevel);

    qRegisterMetaType<QuillImageList>("QuillImageList");
    qRegisterMetaType<QuillError>("QuillError");

    m_dBusThumbnailer = new DBusThumbnailer;
    connect(m_dBusThumbnailer,
            SIGNAL(thumbnailGenerated(const QString, const QString)),
            SLOT(processDBusThumbnailerGenerated(const QString, const QString)));

    connect(m_dBusThumbnailer,
            SIGNAL(thumbnailError(const QString, uint, const QString)),
            SLOT(processDBusThumbnailerError(const QString, uint, const QString)));

    m_writableImageFormats = QImageWriter::supportedImageFormats();
    m_fileList.clear();
}

Core::~Core()
{
    foreach(File *file, m_fileList)
        delete file;
    while (!m_displayLevel.isEmpty()) {
        delete m_displayLevel.first();
        m_displayLevel.removeFirst();
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
    QuillImageFilter::cleanup();
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

    if (!m_fileList.isEmpty() || (count <= 0))
        return;
    int oldCount = m_displayLevel.count()-1;
    if (count > oldCount)
        for (int i = oldCount; i < count; i++) {
            m_displayLevel.append(
              new DisplayLevel(m_displayLevel.last()->size() * 2));
        }
    else if (count < oldCount)
        for (int i = oldCount; i > count; i--) {
            delete m_displayLevel.last();
            m_displayLevel.removeLast();
        }
}

int Core::previewLevelCount() const
{
    return m_displayLevel.count()-1;
}

ImageCache *Core::cache(int level) const
{
    return m_displayLevel[level]->imageCache();
}

void Core::setPreviewSize(int level, const QSize &size)
{
    if ((level < 0) || (level >= m_displayLevel.count()-1))
        return;

    m_displayLevel[level]->setSize(size);
    if (level == m_displayLevel.count()-2)
        m_displayLevel[level+1]->setSize(size*2);
}

QSize Core::previewSize(int level) const
{
    if ((level >=0 ) && (level < m_displayLevel.count()-1))
        return m_displayLevel[level]->size();
    else
        return QSize();
}

void Core::setMinimumPreviewSize(int level, const QSize &size)
{
    if ((level < 0) || (level >= m_displayLevel.count()-1))
        return;

    m_displayLevel[level]->setMinimumSize(size);
}

QSize Core::minimumPreviewSize(int level) const
{
    if ((level >=0 ) && (level < m_displayLevel.count()-1))
        return m_displayLevel[level]->minimumSize();
    else
        return QSize();
}

bool Core::isSubstituteLevel(int level, int targetLevel) const
{
    return (level == targetLevel) ||
        ((level < targetLevel) &&
          !minimumPreviewSize(level).isValid() &&
          !minimumPreviewSize(targetLevel).isValid());
}

int Core::smallestNonCroppedLevel() const
{
    for (int level=0; level < m_displayLevel.count() - 1; level++)
        if (!minimumPreviewSize(level).isValid())
            return level;
    return m_displayLevel.count();
}

int Core::smallestCachedLevel() const
{
    for (int level=0; level < m_displayLevel.count() - 1; level++)
        if (!thumbnailFlavorName(level).isEmpty())
            return level;
    return -1;
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
    if ((level < 0) || (level >= m_displayLevel.count()))
        return;

    m_displayLevel[level]->imageCache()->setMaxSize(limit);
}

int Core::editHistoryCacheSize(int level)
{
    if ((level < 0) || (level >= m_displayLevel.count()))
        return 0;

    return m_displayLevel[level]->imageCache()->maxSize();
}

bool Core::fileExists(const QString &fileName)
{
    foreach(File* file, m_fileList){
        if(file->fileIndexName()==fileName)
            return true;
    }

    return false;
}

File *Core::file(const QString &fileName,
                 const QString &fileFormat)
{
    File* file = 0;

    foreach(File* file, m_fileList){
        if(file->fileIndexName()==fileName)
            return file;
    }
    file = new File();
    // also sets the original file name
    file->setFileName(fileName);

    file->setFileFormat(fileFormat);
    file->setTargetFormat(fileFormat);

    m_fileList.append(file);
    return file;
}

void Core::attach(File *file)
{
    m_fileList.append(file);
}

void Core::detach(File *file)
{
    m_fileList.removeOne(file);
}

QuillUndoCommand *Core::findInAllStacks(int id) const
{
    QuillUndoCommand *command = 0;

    foreach(File* file, m_fileList){
        if (file->exists()) {
            command = file->stack()->find(id);
            if (command)
                break;
        }
    }
    return command;
}

File *Core::priorityFile() const
{
    QString name;
    int level = -1;
    File *priorityFile = 0;

    foreach(File* file, m_fileList){
        if (file->supportsViewing() &&
            (file->displayLevel() > level)) {
            priorityFile = file;
            level = file->displayLevel();
        }
    }
    return priorityFile;
}

File *Core::prioritySaveFile() const
{
    foreach(File* file, m_fileList){
        if (file->isSaveInProgress())
            return file;
    }

    return 0;
}

void Core::suggestNewTask()
{
    // Make sure that nothing is already running on the background.

    if (!m_threadManager->isRunning()) {

        Task *task = m_scheduler->newTask();

        if (task)
            m_threadManager->run(task);
    }

    // The D-Bus thumbnailer runs on a different process instead of
    // Quill's background thread so it can always be invoked.
    if (m_dBusThumbnailingEnabled)
        activateDBusThumbnailer();
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

void Core::setEditHistoryPath(const QString &path)
{
    m_editHistoryPath = path;
}

QString Core::editHistoryPath() const
{
    return m_editHistoryPath;
}

void Core::setThumbnailBasePath(const QString &path)
{
    m_thumbnailBasePath = path;
}

QString Core::failedThumbnailPath()
{
    return m_thumbnailBasePath + Strings::thumbsFail;
}

void Core::setThumbnailFlavorName(int level, const QString &name)
{
    m_displayLevel[level]->setThumbnailFlavorName(name);
}

QString Core::thumbnailFlavorName(int level) const
{
    if ((level < 0) || (level >= m_displayLevel.count()))
        return QString();
    else
        return m_displayLevel[level]->thumbnailFlavorName();
}

QString Core::thumbnailPath(int level) const
{
    if ((level < 0) || (level >= m_displayLevel.count()))
        return QString();

    const QString flavorName = m_displayLevel[level]->thumbnailFlavorName();

    if (flavorName.isEmpty())
        return QString();

    return m_thumbnailBasePath + Strings::slash + flavorName;
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

void Core::insertFile(File *file)
{
    m_fileList.append(file);
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

int Core::numFilesAtLevel(int level) const
{
    int n = 0;

    foreach(File* file, m_fileList){
        if (file->displayLevel() >= level)
            n++;
    }
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

QStringList Core::saveInProgressList() const
{
    QStringList stringList;
    foreach(File* file, m_fileList){
        if (file->isSaveInProgress())
            stringList.append(file->fileName());
    }

    return stringList;
}

QStringList Core::lockedFiles()
{
    return LockFile::lockedFiles();
}

void Core::timeout()
{
    m_loop.exit(1);
}

bool Core::waitUntilFinished(int msec)
{
    if (msec > 0)
        QTimer::singleShot(msec, this, SLOT(timeout()));

    QObject::connect(this, SIGNAL(saved(QString)),
                     &m_loop, SLOT(quit()));
    QObject::connect(this, SIGNAL(error(QuillError)),
                     &m_loop, SLOT(quit()));

    while (isSaveInProgress())
        if (m_loop.exec()) {
            QUILL_LOG(Logger::Module_Core,
                        "WaitUntilFinished: interrupted by timeout");
            return false;
        }

    QUILL_LOG(Logger::Module_Core, "WaitUntilFinished: success");
    return true;
}

void Core::setTemporaryFilePath(const QString &filePath)
{
    m_temporaryFilePath = filePath;
}

QString Core::temporaryFilePath() const
{
    return m_temporaryFilePath;
}

int Core::levelFromFlavor(QString flavor)
{
    for (int level=0; level<=previewLevelCount()-1; level++)
        if (thumbnailFlavorName(level) == flavor)
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

QList<QByteArray> Core::writableImageFormats()
{
    //we read the Qt supported image formats from constructor
    return m_writableImageFormats;
}

QSize Core::targetSizeForLevel(int level, const QSize &fullImageSize)
{
    if ((level >=0 ) && (level < m_displayLevel.count()-1))
        return m_displayLevel[level]->targetSize(fullImageSize);
    else
        return QSize();
}

QRect Core::targetAreaForLevel(int level, const QSize &targetSize,
                               const QSize &fullImageSize)
{
    if ((level >=0 ) && (level < m_displayLevel.count()-1))
        return m_displayLevel[level]->targetArea(targetSize, fullImageSize);
    else
        return QRect();
}

bool Core::isExternallySupportedFormat(const QString &format) const
{
    return m_dBusThumbnailer->supports(format);
}

void Core::activateDBusThumbnailer()
{
    QUILL_LOG(Logger::Module_Core, QString(Q_FUNC_INFO));
    if (m_dBusThumbnailer->isRunning())
        return;
    for (int level=0; level<=previewLevelCount()-1; level++) {
        QString flavor = thumbnailFlavorName(level);
        if (!flavor.isNull())
            // using m_files instead of existingFiles() since this
            // will potentially be called often and the existing file
            // list creation is a slow task.
            foreach(File* file, m_fileList){
                if ((file->state() == File::State_ExternallySupportedFormat) &&
                    (level <= file->displayLevel()) &&
                    file->stack() &&
                    file->stack()->image(level).isNull() &&
                    !file->hasThumbnail(level) &&
                    m_dBusThumbnailer->supports(file->fileFormat())) {

                    QUILL_LOG(Logger::Module_Core, "Requesting thumbnail from D-Bus thumbnailer for "+ file->fileName() + " Mime type " + file->fileFormat() + " Flavor " + flavor);

                    m_dBusThumbnailer->newThumbnailerTask(file->fileName(),
                                                          file->fileFormat(),
                                                          flavor);
                    return;
                }
            }
    }
}

void Core::processDBusThumbnailerGenerated(const QString fileName,
                                           const QString flavor)
{
    QUILL_LOG(Logger::Module_Core, "D-Bus thumbnailer finished with "+ fileName);
    Q_UNUSED(flavor);
    int level = levelFromFlavor(flavor);

    // currently exists to offset a video thumbnailer problem
    if (level >= 0)
        file(fileName, "")->touchThumbnail(level);
    if ((level < 0) || (!file(fileName, "")->hasThumbnail(level)))
        processDBusThumbnailerError(fileName, -1, "No thumbnail found");
    suggestNewTask();
}

void Core::processDBusThumbnailerError(const QString fileName,
                                       uint errorCode,
                                       const QString message)
{
    Q_UNUSED(errorCode);
    Q_UNUSED(message);

    QUILL_LOG(Logger::Module_Core, "D-Bus thumbnailer error with "+ fileName);

    if(fileExists(fileName)){

        foreach(File* file, m_fileList)
            if(file->fileName()==fileName){
                file->emitError(QuillError(QuillError::FileFormatUnsupportedError,
                                           QuillError::ImageFileErrorSource,
                                           fileName));
                break;
            }
    }
}


void Core::emitSaved(QString fileName)
{
    emit saved(fileName);
    QUILL_LOG(Logger::Module_Core, QString(Q_FUNC_INFO)+fileName);
}

void Core::emitRemoved(QString fileName)
{
    emit removed(fileName);
    QUILL_LOG(Logger::Module_Core, QString(Q_FUNC_INFO)+fileName);
}

void Core::emitError(QuillError quillError)
{
    emit error(quillError);
    QUILL_LOG(Logger::Module_Core, QString(Q_FUNC_INFO)+QString(" code")+Logger::intToString((int)(quillError.errorCode()))+QString(" source")+Logger::intToString((int)(quillError.errorSource()))+QString(" data:")+quillError.errorData());
}

const QList<File*> Core::fileList() const
{
    return m_fileList;
}
