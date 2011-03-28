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
#include <limits.h>
#include <QTemporaryFile>
#include <QFileInfo>
#include <QUrl>
#include <QCryptographicHash>
#include <QList>
#include <QDir>
#include <QuillMetadata>
#include "file.h"
#include "core.h"
#include "imagecache.h"
#include "quillundocommand.h"
#include "historyxml.h"
#include "tilemap.h"
#include "filesystem.h"
#include "quillerror.h"
#include "logger.h"
#include "strings.h"

File::File() : m_state(State_Normal),
               m_hasThumbnailError(false),
               m_displayLevel(-1), m_priority(QuillFile::Priority_Normal),
               m_hasThumbnail(Thumbnail_UnknownExists), m_fileName(""), m_originalFileName(""),
               m_fileFormat(""), m_targetFormat(""), m_viewPort(QRect()),
               m_temporaryFile(0),m_original(false),
               m_hasReadEditHistory(false),m_fileIndexName(""),
               m_error(QuillError::NoError)
{
    m_stack = new QuillUndoStack(this);
}

File::~File()
{
        //Trying to lower the display level to purge the cached images
        setDisplayLevel(-1);
        detach();
        delete m_stack;
        delete m_temporaryFile;
}

void File::addReference(QuillFile *file)
{
    if (!m_references.contains(file))
        m_references.append(file);
}

void File::removeReference(QuillFile *file)
{
    if (m_references.contains(file)) {
        m_references.removeOne(file);
        //Trying to lower the display level to purge the cached images
        setDisplayLevel(-1);
        // Recalculate priority from remaining references
        calculatePriority();
        if (allowDelete()) {
            detach();
        }
    }
}

bool File::allowDelete()
{
    return m_references.isEmpty() && !isSaveInProgress()
        && !hasUnsavedThumbnails();
}

void File::detach()
{
    Core::instance()->detach(this);
    foreach (QuillFile *file, m_references)
        file->invalidate();
}

QString File::fileName() const
{
    return m_fileName;
}

QString File::fileFormat() const
{
    return m_fileFormat;
}

bool File::isJpeg() const
{
    return m_fileFormat == Strings::jpeg
        || m_fileFormat == Strings::jpg
        || m_fileFormat == Strings::jpegMimeType;
}

bool File::isSvg() const
{
    return m_fileFormat == Strings::svgMimeType;
}

QString File::originalFileName() const
{
    return m_originalFileName;
}

QString File::targetFormat() const
{
    return m_targetFormat;
}

void File::setFileName(const QString &fileName)
{
    m_fileName = fileName;

    QFileInfo info(fileName);
    m_originalFileName = info.path() + Strings::slashOriginal + info.fileName();

    if (!info.exists()) {
        m_error = QuillError(QuillError::FileNotFoundError,
                             QuillError::ImageFileErrorSource,
                             fileName);
        setExists(false);
    }
    else if (!(info.permissions() & QFile::WriteUser))
        setReadOnly();

    m_lastModified = info.lastModified();
}

void File::setFileFormat(const QString &fileFormat)
{
    m_fileFormat = fileFormat;
}

void File::setOriginalFileName(const QString &originalFileName)
{
    m_originalFileName = originalFileName;
}

void File::setTargetFormat(const QString &targetFormat)
{
    m_targetFormat = targetFormat;
}

void File::setReadOnly()
{
    if (state() != State_Placeholder)
        setState(State_ReadOnly);
}

bool File::isDisplayLevelEnabled(int level) const
{
    return Core::instance()->isSubstituteLevel(level, m_displayLevel);
}

bool File::setDisplayLevel(int level)
{
    int originalDisplayLevel = m_displayLevel;

    if (level < -1 || level > Core::instance()->previewLevelCount())
        return false;

    setDisplayLevelInternal(level);

    if (level > originalDisplayLevel)
            Core::instance()->suggestNewTask();

    return true;
}

void File::setDisplayLevelInternal(int level)
{
    int originalDisplayLevel = m_displayLevel;

    if (level <= originalDisplayLevel) {
        // If some other file object wants to keep the display level up
        foreach (QuillFile *file, m_references)
            if (file->displayLevel() > level)
                level = file->displayLevel();
    }

    // Purge images from cache if lowering display level here
    // Exception: when save is in progress, leave the highest level
    for (int l=originalDisplayLevel; l>level; l--)
        if ((l < Core::instance()->previewLevelCount()) ||
            (!isSaveInProgress()))
            // Hack: level 0 thumbs are not removed if they can be saved
            if ((l > 0) || !hasUnsavedThumbnails())
                Core::instance()->cache(l)->purge(this);

    m_displayLevel = level;

    // setup stack here
    if (m_stack->isClean() && (state() != State_NonExistent))
        m_stack->load();
}

int File::displayLevel() const
{
    return m_displayLevel;
}

void File::calculatePriority()
{
    int priority = INT_MIN;
    foreach (QuillFile *file, m_references)
        if (file->priority() > priority)
            priority = file->priority();
    m_priority = priority;
}

int File::priority() const
{
    return m_priority;
}

void File::save()
{
    if ((state() == State_Normal) && isDirty())
    {
        prepareSave();
        Core::instance()->suggestNewTask();
    }
}

bool File::isSaveInProgress() const
{
    return state() == State_Saving;
}

bool File::isDirty() const
{
    if (m_stack)
        return m_stack->isDirty();
    else
        return false;
}

void File::runFilter(QuillImageFilter *filter)
{
    if (!supportsEditing()) {
        delete filter;
        return;
    }

    if (m_stack->isClean())
        m_stack->load();
    m_stack->add(filter);

    abortSave();
    Core::instance()->suggestNewTask();
}

void File::startSession()
{
    if (supportsEditing())
        m_stack->startSession();
}

void File::endSession()
{
    if (supportsEditing())
        m_stack->endSession();
}

bool File::isSession() const
{
    return ((m_stack) && (m_stack->isSession()));
}

bool File::canUndo() const
{
    if (!supportsEditing())
        return false;
    return m_stack->canUndo();
}

void File::undo()
{
    if (canUndo())
    {
        m_stack->undo();

        abortSave();
        Core::instance()->suggestNewTask();

        emitAllImages();
    }
}

bool File::canRedo() const
{
    if (!supportsEditing())
        return false;

    return m_stack->canRedo();
}

void File::redo()
{
    if (canRedo())
    {
        m_stack->redo();
        abortSave();
        Core::instance()->suggestNewTask();

        emitAllImages();
    }
}

void File::dropRedoHistory()
{
    if (canRedo())
        m_stack->dropRedoHistory();
}

QuillImage File::bestImage(int displayLevel) const
{
    if (!exists())
        return QuillImage();
    return m_stack->bestImage(displayLevel);
}

QuillImage File::image(int level) const
{
    if (!exists())
        return QuillImage();
    return m_stack->image(level);
}

void File::setImage(int level, const QuillImage &image)
{
    m_error = QuillError::NoError;
    setState(State_Placeholder);

    for (int l=0; l<=m_displayLevel; l++)
        Core::instance()->cache(l)->purge(this);

    // Only complete images are approved. Input images are made into such.
    QuillImage fixedImage(image);
    fixedImage.setArea(QRect(QPoint(0, 0), image.fullImageSize()));

    setDisplayLevelInternal(level);

    // Initialize stack for nonexistent files
    if (m_stack->isClean())
        m_stack->load();
    m_stack->setImage(level,
                      QuillImage(fixedImage,
                                 image.convertToFormat(QImage::Format_RGB32)));

    Core::instance()->suggestNewTask();
}

QList<QuillImage> File::allImageLevels(int displayLevel) const
{
    if (!exists() || !m_stack->command())
        return QList<QuillImage>();
    else if ((displayLevel < Core::instance()->previewLevelCount()) ||
             (!m_stack->command()->tileMap()))
        return m_stack->allImageLevels(displayLevel);
    else
        return m_stack->allImageLevels(displayLevel) +
            m_stack->command()->tileMap()->nonEmptyTiles(m_viewPort);
}

QSize File::fullImageSize() const
{
    if (!exists())
        return QSize();

    QSize size = m_stack->fullImageSize();

    if (size.isValid())
        return size;
    else if (!size.isValid() && supportsViewing() && m_stack->command())
        m_stack->calculateFullImageSize(m_stack->command());

    return m_stack->fullImageSize();
}

void File::setViewPort(const QRect &viewPort)
{
    const QRect oldPort = m_viewPort;
    m_viewPort = viewPort;

    // New tiles will only be calculated if the display level allows it
    if (!supportsViewing() ||
        (m_displayLevel < Core::instance()->previewLevelCount()))
        return;

    Core::instance()->suggestNewTask();

    QList<QuillImage> newTiles;

    if ((m_stack->command()) && (m_stack->command()->tileMap()))
        newTiles = m_stack->command()->tileMap()->
        newTiles(oldPort, viewPort);

    if (!newTiles.isEmpty())
        emitTiles(newTiles);
}

QRect File::viewPort() const
{
    return m_viewPort;
}

bool File::checkImageSize(const QSize &fullImageSize)
{
    const QSize imageSizeLimit = Core::instance()->imageSizeLimit();

    if (isSvg()) // Vector graphics are always rendered to a fixed size
        return true;

    if (imageSizeLimit.isValid() &&
        (fullImageSize.boundedTo(imageSizeLimit) != fullImageSize))
        return false;

    int imagePixelsLimit = 0;
    if (!isJpeg())
        imagePixelsLimit = Core::instance()->nonTiledImagePixelsLimit();

    if (imagePixelsLimit == 0)
        imagePixelsLimit = Core::instance()->imagePixelsLimit();

    if ((imagePixelsLimit > 0) &&
        (fullImageSize.width() * fullImageSize.height() > imagePixelsLimit))
        return false;
    else
        return true;
}

QuillUndoStack *File::stack() const
{
    return m_stack;
}

bool File::hasUnsavedThumbnails()
{
    return (m_stack && m_stack->hasImage(0) &&
            !Core::instance()->thumbnailFlavorName(0).isEmpty() &&
            !hasThumbnail(0) && Core::instance()->isThumbnailCreationEnabled());
}

bool File::hasThumbnail(int level)
{
    if(isOriginal()){
        return false;
    }
    if (Core::instance()->thumbnailFlavorName(level).isEmpty()) {
        return false;
    }
    // Optimization currently for level 0 only
    // For externally supported files, thumbnails may appear at any time
    // so they need to be always checked from the file system
    if ((level == 0) &&
        (state() != State_ExternallySupportedFormat) &&
        (m_hasThumbnail != File::Thumbnail_UnknownExists)) {
         return m_hasThumbnail == File::Thumbnail_Exists;
    }

    // Information about thumbnail existence not known, it must be calculated now
    QFileInfo info(thumbnailFileName(level));

    File::ThumbnailExistenceState result = File::Thumbnail_NotExists;

    if (info.exists() && (info.lastModified() == m_lastModified))
        result = File::Thumbnail_Exists;

    if (level == 0)
        m_hasThumbnail = result;

    return result == File::Thumbnail_Exists;
}

QString File::fileNameHash(const QString &fileName)
{
    QFileInfo info(fileName);
    // canonicalFilePath() will not work unless the file exists
    // Here we only assume that the directory exists
    const QUrl uri = QUrl::fromLocalFile(info.dir().canonicalPath() + "/"
                            + info.fileName());

    const QByteArray hashValue =
        QCryptographicHash::hash(uri.toString().toLatin1(),
                                 QCryptographicHash::Md5);

    return hashValue.toHex();
}

QString File::thumbnailFileName(int level)
{
    if(isOriginal())
        return QString();
    if (m_fileNameHash.isEmpty())
        m_fileNameHash = fileNameHash(m_fileName);

    QString hashValueString = m_fileNameHash;
    hashValueString.append("." + Core::instance()->thumbnailExtension());
    hashValueString.prepend(Core::instance()->thumbnailDirectory(level) +
                            QDir::separator());

    return hashValueString;
}

QString File::editHistoryFileName(const QString &fileName,
                                  const QString &editHistoryPath)
{
    QString hashValueString = fileNameHash(fileName);
    hashValueString.append(".xml");
    hashValueString.prepend(editHistoryPath + QDir::separator());

    return hashValueString;
}

void File::readFromEditHistory(const QString &fileName,
                                const QString &originalFileName,
                                QuillError *error)
{
    // If original is not found, we will ignore any edit history.
    if (!QFile::exists(originalFileName)) {
        *error = QuillError(QuillError::FileNotFoundError,
                            QuillError::ImageOriginalErrorSource,
                            originalFileName);
        return;
    }

    QFile file(editHistoryFileName(fileName,
                                   Core::instance()->editHistoryPath()));

    QUILL_LOG(Logger::Module_File,
                "Reading edit history from "+file.fileName());

    if (!file.exists()) {
        *error = QuillError(QuillError::FileNotFoundError,
                            QuillError::EditHistoryErrorSource,
                            file.fileName());
        return;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        *error = QuillError(QuillError::FileOpenForReadError,
                            QuillError::EditHistoryErrorSource,
                            file.fileName());
        return;
    }
    const QByteArray history = file.readAll();
    if (history.isEmpty()) {
        *error = QuillError(QuillError::FileReadError,
                            QuillError::EditHistoryErrorSource,
                            file.fileName());
        return;
    }
    file.close();

    // If a file is write protected, set the state to read-only.
    bool readOnly = false;

    if (!file.open(QIODevice::ReadWrite)) {
        *error = QuillError(QuillError::FileOpenForWriteError,
                            QuillError::EditHistoryErrorSource,
                            file.fileName());
        readOnly = true;
    }
    file.close();
    if(readOnly)
        setReadOnly();
    QUILL_LOG(Logger::Module_File,
                "Edit history size is "+QString::number(history.size())+" bytes");
    QUILL_LOG(Logger::Module_File,"Edit history dump: "+history);

    if(!HistoryXml::decodeOne(history,this)){
        *error = QuillError(QuillError::FileCorruptError,
                            QuillError::EditHistoryErrorSource,
                            file.fileName());
    }
}

void File::writeEditHistory(const QString &history, QuillError *error)
{
    if (!QDir().mkpath(Core::instance()->editHistoryPath())) {
        *error = QuillError(QuillError::DirCreateError,
                            QuillError::EditHistoryErrorSource,
                            Core::instance()->editHistoryPath());
        return;
    }
    QFile file(editHistoryFileName(m_fileName,
                                   Core::instance()->editHistoryPath()));

    QUILL_LOG(Logger::Module_File,"Writing edit history to "+file.fileName());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        *error = QuillError(QuillError::FileOpenForWriteError,
                            QuillError::EditHistoryErrorSource,
                            file.fileName());
        return;
    }
    qint64 fileSize = file.write(history.toAscii());
    if(fileSize == -1) {
        *error = QuillError(QuillError::FileWriteError,
                            QuillError::EditHistoryErrorSource,
                            file.fileName());
        return;
    }
    file.close();
}

void File::emitSingleImage(QuillImage image, int level)
{
    foreach(QuillFile *file, m_references)
        if (Core::instance()->isSubstituteLevel(level, file->displayLevel()))
            file->emitImageAvailable(image);
}

void File::emitTiles(QList<QuillImage> tiles)
{
    foreach(QuillFile *file, m_references)
        if (file->displayLevel() == Core::instance()->previewLevelCount())
            file->emitImageAvailable(tiles);
}

void File::emitAllImages()
{
    foreach(QuillFile *file, m_references) {
        QList<QuillImage> allImages = allImageLevels(file->displayLevel());
        if (!allImages.isEmpty())
            file->emitImageAvailable(allImages);
    }
}

void File::remove()
{
    if (state() == State_NonExistent)
        return;

    QFile(m_fileName).remove();
    QFile(m_originalFileName).remove();
    QFile::remove(editHistoryFileName(m_fileName,
                                      Core::instance()->editHistoryPath()));
    removeThumbnails();

    abortSave();
    setState(State_NonExistent);

    delete m_stack;
    // A file always needs a stack.
    m_stack = new QuillUndoStack(this);

    if (hasOriginal())
        original()->remove();

    Core::instance()->emitRemoved(m_fileName);
    emit removed();
}

bool File::exists() const
{
    return (state() != State_NonExistent);
}

void File::setExists(bool exists)
{
    if (!exists)
        setState(State_NonExistent);
    else if (state() == State_NonExistent)
        setState(State_Normal);
}

QDateTime File::lastModified() const
{
    return m_lastModified;
}

void File::refreshLastModified()
{
    m_lastModified = QFileInfo(m_fileName).lastModified();
}

void File::setSupported(bool supported)
{
    if (supported && (state() == State_UnsupportedFormat))
        setState(State_Normal);
    else if (!supported)
        setState(State_UnsupportedFormat);
}

void File::setThumbnailSupported(bool supported)
{
    if (supported && (state() == State_UnsupportedFormat))
        setState(State_ExternallySupportedFormat);
    else if (!supported)
        setState(State_UnsupportedFormat);
}

QuillError File::overwritingCopy(const QString &fileName,
                                 const QString &newName)
{
    // Uses NoErrorSource as an error source since this lower-level
    // function does not know of error sources in a larger context.

    if (!QDir().mkpath(QFileInfo(newName).path()))
        return QuillError(QuillError::DirCreateError,
                          QuillError::NoErrorSource,
                          QFileInfo(newName).path());

    QFile source(fileName),
        target(newName);

    if (!source.open(QIODevice::ReadOnly))
        return QuillError(QuillError::FileOpenForReadError,
                          QuillError::NoErrorSource,
                          fileName);

    const QByteArray buffer = source.readAll();
    if (buffer.isEmpty())
        return QuillError(QuillError::FileReadError,
                          QuillError::NoErrorSource,
                          fileName);

    source.close();

    if (!target.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return QuillError(QuillError::FileOpenForWriteError,
                          QuillError::NoErrorSource,
                          newName);

    qint64 fileSize = target.write(buffer);
    if (fileSize == -1)
        return QuillError(QuillError::FileWriteError,
                          QuillError::NoErrorSource,
                          newName);

    target.close();
    return QuillError();
}

void File::removeThumbnails()
{
    for (int level=0; level<Core::instance()->previewLevelCount(); level++)
        QFile::remove(thumbnailFileName(level));
    m_hasThumbnail = File::Thumbnail_NotExists;
}

void File::touchThumbnail(int level)
{
    FileSystem::setFileModificationDateTime(thumbnailFileName(level),
                                            m_lastModified);
}

void File::touchThumbnails()
{
    for (int level=0; level<Core::instance()->previewLevelCount(); level++)
        touchThumbnail(level);
}

void File::prepareSave()
{
    delete m_temporaryFile;

    // Save filter always operates with temporary files.

    QFileInfo info(m_fileName);

    // This guarantees that the temporary file will have the same
    // extension as the target file, so that the correct format can be
    // deduced by QImageReader.

    QString path = Core::instance()->temporaryFilePath();
    if (path.isNull())
        path = Strings::tempDirDefault;

    if (!QDir().mkpath(path)) {
        emitError(QuillError(QuillError::DirCreateError,
                             QuillError::TemporaryFileErrorSource,
                             path));
        return;
    }

    const QString filePath = path + QDir::separator()
                           + Strings::tempFilePattern
                           + info.fileName();

    m_temporaryFile = new QTemporaryFile(filePath);
    if (!m_temporaryFile) {
        emitError(QuillError(QuillError::FileOpenForReadError,
                             QuillError::TemporaryFileErrorSource,
                             filePath));
        return;
    }

    if (!m_temporaryFile->open()) {
        emitError(QuillError(QuillError::FileOpenForReadError,
                             QuillError::TemporaryFileErrorSource,
                             m_temporaryFile->fileName()));
        return;
    }

    QByteArray rawExifDump;
    if (isJpeg()) {
        QuillMetadata metadata(m_fileName, QuillMetadata::ExifFormat);
        ResetOrientationTag(metadata);
        rawExifDump = metadata.dump(QuillMetadata::ExifFormat);
    }

    m_stack->prepareSave(m_temporaryFile->fileName(), rawExifDump);

    setState(State_Saving);
}

void File::ResetOrientationTag(QuillMetadata &metadata)
{
    QVariant orientation = metadata.entry(QuillMetadata::Tag_Orientation);
    if (!orientation.isNull())
        metadata.setEntry(QuillMetadata::Tag_Orientation, QVariant(1));
}

void File::concludeSave()
{
    // If save is concluded, purge any full images still in memory.
    if (m_displayLevel < Core::instance()->previewLevelCount())
        Core::instance()->cache(Core::instance()->previewLevelCount())->purge(this);

    const QString temporaryName = m_temporaryFile->fileName();
    m_temporaryFile->close();

    QFile file(m_originalFileName);

    // Original file does not exist - backup current into original
    // before replacing with contents of temp file.

    if ((!file.exists()) || (!file.size() > 0)) {
        QuillError result = File::overwritingCopy(m_fileName,
                                                  m_originalFileName);
        if (result.errorCode() != QuillError::NoError) {
            result.setErrorSource(QuillError::ImageOriginalErrorSource);
            emitError(result);
            goto cleanup;
        }
    }

    // Copy metadata from previous version to new one

    if (isJpeg()) {
        QuillMetadata metadata(m_fileName);
        ResetOrientationTag(metadata);
        if (!metadata.write(temporaryName, QuillMetadata::XmpFormat)) {
            // If metadata write failed, the temp file is likely corrupt
            emitError(QuillError(QuillError::FileWriteError,
                                 QuillError::TemporaryFileErrorSource,
                                 m_fileName));
            goto cleanup;
        }
    }

    // This is more efficient than renaming between partitions.

    {
        QuillError result = File::overwritingCopy(temporaryName,
                                                  m_fileName);
        if (result.errorCode() != QuillError::NoError) {
            result.setErrorSource(QuillError::ImageFileErrorSource);
            emitError(result);
            goto cleanup;
        }

        m_stack->concludeSave();
        if (hasOriginal())
            original()->stack()->concludeSave();

        writeEditHistory(HistoryXml::encode(this), &result);

        if (result.errorCode() != QuillError::NoError) {
            emitError(result);
            goto cleanup;
        }
    }

    removeThumbnails();
    refreshLastModified();

    emit saved();
    Core::instance()->emitSaved(m_fileName);

 cleanup:
    setState(State_Normal);

    QFile::remove(temporaryName);

    delete m_temporaryFile;
    m_temporaryFile = 0;
}

void File::abortSave()
{
    m_stack->abortSave();
    delete m_temporaryFile;
    m_temporaryFile = 0;
    setState(State_Normal);
}

void File::registerThumbnail(int level)
{
    if (level == 0)
        m_hasThumbnail = File::Thumbnail_Exists;
    touchThumbnail(level);
}

bool File::hasOriginal()
{
    QString indexName = QString('\\') + m_fileName;
    return Core::instance()->fileExists(indexName);
}

File *File::original()
{
    QString indexName = QString('\\') + m_fileName;

    if (Core::instance()->fileExists(indexName))
        return Core::instance()->file(indexName, "");

    File *original = new File();
    original->setFileName(m_fileName);
    original->setFileFormat(m_fileFormat);
    original->setOriginalFileName(m_originalFileName);
    original->setReadOnly();
    //We set a flag for the original file, then we can distinguish original one from edited one
    original->setOriginal(true);
    original->setFileIndexName(indexName);
    Core::instance()->insertFile(original);
    return original;
}

void File::processFilterError(QuillImageFilter *filter)
{
    if (filter->role() == QuillImageFilter::Role_Load) {
        QuillError::ErrorCode errorCode =
            QuillError::translateFilterError(filter->error());

        QuillError::ErrorSource errorSource;

        if (filter->option(QuillImageFilter::FileName).toString() ==
            m_fileName) {
            errorSource = QuillError::ImageFileErrorSource;
            if ((errorCode == QuillError::FileFormatUnsupportedError) ||
                (errorCode == QuillError::FileCorruptError)) {
                setSupported(false);
                if (Core::instance()->isDBusThumbnailingEnabled() &&
                    Core::instance()->isExternallySupportedFormat(m_fileFormat)) {
                    setThumbnailSupported(true);
                    // Not emitting an error yet, as D-Bus thumbnailer might
                    // still find an use for the file
                    return;
                }
                // Corner-case: thumbnailer is disabled but thumbnails do already exist
                else if (!Core::instance()->isDBusThumbnailingEnabled() &&
                             Core::instance()->isExternallySupportedFormat(m_fileFormat) &&
                             hasThumbnail(displayLevel())) {
                     setThumbnailSupported(true);
                     return;
                }
            }
            else
                setExists(false);
        }
        else {
            errorSource = QuillError::ImageOriginalErrorSource;
        }

        emitError(QuillError(errorCode, errorSource,
                             filter->option(QuillImageFilter::FileName).toString()));
    }
}

void File::imageSizeError()
{
    emitError(QuillError(QuillError::ImageSizeLimitError,
                         QuillError::ImageFileErrorSource,
                         m_fileName));
    setState(State_UnsupportedFormat);
}

void File::refresh()
{
    // Drop any previous error state
    m_error = QuillError::NoError;

    // Forces re-checking thumbnails existence
    m_hasThumbnail = File::Thumbnail_UnknownExists;

    refreshLastModified();

    for (int l=0; l<=m_displayLevel; l++)
        Core::instance()->cache(l)->purge(this);

    setState(State_Normal);

    m_stack->refresh();

    Core::instance()->suggestNewTask();
}

bool File::isWaitingForData() const
{
    return state() == State_Placeholder;
}

void File::setThumbnailError(bool status)
{
    m_hasThumbnailError = status;
}

bool File::hasThumbnailError() const
{
    return m_hasThumbnailError;
}

void File::emitError(QuillError quillError)
{
    m_error = quillError;
    emit error(quillError);
    Core::instance()->emitError(quillError);
    QUILL_LOG(Logger::Module_File,QString(Q_FUNC_INFO)+QString(" code")+Logger::intToString((int)(quillError.errorCode()))+QString(" source")+Logger::intToString((int)(quillError.errorSource()))+QString(" data:")+quillError.errorData());
}

bool File::canRevert() const
{
    if (!supportsEditing())
        return false;
    return m_stack->canRevert();
}

void File::revert()
{
    if (canRevert()){
        m_stack->revert();
        abortSave();
        Core::instance()->suggestNewTask();
        emitAllImages();
    }
}

bool File::canRestore() const
{
    if (!supportsEditing())
        return false;

    return m_stack->canRestore();
}

void File::restore()
{
    if(canRestore()){
        m_stack->restore();
        abortSave();
        Core::instance()->suggestNewTask();
        emitAllImages();
    }
}

File::State File::state() const
{
    return m_state;
}

void File::setState(File::State state)
{
    m_state = state;
}

bool File::supportsThumbnails() const
{
    return ((m_state != State_NonExistent) &&
            (m_state != State_UnsupportedFormat));
}

bool File::supportsViewing() const
{
    return ((m_state != State_NonExistent) &&
            (m_state != State_UnsupportedFormat) &&
            (m_state != State_ExternallySupportedFormat));
}

bool File::supportsEditing() const
{
    // this needs to be called to determine if the file has a read only format
    if (m_stack->isClean()&&m_hasReadEditHistory)
        m_stack->load();

    //if it supports editting, we read the edit history
    if((m_state != State_NonExistent) &&
       (m_state != State_UnsupportedFormat) &&
       (m_state != State_ExternallySupportedFormat) &&
       (m_state != State_ReadOnly)){
        if(!m_hasReadEditHistory&&!m_original&&(m_state != State_Placeholder)){
            const_cast<File *>(this)->m_hasReadEditHistory = true;
            QuillError error;
            const_cast<File *>(this)->readFromEditHistory(m_fileName,m_originalFileName,&error);
            if ((error.errorCode() != QuillError::NoError) &&
                (error.errorCode() != QuillError::FileNotFoundError)){
                Core::instance()->emitError(error);
                return false;
            }

            // No edit history was found, re-setup the stack
            if (m_stack->isClean()){
                m_stack->load();
            }

            if ((m_state == State_NonExistent) ||
                (m_state == State_UnsupportedFormat) ||
                (m_state == State_ExternallySupportedFormat) ||
                (m_state == State_ReadOnly))
                return false;
        }
        return true;
    }
    else
        return false;
}

bool File::isOriginal() const
{
    return m_original;
}

void File::setOriginal(bool flag)
{
    m_original = flag;
}

bool File::readEditHistory() const
{
    return m_hasReadEditHistory;
}

void File::setFileIndexName(const QString indexName)
{
    m_fileIndexName = indexName;
}

QString File::fileIndexName() const
{
    if(isOriginal())
        return m_fileIndexName;
    else
        return m_fileName;
}

QuillError File::error() const
{
    return m_error;
}

