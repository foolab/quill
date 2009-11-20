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
#include "quillfile.h"
#include "file.h"

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
    priv = new QuillFilePrivate;
    priv->m_displayLevel = -1;

    attach(Core::instance()->file(fileName, fileFormat));
}

QuillFile::~QuillFile()
{
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

        connect(priv->m_file, SIGNAL(imageAvailable(const QuillImageList)),
                SIGNAL(imageAvailable(const QuillImageList)));

        connect(priv->m_file, SIGNAL(saved()),
                SIGNAL(saved()));

        connect(priv->m_file, SIGNAL(removed()),
                SIGNAL(removed()));

        connect(priv->m_file, SIGNAL(error(Quill::Error)),
                SIGNAL(error(Quill::Error)));
    }
}

QString QuillFile::fileName() const
{
    if (priv->m_file)
        return priv->m_file->fileName();
    else
        return QString();
}

QString QuillFile::originalFileName() const
{
    if (priv->m_file)
        return priv->m_file->originalFileName();
    else
        return QString();
}

QString QuillFile::fileFormat() const
{
    if (priv->m_file)
        return priv->m_file->fileFormat();
    else
        return QString();
}

QString QuillFile::targetFormat() const
{
    if (priv->m_file)
        return priv->m_file->targetFormat();
    else
        return QString();
}

bool QuillFile::isReadOnly() const
{
    if (priv->m_file)
        return priv->m_file->isReadOnly();
    else
        return true;
}

void QuillFile::setReadOnly()
{
    if (priv->m_file)
        priv->m_file->setReadOnly();
}

bool QuillFile::setDisplayLevel(int level)
{
    if (priv->m_file)
        return priv->m_file->setDisplayLevel(level);
    else
        return false;
}

int QuillFile::displayLevel() const
{
    if (priv->m_file)
        return priv->m_file->displayLevel();
    else
        return -1;
}

void QuillFile::save()
{
    if (priv->m_file)
        priv->m_file->save();
}

bool QuillFile::isSaveInProgress() const
{
    if (priv->m_file)
        return priv->m_file->isSaveInProgress();
    else
        return false;
}

void QuillFile::runFilter(QuillImageFilter *filter)
{
    if (priv->m_file)
        priv->m_file->runFilter(filter);
}

void QuillFile::startSession()
{
    if (priv->m_file)
        priv->m_file->startSession();
}

void QuillFile::endSession()
{
    if (priv->m_file)
        priv->m_file->endSession();
}

bool QuillFile::isSession() const
{
    if (priv->m_file)
        return priv->m_file->isSession();
    else
        return false;
}

bool QuillFile::canUndo() const
{
    if (priv->m_file)
        return priv->m_file->canUndo();
    else
        return false;
}

void QuillFile::undo()
{
    if (priv->m_file)
        priv->m_file->undo();
}

bool QuillFile::canRedo() const
{
    if (priv->m_file)
        return priv->m_file->canRedo();
    else
        return false;
}

void QuillFile::redo()
{
    if (priv->m_file)
        priv->m_file->redo();
}

QuillImage QuillFile::image() const
{
    if (priv->m_file)
        return priv->m_file->image();
    else
        return QuillImage();
}

QuillImage QuillFile::image(int level) const
{
    if (priv->m_file)
        return priv->m_file->image(level);
    else
        return QuillImage();
}

QList<QuillImage> QuillFile::allImageLevels() const
{
    if (priv->m_file)
        return priv->m_file->allImageLevels();
    else
        return QList<QuillImage>();
}

QSize QuillFile::fullImageSize() const
{
    if (priv->m_file)
        return priv->m_file->fullImageSize();
    else
        return QSize();
}

void QuillFile::setViewPort(const QRect &viewPort)
{
    if (priv->m_file)
        priv->m_file->setViewPort(viewPort);
}

QRect QuillFile::viewPort() const
{
    if (priv->m_file)
        return priv->m_file->viewPort();
    else
        return QRect();
}

bool QuillFile::hasThumbnail(int level) const
{
    if (priv->m_file)
        return priv->m_file->hasThumbnail(level);
    else
        return false;
}

QString QuillFile::thumbnailFileName(int level) const
{
    if (priv->m_file)
        return priv->m_file->thumbnailFileName(level);
    else
        return QString();
}

bool QuillFile::exists() const
{
    if (priv->m_file)
        return priv->m_file->exists();
    else
        return false;
}

bool QuillFile::supported() const
{
    if (priv->m_file)
        return priv->m_file->supported();
    else
        return false;
}

void QuillFile::setSupported(bool supported)
{
    if (priv->m_file)
        priv->m_file->setSupported(supported);
}

void QuillFile::remove()
{
    if (priv->m_file)
        priv->m_file->remove();
}

void QuillFile::removeThumbnails()
{
    if (priv->m_file)
        priv->m_file->removeThumbnails();
}

QuillFile *QuillFile::original()
{
    QuillFile *file = new QuillFile();
    file->attach(priv->m_file->original());
    return file;
}

void QuillFile::invalidate()
{
    priv->m_file = 0;
}

bool QuillFile::isValid()
{
    return (priv->m_file != 0);
}
