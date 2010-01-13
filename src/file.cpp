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

#include <QTemporaryFile>
#include <QFileInfo>
#include <QUrl>
#include <QCryptographicHash>
#include <QList>
#include <QDir>
#include <QDebug>
#include "file.h"
#include "core.h"
#include "imagecache.h"
#include "quillundostack.h"
#include "quillundocommand.h"
#include "historyxml.h"
#include "tilemap.h"
#include "quillerror.h"

class FilePrivate
{
public:
    bool exists;
    bool supported;
    bool readOnly;

    QuillUndoStack *stack;

    int displayLevel;

    QString fileName;
    QString originalFileName;
    QString fileFormat;
    QString targetFormat;

    QRect viewPort;

    bool waitingForData;
    bool saveInProgress;
    QTemporaryFile *temporaryFile;
};

File::File()
{
    priv = new FilePrivate;
    priv->exists = true;
    priv->supported = true;
    priv->readOnly = false;

    priv->stack = new QuillUndoStack(this);
    priv->displayLevel = -1;

    priv->fileName = "";
    priv->originalFileName = "";
    priv->fileFormat = "";
    priv->targetFormat = "";
    priv->viewPort = QRect();

    priv->waitingForData = false;
    priv->saveInProgress = false;
    priv->temporaryFile = 0;
}

File::~File()
{
    detach();
    delete priv->stack;
    delete priv->temporaryFile;
    delete priv;
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
        if (m_references.isEmpty() && !priv->saveInProgress) {
            detach();
        }
    }
}

bool File::allowDelete()
{
    return m_references.isEmpty() && !priv->saveInProgress;
}

void File::detach()
{
    Core::instance()->detach(this);
    foreach (QuillFile *file, m_references)
        file->invalidate();
}

QString File::fileName() const
{
    return priv->fileName;
}

QString File::fileFormat() const
{
    return priv->fileFormat;
}

QString File::originalFileName() const
{
    return priv->originalFileName;
}

QString File::targetFormat() const
{
    return priv->targetFormat;
}

void File::setFileName(const QString &fileName)
{
    priv->fileName = fileName;
}

void File::setFileFormat(const QString &fileFormat)
{
    priv->fileFormat = fileFormat;
}

void File::setOriginalFileName(const QString &originalFileName)
{
    priv->originalFileName = originalFileName;
}

void File::setTargetFormat(const QString &targetFormat)
{
    priv->targetFormat = targetFormat;
}

void File::setReadOnly()
{
    priv->readOnly = true;
}

bool File::isReadOnly() const
{
    return priv->readOnly;
}

bool File::setDisplayLevel(int level)
{
    if (level > priv->displayLevel) {
        // Block if trying to raise display level over strict limits
        for (int l=priv->displayLevel+1; l<=level; l++)
            if (Core::instance()->numFilesAtLevel(l) >= Core::instance()->fileLimit(l)) {
                emitError(QuillError(QuillError::GlobalFileLimitError,
                                     QuillError::NoErrorSource,
                                     priv->fileName));
                return false;
            }
    } else {
        // If some other file object wants to keep the display level up
        foreach (QuillFile *file, m_references)
            if (file->displayLevel() > level)
                level = file->displayLevel();
    }

    // Purge images from cache if lowering display level here
    // Exception: when save is in progress, leave the highest level
    for (int l=priv->displayLevel; l>level; l--)
        if ((l < Core::instance()->previewLevelCount()) || (!priv->saveInProgress))
            Core::instance()->cache(l)->purge(this);

    priv->displayLevel = level;

    // setup stack here
    if (exists() && (level >= 0) && (priv->stack->count() == 0)) {
        priv->stack->load();
    }

    Core::instance()->suggestNewTask();
    return true;
}

int File::displayLevel() const
{
    return priv->displayLevel;
}

void File::save()
{
    if (priv->exists && priv->supported && !priv->readOnly &&
        !priv->saveInProgress && isDirty())
    {
        prepareSave();
        Core::instance()->suggestNewTask();
    }
}

bool File::isSaveInProgress() const
{
    return priv->saveInProgress;
}

bool File::isDirty() const
{
    if (priv->stack)
        return priv->stack->isDirty();
    else
        return false;
}

void File::runFilter(QuillImageFilter *filter)
{
    if (!priv->exists || !priv->supported || priv->readOnly)
        return;

    if (priv->stack->count() == 0)
        priv->stack->load();
    priv->stack->add(filter);

    priv->saveInProgress = false;
    Core::instance()->dump();
    Core::instance()->suggestNewTask();
}

void File::startSession()
{
    if (priv->exists && priv->supported && !priv->readOnly)
        priv->stack->startSession();
}

void File::endSession()
{
    if (priv->stack)
        priv->stack->endSession();
}

bool File::isSession() const
{
    return ((priv->stack) && (priv->stack->isSession()));
}

bool File::canUndo() const
{
    if (!priv->exists || !priv->supported || priv->readOnly)
        return false;
    return priv->stack->canUndo();
}

void File::undo()
{
    if (canUndo())
    {
        priv->stack->undo();

        priv->saveInProgress = false;
        Core::instance()->dump();
        Core::instance()->suggestNewTask();

        emitAllImages();
    }
}

bool File::canRedo() const
{
    if (!priv->exists || !priv->supported || priv->readOnly)
        return false;

    return priv->stack->canRedo();
}

void File::redo()
{
    if (canRedo())
    {
        priv->stack->redo();

        priv->saveInProgress = false;
        Core::instance()->dump();
        Core::instance()->suggestNewTask();

        emitAllImages();
    }
}

QuillImage File::bestImage(int displayLevel) const
{
    if (!priv->exists)
        return QuillImage();
    return priv->stack->bestImage(displayLevel);
}

QuillImage File::image(int level) const
{
    if (!priv->exists)
        return QuillImage();
    return priv->stack->image(level);
}

void File::setImage(int level, const QuillImage &image)
{
    priv->stack->setImage(level, image);
}

QList<QuillImage> File::allImageLevels(int displayLevel) const
{
    if (!priv->exists || !priv->stack->command())
        return QList<QuillImage>();
    else if ((!priv->stack->command()->tileMap()) ||
             (displayLevel < Core::instance()->previewLevelCount()))
        return priv->stack->allImageLevels(displayLevel);
    else
        return priv->stack->allImageLevels(displayLevel) +
            priv->stack->command()->tileMap()->nonEmptyTiles(priv->viewPort);
}

QSize File::fullImageSize() const
{
    if (!priv->exists)
        return QSize();
    return priv->stack->fullImageSize();
}

void File::setViewPort(const QRect &viewPort)
{
    const QRect oldPort = priv->viewPort;
    priv->viewPort = viewPort;

    // New tiles will only be calculated if the display level allows it
    if (!priv->exists || (priv->displayLevel < Core::instance()->previewLevelCount()))
        return;

    Core::instance()->suggestNewTask();

    QList<QuillImage> newTiles;

    if ((priv->stack->command()) && (priv->stack->command()->tileMap()))
        newTiles = priv->stack->command()->tileMap()->
	    newTiles(oldPort, viewPort);

    if (!newTiles.isEmpty())
        emitTiles(newTiles);
}

QRect File::viewPort() const
{
    return priv->viewPort;
}

QuillUndoStack *File::stack() const
{
    return priv->stack;
}

bool File::hasThumbnail(int level) const
{
    if (!priv->exists)
        return false;

    if (Core::instance()->thumbnailDirectory(level).isEmpty())
        return false;

    return QFile::exists(thumbnailFileName(level));
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

QString File::thumbnailFileName(int level) const
{
    QString hashValueString = fileNameHash(priv->fileName);
    hashValueString.append("." + Core::instance()->thumbnailExtension());
    hashValueString.prepend(Core::instance()->thumbnailDirectory(level) +
                            QDir::separator());

    return hashValueString;
}

QString File::editHistoryFileName(const QString &fileName,
                                  const QString &editHistoryDirectory)
{
    QString hashValueString = fileNameHash(fileName);
    hashValueString.append(".xml");
    hashValueString.prepend(editHistoryDirectory + QDir::separator());

    return hashValueString;
}

File *File::readFromEditHistory(const QString &fileName, QuillError *error)
{
    QFile file(editHistoryFileName(fileName,
                                   Core::instance()->editHistoryDirectory()));

    qDebug() << "Reading edit history from" << file.fileName();

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

    qDebug() << "Read" << history.size() << "bytes";
    qDebug() << history;

    File *result = HistoryXml::decodeOne(history);
    if (!result)
        *error = QuillError(QuillError::FileCorruptError,
                            QuillError::EditHistoryErrorSource,
                            file.fileName());
    return result;
}

void File::writeEditHistory(const QString &history, QuillError *error)
{
    if (!QDir().mkpath(Core::instance()->editHistoryDirectory())) {
        *error = QuillError(QuillError::DirCreateError,
                            QuillError::EditHistoryErrorSource,
                            Core::instance()->editHistoryDirectory());
        return;
    }
    QFile file(editHistoryFileName(priv->fileName,
                                   Core::instance()->editHistoryDirectory()));

    qDebug() << "Writing edit history to" << file.fileName();
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        *error = QuillError(QuillError::FileOpenForWriteError,
                            QuillError::EditHistoryErrorSource,
                            Core::instance()->editHistoryDirectory());
        return;
    }
    qint64 fileSize = file.write(history.toAscii());
    if(fileSize == -1) {
        *error = QuillError(QuillError::FileWriteError,
                            QuillError::EditHistoryErrorSource,
                            Core::instance()->editHistoryDirectory());
        return;
    }
    file.close();
}

void File::emitSingleImage(QuillImage image, int level)
{
    foreach(QuillFile *file, m_references)
        if (file->displayLevel() >= level)
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
    if (!priv->exists)
        return;

    QFile(priv->fileName).remove();
    QFile(priv->originalFileName).remove();
    QFile::remove(editHistoryFileName(priv->fileName,
                                      Core::instance()->editHistoryDirectory()));
    removeThumbnails();

    priv->exists = false;
    priv->saveInProgress = false;
    delete priv->stack;
    priv->stack = 0;

    if (hasOriginal())
        original()->remove();

    emit removed();
}

bool File::exists() const
{
    return priv->exists;
}

void File::setExists(bool exists)
{
    priv->exists = exists;
}

void File::setSupported(bool supported)
{
    priv->supported = supported;
}

bool File::supported() const
{
    return priv->supported;
}

QuillError File::overwritingCopy(const QString &fileName,
                                 const QString &newName)
{
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
}

void File::prepareSave()
{
    delete priv->temporaryFile;

    // Save filter always operates with temporary files.

    QFileInfo info(priv->fileName);

    // This guarantees that the temporary file will have the same
    // extension as the target file, so that the correct format can be
    // deduced by QImageReader.

    QString path = Core::instance()->temporaryFileDirectory();
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

    priv->temporaryFile = new QTemporaryFile(filePath);
    if (!priv->temporaryFile) {
        emitError(QuillError(QuillError::FileOpenForReadError,
                             QuillError::TemporaryFileErrorSource,
                             filePath));
        return;
    }

    if (!priv->temporaryFile->open()) {
        emitError(QuillError(QuillError::FileOpenForReadError,
                             QuillError::TemporaryFileErrorSource,
                             priv->temporaryFile->fileName()));
        return;
    }

    priv->stack->prepareSave(priv->temporaryFile->fileName());

    priv->saveInProgress = true;
}

void File::concludeSave()
{
    // If save is concluded, purge any full images still in memory.
    if (priv->displayLevel < Core::instance()->previewLevelCount())
        Core::instance()->cache(Core::instance()->previewLevelCount())->purge(this);

    const QString temporaryName = priv->temporaryFile->fileName();
    priv->temporaryFile->close();

    QFile file(priv->originalFileName);

    // Original file does not exist - backup current into original
    // before replacing with contents of temp file.

    if ((!file.exists()) || (!file.size() > 0)) {
        QuillError result = File::overwritingCopy(priv->fileName,
                                                  priv->originalFileName);
        if (result.errorCode() != QuillError::NoError) {
            result.setErrorSource(QuillError::TemporaryFileErrorSource);
            emitError(result);
            return;
        }
    }

    // This is more efficient than renaming between partitions.

    QuillError result = File::overwritingCopy(temporaryName,
                                              priv->fileName);
    if (result.errorCode() != QuillError::NoError) {
        result.setErrorSource(QuillError::ImageFileErrorSource);
        emitError(result);
        return;
    }

    priv->stack->concludeSave();

    writeEditHistory(HistoryXml::encode(this), &result);

    if (result.errorCode() != QuillError::NoError) {
        emitError(result);
        return;
    }

    removeThumbnails();
    priv->saveInProgress = false;

    Core::instance()->dump();

    QFile::remove(temporaryName);

    delete priv->temporaryFile;
    priv->temporaryFile = 0;

    emit saved();
    Core::instance()->emitSaved(priv->fileName);
}

bool File::hasOriginal()
{
    QString indexName = QString('\\') + priv->fileName;
    return Core::instance()->fileExists(indexName);
}

File *File::original()
{
    QString indexName = QString('\\') + priv->fileName;

    if (Core::instance()->fileExists(indexName))
        return Core::instance()->file(indexName, "");

    File *original = new File();
    original->setFileName(priv->fileName);
    original->setFileFormat(priv->fileFormat);
    original->setOriginalFileName(priv->originalFileName);
    original->setReadOnly();

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
            priv->fileName) {
            errorSource = QuillError::ImageFileErrorSource;
            if ((errorCode == QuillError::FileFormatUnsupportedError) ||
                (errorCode == QuillError::FileCorruptError))
                setSupported(false);
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

void File::setWaitingForData(bool status)
{
    priv->waitingForData = status;

    if (!status) {
        priv->supported = true;
        priv->stack->calculateFullImageSize(priv->stack->command(0));
        Core::instance()->suggestNewTask();
    }
}

bool File::isWaitingForData() const
{
    return priv->waitingForData;
}

void File::emitError(QuillError quillError)
{
    qDebug() << "Error" << quillError.errorCode() << "at source" << quillError.errorSource() << "data" << quillError.errorData();

    emit error(quillError);
    Core::instance()->emitError(quillError);
}
