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

#include "core.h"
#include "logger.h"
#include "quillfile.h"
#include "file.h"
#include "historyxml.h"

class QuillFilePrivate {
public:
    File *m_file;
    int m_displayLevel;
};

QuillFile::QuillFile()
{
    priv = new QuillFilePrivate;
    priv->m_file = 0;
    priv->m_displayLevel = -1;
}

QuillFile::QuillFile(const QString &fileName,
                     const QString &fileFormat)
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO)+" file name: "+fileName+" file format: "+fileFormat);
    priv = new QuillFilePrivate;
    priv->m_displayLevel = -1;

    attach(Core::instance()->file(fileName, fileFormat));
}

QuillFile::QuillFile(File *file)
{
    priv = new QuillFilePrivate;
    priv->m_displayLevel = -1;
    attach(file);
}

QuillFile::~QuillFile()
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file) {
        priv->m_file->removeReference(this);
        if (priv->m_file->allowDelete())
            delete priv->m_file;
    }

    delete priv;
}

void QuillFile::attach(File *file)
{
    priv->m_file = file;

    if (priv->m_file) {
        priv->m_file->addReference(this);

        connect(priv->m_file, SIGNAL(saved()),
                SIGNAL(saved()));

        connect(priv->m_file, SIGNAL(removed()),
                SIGNAL(removed()));

        connect(priv->m_file,
                SIGNAL(error(QuillError)),
                SIGNAL(error(QuillError)));
    }
}

QString QuillFile::fileName() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->fileName();
    else
        return QString();
}

QString QuillFile::originalFileName() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->originalFileName();
    else
        return QString();
}

QString QuillFile::fileFormat() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->fileFormat();
    else
        return QString();
}

QString QuillFile::targetFormat() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->targetFormat();
    else
        return QString();
}

bool QuillFile::isReadOnly() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->isReadOnly();
    else
        return true;
}

void QuillFile::setReadOnly()
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->setReadOnly();
}

bool QuillFile::setDisplayLevel(int level)
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO)+Logger::intToString(level));
    // This needs to be done since setDisplayLevel checks the display levels
    // of referring QuillFile objects.
    int oldLevel = priv->m_displayLevel;
    priv->m_displayLevel = level;
    if (priv->m_file->setDisplayLevel(level))
        return true;
    else {
        priv->m_displayLevel = oldLevel;
        return false;
    }
}

int QuillFile::displayLevel() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    return priv->m_displayLevel;
}

void QuillFile::setPriority(int priority)
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO)+Logger::intToString(priority));
    Logger::log("[QuillFile] Warning: method setPriority() not implemented yet");
    Q_UNUSED(priority);
}

int QuillFile::priority() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    Logger::log("[QuillFile] Warning: method priority() not implemented yet");
    return 0;
}

void QuillFile::saveAs(const QString &fileName,
                       const QString &fileFormat)
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->saveAs(fileName, fileFormat);
}

void QuillFile::save()
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->save();
}

bool QuillFile::isSaveInProgress() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->isSaveInProgress();
    else
        return false;
}

void QuillFile::runFilter(QuillImageFilter *filter)
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->runFilter(filter);
}

void QuillFile::startSession()
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->startSession();
}

void QuillFile::endSession()
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->endSession();
}

bool QuillFile::isSession() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->isSession();
    else
        return false;
}

bool QuillFile::canUndo() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->canUndo();
    else
        return false;
}

void QuillFile::undo()
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->undo();
}

bool QuillFile::canRedo() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->canRedo();
    else
        return false;
}

void QuillFile::redo()
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->redo();
}

void QuillFile::dropRedoHistory()
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->dropRedoHistory();
}

QuillImage QuillFile::image() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->bestImage(priv->m_displayLevel);
    else
        return QuillImage();
}

QuillImage QuillFile::image(int level) const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO)+Logger::intToString(level));
    if ((level > priv->m_displayLevel) || !priv->m_file)
        return QuillImage();
    else
        return priv->m_file->image(level);
}

void QuillFile::setImage(int level, const QuillImage &image)
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO)+Logger::intToString(level));
    if ((level <= priv->m_displayLevel) && priv->m_file)
        priv->m_file->setImage(level, image);
}

QList<QuillImage> QuillFile::allImageLevels() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->allImageLevels(priv->m_displayLevel);
    else
        return QList<QuillImage>();
}

QSize QuillFile::fullImageSize() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->fullImageSize();
    else
        return QSize();
}

void QuillFile::setViewPort(const QRect &viewPort)
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->setViewPort(viewPort);
}

QRect QuillFile::viewPort() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->viewPort();
    else
        return QRect();
}

bool QuillFile::hasThumbnail(int level) const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO)+Logger::intToString(level));
    if (priv->m_file)
        return priv->m_file->hasThumbnail(level);
    else
        return false;
}

QString QuillFile::thumbnailFileName(int level) const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO)+Logger::intToString(level));
    if (priv->m_file)
        return priv->m_file->thumbnailFileName(level);
    else
        return QString();
}

bool QuillFile::exists() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->exists();
    else
        return false;
}

QDateTime QuillFile::lastModified() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->lastModified();
    else
        return QDateTime();
}

bool QuillFile::supportsThumbnails() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->supportsThumbnails();
    else
        return false;
}

bool QuillFile::supportsViewing() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->supportsViewing();
    else
        return false;
}

bool QuillFile::supportsEditing() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->supportsEditing();
    else
        return false;
}

bool QuillFile::supported() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->thumbnailSupported();
    else
        return false;
}

void QuillFile::setSupported(bool supported)
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO)+Logger::boolToString(supported));
    if (priv->m_file) {
        priv->m_file->setSupported(supported);
        priv->m_file->setThumbnailSupported(supported);
    }
}

void QuillFile::remove()
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->remove();
}

void QuillFile::removeThumbnails()
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->removeThumbnails();
}

QuillFile *QuillFile::original()
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    QuillFile *file = new QuillFile();
    file->attach(priv->m_file->original());
    return file;
}

void QuillFile::invalidate()
{
    priv->m_file = 0;
}

bool QuillFile::isValid() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    return (priv->m_file != 0);
}

void QuillFile::emitImageAvailable(const QuillImage &image)
{
    QList<QuillImage> imageList;
    imageList.append(image);
    emit imageAvailable(imageList);
}

void QuillFile::emitImageAvailable(const QList<QuillImage> &imageList)
{
    emit imageAvailable(imageList);
}

void QuillFile::refresh()
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->refresh();
}

void QuillFile::setWaitingForData(bool status)
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO)+Logger::boolToString(status));
    if (priv->m_file)
        priv->m_file->setWaitingForData(status);
}

bool QuillFile::isWaitingForData() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->isWaitingForData();
    else
        return false;
}

bool QuillFile::canRevert() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->canRevert();
    else
        return false;
}

void QuillFile::revert()
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->revert();
}

bool QuillFile::canRestore() const
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->canRestore();
    else
        return false;
}

void QuillFile::restore()
{
    Logger::log("[QuillFile] "+QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->restore();
}

File* QuillFile::internalFile()
{
    return priv->m_file;
}
