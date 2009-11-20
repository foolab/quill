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
#include "quill.h"
#include "core.h"
#include "imagecache.h"
#include "quillundostack.h"
#include "quillundocommand.h"
#include "historyxml.h"
#include "tilemap.h"

class QuillFilePrivate
{
public:
    Core *core;

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

    bool saveInProgress;
    QTemporaryFile *temporaryFile;
};

File::File(QObject *parent)
{
    priv = new QuillFilePrivate;
    priv->exists = true;
    priv->supported = true;
    priv->readOnly = false;

    priv->core = dynamic_cast<Core*>(parent);
    priv->stack = new QuillUndoStack(priv->core, this);
    priv->displayLevel = -1;

    priv->fileName = "";
    priv->originalFileName = "";
    priv->fileFormat = "";
    priv->targetFormat = "";
    priv->viewPort = QRect();

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
    priv->core->detach(this);
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
            if (priv->core->numFilesAtLevel(l) >= priv->core->fileLimit(l)) {
                setError(Quill::ErrorFileLimitExceeded);
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
        if ((l < priv->core->previewLevelCount()) || (!priv->saveInProgress))
            priv->core->cache(l)->purge(this);

    priv->displayLevel = level;

    // setup stack here
    if (exists() && (level >= 0) && (priv->stack->count() == 0))
        priv->stack->load();

    priv->core->suggestNewTask();
    return true;
}

int File::displayLevel() const
{
    return priv->displayLevel;
}

void File::save()
{
    if (priv->exists && priv->supported && !priv->readOnly &&
        !priv->saveInProgress && priv->stack->isDirty())
    {
        priv->saveInProgress = true;
        prepareSave();
        priv->core->suggestNewTask();
    }
}

bool File::isSaveInProgress() const
{
    return priv->saveInProgress;
}
/*
QuillFile *File::exportFile(const QString &newFileName,
                                 const QString &fileFormat)
{
        if (!priv->exists || !priv->supported)
        return 0;

    const QByteArray dump = HistoryXml::encode(this);
    QuillFile *file = HistoryXml::decodeOne(dump, priv->core);

    file->setFileName(newFileName);
    file->setTargetFormat(fileFormat);
    // todo: saved index needs to be reset to -1
    file->save();

    return file;
    return 0;
}
*/

void File::runFilter(QuillImageFilter *filter)
{
    if (!priv->exists || !priv->supported || priv->readOnly)
        return;

    if (priv->stack->count() == 0)
        priv->stack->load();
    priv->stack->add(filter);

    priv->saveInProgress = false;
    priv->core->suggestNewTask();
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
        priv->core->suggestNewTask();

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
        priv->core->suggestNewTask();

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

QList<QuillImage> File::allImageLevels(int displayLevel) const
{
    if (!priv->exists || !priv->stack->command())
        return QList<QuillImage>();
    else if ((!priv->stack->command()->tileMap()) ||
             (displayLevel < priv->core->previewLevelCount()))
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
    if (!priv->exists || (priv->displayLevel < priv->core->previewLevelCount()))
        return;

    priv->core->suggestNewTask();

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

    if (priv->core->thumbnailDirectory(level).isEmpty())
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
    hashValueString.append("." + priv->core->thumbnailExtension());
    hashValueString.prepend(priv->core->thumbnailDirectory(level) +
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

File *File::readFromEditHistory(const QString &fileName,
                                          QObject *parent)
{
    Core *core = dynamic_cast<Core*>(parent);

    QFile file(editHistoryFileName(fileName, core->editHistoryDirectory()));

    qDebug() << "Reading edit history from" << file.fileName();

    if (!file.exists())
        return 0;
    file.open(QIODevice::ReadOnly);
    const QByteArray history = file.readAll();
    file.close();

    qDebug() << "Read" << history.size() << "bytes";
    qDebug() << history;

    return HistoryXml::decodeOne(history, core);
}

void File::writeEditHistory(const QString &history)
{
    QDir().mkpath(priv->core->editHistoryDirectory());
    QFile file(editHistoryFileName(priv->fileName,
                                   priv->core->editHistoryDirectory()));

    qDebug() << "Writing edit history to" << file.fileName();

    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    file.write(history.toAscii());
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
        if (file->displayLevel() == priv->core->previewLevelCount())
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
                                      priv->core->editHistoryDirectory()));
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

void File::setSupported(bool supported)
{
    priv->supported = supported;
}

bool File::supported() const
{
    return priv->supported;
}

void File::overwritingCopy(const QString &fileName,
                                const QString &newName)
{
    QDir().mkpath(QFileInfo(newName).path());

    QFile source(fileName),
        target(newName);

    source.open(QIODevice::ReadOnly);
    const QByteArray buffer = source.readAll();
    source.close();

    target.open(QIODevice::WriteOnly | QIODevice::Truncate);
    target.write(buffer);
    target.close();
}

void File::removeThumbnails()
{
    for (int level=0; level<priv->core->previewLevelCount(); level++)
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

    priv->temporaryFile =
        new QTemporaryFile("/tmp/qt_temp.XXXXXX." + info.fileName());
    priv->temporaryFile->open();

    priv->stack->prepareSave(priv->temporaryFile->fileName());
}

void File::concludeSave()
{
    // If save is concluded, purge any full images still in memory.
    if (priv->displayLevel < priv->core->previewLevelCount())
        priv->core->cache(priv->core->previewLevelCount())->purge(this);

    const QString temporaryName = priv->temporaryFile->fileName();
    priv->temporaryFile->close();

    QFile file(priv->originalFileName);

    // Original file does not exist - backup current into original
    // before replacing with contents of temp file.

    if ((!file.exists()) || (!file.size() > 0))
        File::overwritingCopy(priv->fileName,
                                   priv->originalFileName);

    // This is more efficient than renaming between partitions.

    File::overwritingCopy(temporaryName,
                               priv->fileName);

    priv->stack->concludeSave();

    writeEditHistory(HistoryXml::encode(this));
    removeThumbnails();
    priv->saveInProgress = false;

    QFile::remove(temporaryName);

    delete priv->temporaryFile;
    priv->temporaryFile = 0;

    emit saved();
}

bool File::hasOriginal()
{
    QString indexName = QString('\\') + priv->fileName;
    return priv->core->fileExists(indexName);
}

File *File::original()
{
    QString indexName = QString('\\') + priv->fileName;

    if (priv->core->fileExists(indexName))
        return priv->core->file(indexName, "");

    File *original = new File(priv->core);
    original->setFileName(priv->fileName);
    original->setFileFormat(priv->fileFormat);
    original->setOriginalFileName(priv->originalFileName);
    original->setReadOnly();

    priv->core->insertFile(original, indexName);
    return original;
}

void File::setError(Quill::Error errorCode)
{
    qDebug() << "Error" << errorCode << "with file" << priv->fileName << "!";

    emit error(errorCode);
}
