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

#include "core.h"
#include "logger.h"
#include "quillfile.h"
#include "file.h"
#include "historyxml.h"
#include "unix_platform.h"

// values defined in the header
const int QuillFile::Priority_Low;
const int QuillFile::Priority_Normal;
const int QuillFile::Priority_High;

class QuillFilePrivate {
public:
    File *m_file;
    int m_displayLevel;
    int m_priority;
};

QuillFile::QuillFile()
{
    priv = new QuillFilePrivate;
    priv->m_file = 0;
    priv->m_displayLevel = -1;
    priv->m_priority = Priority_Normal;
}

QuillFile::QuillFile(const QString &fileName,
                     const QString &fileFormat)
{
    QUILL_LOG(Logger::Module_QuillFile,
                QString(Q_FUNC_INFO)+" file name: "+fileName+" file format: "+fileFormat);
    priv = new QuillFilePrivate;
    priv->m_displayLevel = -1;
    priv->m_priority = Priority_Normal;

    attach(Core::instance()->file(fileName, fileFormat));
}

QuillFile::QuillFile(File *file)
{
    priv = new QuillFilePrivate;
    priv->m_displayLevel = -1;
    priv->m_priority = Priority_Normal;
    attach(file);
}

QuillFile::~QuillFile()
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
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

        priv->m_file->calculatePriority();
    }
}

QString QuillFile::fileName() const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->fileName();
    else
        return QString();
}

QString QuillFile::originalFileName() const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->originalFileName();
    else
        return QString();
}

QString QuillFile::fileFormat() const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->fileFormat();
    else
        return QString();
}

QString QuillFile::targetFormat() const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->targetFormat();
    else
        return QString();
}

bool QuillFile::setDisplayLevel(int level)
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO)+Logger::intToString(level));
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
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    return priv->m_displayLevel;
}

void QuillFile::setPriority(int priority)
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO)+Logger::intToString(priority));
    priv->m_priority = priority;
    priv->m_file->calculatePriority();
}

int QuillFile::priority() const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO)+Logger::intToString(priv->m_priority));
    return priv->m_priority;
}

void QuillFile::save()
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->save();
}

bool QuillFile::isSaveInProgress() const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->isSaveInProgress();
    else
        return false;
}

bool QuillFile::isDirty() const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->isDirty();
    else
        return false;
}

void QuillFile::runFilter(QuillImageFilter *filter)
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->runFilter(filter);
}

void QuillFile::startSession()
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->startSession();
}

void QuillFile::endSession()
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->endSession();
}

bool QuillFile::isSession() const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->isSession();
    else
        return false;
}

bool QuillFile::canUndo() const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->canUndo();
    else
        return false;
}

void QuillFile::undo()
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->undo();
}

bool QuillFile::canRedo() const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->canRedo();
    else
        return false;
}

void QuillFile::redo()
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->redo();
}

void QuillFile::dropRedoHistory()
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->dropRedoHistory();
}

QuillImage QuillFile::image() const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->bestImage(priv->m_displayLevel);
    else
        return QuillImage();
}

QuillImage QuillFile::image(int level) const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO)+Logger::intToString(level));
    if ((level > priv->m_displayLevel) || !priv->m_file)
        return QuillImage();
    else
        return priv->m_file->image(level);
}

void QuillFile::setImage(int level, const QuillImage &image)
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO)+Logger::intToString(level));
    if (priv->m_file) {
        priv->m_file->setImage(level, image);
        // setImage() may modify the internal display level of File object,
        // update the level for this QuillFile instance.
        setDisplayLevel(level);
    }
}

QList<QuillImage> QuillFile::allImageLevels() const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->allImageLevels(priv->m_displayLevel);
    else
        return QList<QuillImage>();
}

QSize QuillFile::fullImageSize() const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->fullImageSize();
    else
        return QSize();
}

void QuillFile::setViewPort(const QRect &viewPort)
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->setViewPort(viewPort);
}

QRect QuillFile::viewPort() const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->viewPort();
    else
        return QRect();
}

bool QuillFile::hasThumbnail(int level) const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO)+Logger::intToString(level));
    //make sure level has a valid value
    if (priv->m_file&&level>=0)
        return priv->m_file->hasThumbnail(level);
    else
        return false;
}

QString QuillFile::thumbnailFileName(int level) const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO)+Logger::intToString(level));
    if (priv->m_file)
        return priv->m_file->thumbnailFileName(level);
    else
        return QString();
}

QString QuillFile::failedThumbnailFileName() const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->failedThumbnailFileName();
    else
        return QString();
}

bool QuillFile::exists() const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->exists();
    else
        return false;
}

QDateTime QuillFile::lastModified() const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->lastModified();
    else
        return QDateTime();
}

bool QuillFile::supportsThumbnails() const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->supportsThumbnails();
    else
        return false;
}

bool QuillFile::supportsViewing() const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->supportsViewing();
    else
        return false;
}

bool QuillFile::supportsEditing() const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->supportsEditing();
    else
        return false;
}

void QuillFile::remove()
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->remove();
}

void QuillFile::removeThumbnails()
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->removeThumbnails();
}

QuillFile *QuillFile::original()
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
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
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
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
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->refresh();
}

bool QuillFile::canRevert() const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->canRevert();
    else
        return false;
}

void QuillFile::revert()
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->revert();
}

bool QuillFile::canRestore() const
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        return priv->m_file->canRestore();
    else
        return false;
}

void QuillFile::restore()
{
    QUILL_LOG(Logger::Module_QuillFile, QString(Q_FUNC_INFO));
    if (priv->m_file)
        priv->m_file->restore();
}

QuillError QuillFile::error() const
{
    if (priv->m_file)
        return priv->m_file->error();

    // This should never happen, return unspecified just in case
    return QuillError(QuillError::UnspecifiedError);
}

File* QuillFile::internalFile()
{
    return priv->m_file;
}

bool QuillFile::lock(bool overrideOwnLock)
{
    return LockFile::lockQuillFile(this, overrideOwnLock);
}

void QuillFile::unlock()
{
    LockFile::unlockQuillFile(this);
}

bool QuillFile::isLocked(bool overrideOwnLock) const
{
    return LockFile::isQuillFileLocked(this, overrideOwnLock);
}
