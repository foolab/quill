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
#include "quillfile.h"
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

    QuillFile *original;
};

QuillFile::QuillFile(QObject *parent)
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

    priv->original = 0;
}

QuillFile::~QuillFile()
{
    delete priv->stack;
    delete priv->temporaryFile;
    delete priv;
}

QString QuillFile::fileName() const
{
    return priv->fileName;
}

QString QuillFile::fileFormat() const
{
    return priv->fileFormat;
}

QString QuillFile::originalFileName() const
{
    return priv->originalFileName;
}

QString QuillFile::targetFormat() const
{
    return priv->targetFormat;
}

void QuillFile::setFileName(const QString &fileName)
{
    priv->fileName = fileName;
}

void QuillFile::setFileFormat(const QString &fileFormat)
{
    priv->fileFormat = fileFormat;
}

void QuillFile::setOriginalFileName(const QString &originalFileName)
{
    priv->originalFileName = originalFileName;
}

void QuillFile::setTargetFormat(const QString &targetFormat)
{
    priv->targetFormat = targetFormat;
}

void QuillFile::setReadOnly()
{
    priv->readOnly = true;
}

bool QuillFile::isReadOnly() const
{
    return priv->readOnly;
}

bool QuillFile::setDisplayLevel(int level)
{
    // Block if trying to raise display level over strict limits
    for (int l=priv->displayLevel+1; l<=level; l++)
        if (priv->core->numFilesAtLevel(l) >= priv->core->fileLimit(l)) {
            // workaround of setError setting supported to false
            bool prevSupported = priv->supported;
            setError(Quill::ErrorFileLimitExceeded);
            priv->supported = prevSupported;
            return false;
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

int QuillFile::displayLevel() const
{
    return priv->displayLevel;
}

void QuillFile::save()
{
    if (priv->exists && priv->supported && !priv->readOnly &&
        !priv->saveInProgress && priv->stack->isDirty())
    {
        priv->saveInProgress = true;
        prepareSave();
        priv->core->suggestNewTask();
    }
}

bool QuillFile::isSaveInProgress() const
{
    return priv->saveInProgress;
}

QuillFile *QuillFile::exportFile(const QString &newFileName,
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
}

void QuillFile::runFilter(QuillImageFilter *filter)
{
    if (!priv->exists || !priv->supported || priv->readOnly)
        return;

    if (priv->stack->count() == 0)
        priv->stack->load();
    priv->stack->add(filter);

    priv->saveInProgress = false;
    priv->core->suggestNewTask();
}

void QuillFile::startSession()
{
    if (priv->exists && priv->supported && !priv->readOnly)
        priv->stack->startSession();
}

void QuillFile::endSession()
{
    if (priv->stack)
        priv->stack->endSession();
}

bool QuillFile::isSession() const
{
    return ((priv->stack) && (priv->stack->isSession()));
}

bool QuillFile::canUndo() const
{
    if (!priv->exists || !priv->supported || priv->readOnly)
        return false;
    return priv->stack->canUndo();
}

void QuillFile::undo()
{
    if (canUndo())
    {
        priv->stack->undo();

        priv->saveInProgress = false;
        priv->core->suggestNewTask();

        QList<QuillImage> levels = allImageLevels();
        if (!levels.isEmpty())
            emit imageAvailable(levels);
    }
}

bool QuillFile::canRedo() const
{
    if (!priv->exists || !priv->supported || priv->readOnly)
        return false;

    return priv->stack->canRedo();
}

void QuillFile::redo()
{
    if (canRedo())
    {
        priv->stack->redo();

        priv->saveInProgress = false;
        priv->core->suggestNewTask();

        QList<QuillImage> levels = allImageLevels();
        if (!levels.isEmpty())
            emit imageAvailable(levels);
    }
}

QuillImage QuillFile::image() const
{
    if (!priv->exists)
        return QuillImage();
    return priv->stack->image();
}

QuillImage QuillFile::image(int level) const
{
    if (!priv->exists)
        return QuillImage();
    return priv->stack->image(level);
}

QList<QuillImage> QuillFile::allImageLevels() const
{
    if (!priv->exists || !priv->stack->command())
        return QList<QuillImage>();
    else if ((!priv->stack->command()->tileMap()) ||
             (priv->displayLevel < priv->core->previewLevelCount()))
        return priv->stack->allImageLevels(priv->displayLevel);
    else
        return priv->stack->allImageLevels(priv->displayLevel) +
            priv->stack->command()->tileMap()->nonEmptyTiles(priv->viewPort);
}

QSize QuillFile::fullImageSize() const
{
    if (!priv->exists)
        return QSize();
    return priv->stack->fullImageSize();
}

void QuillFile::setViewPort(const QRect &viewPort)
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
        emit imageAvailable(newTiles);
}

QRect QuillFile::viewPort() const
{
    return priv->viewPort;
}

QuillUndoStack *QuillFile::stack() const
{
    return priv->stack;
}

bool QuillFile::hasThumbnail(int level) const
{
    if (!priv->exists)
        return false;

    if (priv->core->thumbnailDirectory(level).isEmpty())
        return false;

    return QFile::exists(thumbnailFileName(level));
}

QString QuillFile::fileNameHash(const QString &fileName)
{
    const QUrl uri =
        QUrl::fromLocalFile(QFileInfo(fileName).canonicalFilePath());

    const QByteArray hashValue =
        QCryptographicHash::hash(uri.toString().toLatin1(),
                                 QCryptographicHash::Md5);

    return hashValue.toHex();
}

QString QuillFile::thumbnailFileName(int level) const
{
    QString hashValueString = fileNameHash(priv->fileName);
    hashValueString.append("." + priv->core->thumbnailExtension());
    hashValueString.prepend(priv->core->thumbnailDirectory(level) +
                            QDir::separator());

    return hashValueString;
}

QString QuillFile::editHistoryFileName(const QString &fileName,
                                       const QString &editHistoryDirectory)
{
    QString hashValueString = fileNameHash(fileName);
    hashValueString.append(".xml");
    hashValueString.prepend(editHistoryDirectory + QDir::separator());

    return hashValueString;
}

QuillFile *QuillFile::readFromEditHistory(const QString &fileName,
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

void QuillFile::writeEditHistory(const QString &history)
{
    QDir().mkpath(priv->core->editHistoryDirectory());
    QFile file(editHistoryFileName(priv->fileName,
                                   priv->core->editHistoryDirectory()));

    qDebug() << "Writing edit history to" << file.fileName();

    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    file.write(history.toAscii());
    file.close();
}

void QuillFile::emitImageAvailable(QList<QuillImage> imageList)
{
    emit imageAvailable(imageList);
}

void QuillFile::remove()
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

    if (priv->original)
        priv->original->remove();

    emit removed();
}

bool QuillFile::exists() const
{
    return priv->exists;
}

bool QuillFile::supported() const
{
    return priv->supported;
}

void QuillFile::overwritingCopy(const QString &fileName,
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

void QuillFile::removeThumbnails()
{
    for (int level=0; level<priv->core->previewLevelCount(); level++)
        if (hasThumbnail(level))
            QFile::remove(thumbnailFileName(level));
}

void QuillFile::prepareSave()
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

void QuillFile::concludeSave()
{
    priv->stack->concludeSave();

    // If save is concluded, purge any full images still in memory.
    if (priv->displayLevel < priv->core->previewLevelCount())
        priv->core->cache(priv->core->previewLevelCount())->purge(this);

    const QString temporaryName = priv->temporaryFile->fileName();
    priv->temporaryFile->close();

    QFile file(priv->originalFileName);

    // Original file does not exist - backup current into original
    // before replacing with contents of temp file.

    if ((!file.exists()) || (!file.size() > 0))
        QuillFile::overwritingCopy(priv->fileName,
                                   priv->originalFileName);

    // This is more efficient than renaming between partitions.

    QuillFile::overwritingCopy(temporaryName,
                               priv->fileName);

    writeEditHistory(HistoryXml::encode(this));
    removeThumbnails();
    priv->saveInProgress = false;

    QFile::remove(temporaryName);

    delete priv->temporaryFile;
    priv->temporaryFile = 0;

    emit saved();
}

QuillFile *QuillFile::original()
{
    if (priv->original)
        return priv->original;

    QuillFile *original = new QuillFile(priv->core);
    original->setFileName(priv->fileName);
    original->setFileFormat(priv->fileFormat);
    original->setOriginalFileName(priv->originalFileName);
    original->setReadOnly();

    priv->original = original;
    priv->core->insertFile(original, "");
    return original;
}

void QuillFile::setError(Quill::Error errorCode)
{
    qDebug() << "Error" << errorCode << "with file" << priv->fileName << "!";

    priv->supported = false;
    emit error(errorCode);
}
