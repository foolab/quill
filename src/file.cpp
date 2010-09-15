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
#include "quillerror.h"
#include "logger.h"

File::File() : m_state(State_Normal),
               m_hasThumbnailError(false), m_isClone(false),
               m_displayLevel(-1), m_hasThumbnail(false),
               m_fileName(""), m_originalFileName(""),
               m_fileFormat(""), m_targetFormat(""), m_viewPort(QRect()),
               m_temporaryFile(0),m_original(false)
{
    m_stack = new QuillUndoStack(this);
}

File::~File()
{
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
        // Trying to lower the display level, remaining references will
        // prevent this from happening.
        setDisplayLevel(-1);
        // Recalculate priority from remaining references
        calculatePriority();
        if (m_references.isEmpty() && !isSaveInProgress()) {
            detach();
        }
    }
}

bool File::allowDelete()
{
    return m_references.isEmpty() && !isSaveInProgress();
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
    return m_fileFormat == "jpeg" || m_fileFormat == "jpg" ||
        m_fileFormat == "image/jpeg";
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
    if (!info.exists()) {
        emitError(QuillError(QuillError::FileNotFoundError,
                             QuillError::ImageFileErrorSource,
                             fileName));
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
            Core::instance()->cache(l)->purge(this);

    // Optimization: check thumbnail existence in file system here
    // and only here!
    if ((level >= 0) && (m_displayLevel < 0))
        m_hasThumbnail = hasThumbnail(0);

    m_displayLevel = level;

    // setup stack here
    if (m_stack->isClean() && (state() != State_NonExistent))
        m_stack->load();

    if (level > originalDisplayLevel)
        Core::instance()->suggestNewTask();
    return true;
}

int File::displayLevel() const
{
    return m_displayLevel;
}

void File::calculatePriority()
{
    int priority = -INT_MAX;
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

void File::saveAs(const QString &fileName, const QString &fileFormat)
{
    if (supportsViewing() &&
        !Core::instance()->fileExists(fileName)) {
        // Create placeholder
        QFile qFile(fileName);
        qFile.open(QIODevice::WriteOnly);

        QByteArray history = HistoryXml::encode(this);
        File *file = HistoryXml::decodeOne(history);

        file->setOriginalFileName(m_fileName);
        file->setFileFormat(m_fileFormat);

        file->setFileName(fileName);
        file->setTargetFormat(fileFormat);

        file->setClone(true);
        file->prepareSave();
        Core::instance()->attach(file);
        Core::instance()->suggestNewTask();
    }
}

bool File::isSaveInProgress() const
{
    return state() == State_Saving;
}

bool File::isDirty() const
{
    if (m_isClone)
        return true;
    else if (m_stack)
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

    if (m_stack->count() == 0)
        m_stack->load();
    m_stack->add(filter);

    abortSave();
    Core::instance()->dump();
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
        Core::instance()->dump();
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
        Core::instance()->dump();
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
    if ((state() == State_NonExistent) || (state() == State_UnsupportedFormat))
        setState(State_Placeholder);

    // Initialize stack for nonexistent files
    if (m_stack->isClean())
        m_stack->load();
    m_stack->setImage(level, image.convertToFormat(QImage::Format_RGB32));

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

bool File::hasThumbnail(int level)
{
    if(isOriginal())
        return false;
    if (Core::instance()->thumbnailFlavorName(level).isEmpty())
        return false;

    // Optimization currently for level 0 only
    // For externally supported files, thumbnails may appear at any time
    // so they need to be always checked from the file system
    if ((level == 0) &&
        (state() != State_ExternallySupportedFormat) &&
        (m_displayLevel >= 0))
        return m_hasThumbnail;

    QFileInfo info(thumbnailFileName(level));

    if (!info.exists())
        return false;

    return ((info.lastModified() >= lastModified()) ||
            // Ignore main files with modification dates in the future
            (lastModified() > QDateTime::currentDateTime()));
}

QString File::fileNameHash(const QString &fileName)
{
    const QUrl uri =
        QUrl::fromLocalFile(QFileInfo(fileName).canonicalFilePath());

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

File *File::readFromEditHistory(const QString &fileName,
                                const QString &originalFileName,
                                QuillError *error)
{
    // If original is not found, we will ignore any edit history.
    if (!QFile::exists(originalFileName)) {
        *error = QuillError(QuillError::FileNotFoundError,
                            QuillError::ImageOriginalErrorSource,
                            originalFileName);
        return 0;
    }

    QFile file(editHistoryFileName(fileName,
                                   Core::instance()->editHistoryPath()));

    Logger::log("[File] Reading edit history from "+file.fileName());

    if (!file.exists()) {
        *error = QuillError(QuillError::FileNotFoundError,
                            QuillError::EditHistoryErrorSource,
                            file.fileName());
        return 0;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        *error = QuillError(QuillError::FileOpenForReadError,
                            QuillError::EditHistoryErrorSource,
                            file.fileName());
        return 0;
    }
    const QByteArray history = file.readAll();
    if (history.isEmpty()) {
        *error = QuillError(QuillError::FileReadError,
                            QuillError::EditHistoryErrorSource,
                            file.fileName());
        return 0;
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

    Logger::log("[File] Edit history size is "+QString::number(history.size())+
                " bytes");
    Logger::log("[File] Edit history dump: "+history);

    File *result = HistoryXml::decodeOne(history);
    if (!result)
        *error = QuillError(QuillError::FileCorruptError,
                            QuillError::EditHistoryErrorSource,
                            file.fileName());
    if (result && readOnly)
        result->setReadOnly();

    return result;
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

    Logger::log("[File] Writing edit history to "+file.fileName());
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
        if (hasThumbnail(level))
            QFile::remove(thumbnailFileName(level));
    m_hasThumbnail = false;
}

void File::touchThumbnails()
{
    for (int level=0; level<Core::instance()->previewLevelCount(); level++)
        if (hasThumbnail(level)) {
            QFile file(thumbnailFileName(level));
            file.open(QIODevice::Append);
            file.close();
        }
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
        path = "/tmp";

    if (!QDir().mkpath(path)) {
        emitError(QuillError(QuillError::DirCreateError,
                             QuillError::TemporaryFileErrorSource,
                             path));
        return;
    }

    const QString filePath =
        path + QDir::separator() + "qt_temp.XXXXXX." + info.fileName();

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
    if (isJpeg() && !m_isClone) {
        QuillMetadata metadata(m_fileName);
        rawExifDump = metadata.dump(QuillMetadata::ExifFormat);
    }

    m_stack->prepareSave(m_temporaryFile->fileName(), rawExifDump);

    setState(State_Saving);
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

    if (isJpeg() && !m_isClone) {
        QuillMetadata metadata(m_fileName);
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

        if (!m_isClone)
            writeEditHistory(HistoryXml::encode(this), &result);

        if (result.errorCode() != QuillError::NoError) {
            emitError(result);
            goto cleanup;
        }
    }

    removeThumbnails();

    m_lastModified = QDateTime::currentDateTime();

    emit saved();
    Core::instance()->emitSaved(m_fileName);

 cleanup:
    setState(State_Normal);

    Core::instance()->dump();

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
        m_hasThumbnail = true;
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
    Core::instance()->insertFile(original, indexName);
    return original;
}

void File::processFilterError(QuillImageFilter *filter)
{
    if ((fullImageSize().isEmpty()) &&
        (filter->role() == QuillImageFilter::Role_Load)) {
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
    // Purge temporary images from cache
    if (state() != State_Placeholder)
        for (int l=0; l<=m_displayLevel; l++)
            Core::instance()->cache(l)->purge(this);
    else
        touchThumbnails();

    setState(State_Normal);

    if (!m_stack->isClean())
        m_stack->calculateFullImageSize(m_stack->command(0));

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
    emit error(quillError);
    Core::instance()->emitError(quillError);
    Logger::log("[File] "+QString(Q_FUNC_INFO)+QString(" code")+Logger::intToString((int)(quillError.errorCode()))+QString(" source")+Logger::intToString((int)(quillError.errorSource()))+QString(" data:")+quillError.errorData());
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
        Core::instance()->dump();
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
        Core::instance()->dump();
        Core::instance()->suggestNewTask();
        emitAllImages();
    }
}

bool File::isClone()
{
    return m_isClone;
}

void File::setClone(bool status)
{
    m_isClone = status;
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
    if (m_stack->isClean())
        m_stack->load();

    return ((m_state != State_NonExistent) &&
            (m_state != State_UnsupportedFormat) &&
            (m_state != State_ExternallySupportedFormat) &&
            (m_state != State_ReadOnly));
}

bool File::isOriginal() const
{
    return m_original;
}

void File::setOriginal(bool flag)
{
    m_original = flag;
}
