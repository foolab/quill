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

#include <QImage>
#include <QImageReader>
#include <QCryptographicHash>
#include <QByteArray>
#include <QDir>
#include <QuillImageFilter>
#include <QDebug>
#include "quill.h"
#include "quillfile.h"
#include "core.h"
#include "threadmanager.h"

class QuillPrivate
{
public:
    Core *core;
    static Quill *instance;
};

QSize Quill::defaultViewPortSize = QSize(640, 400);
Quill *QuillPrivate::instance = 0;

Quill::Quill(const QSize &viewPortSize,
             Quill::ThreadingMode threadingMode)
{
    qRegisterMetaType<QuillImage>("QuillImage");
    qRegisterMetaType<QuillImageList>("QuillImageList");

    priv = new QuillPrivate();
    priv->core = new Core(viewPortSize, threadingMode);

    QuillImageFilter::registerAll();
}

Quill::~Quill()
{
    delete priv->core;
    delete priv;
    QuillPrivate::instance = 0;
}

Quill *Quill::instance()
{
    if (QuillPrivate::instance == 0)
        QuillPrivate::instance = new Quill();

    return QuillPrivate::instance;
}

QuillFile *Quill::file(const QString &fileName, const QString &fileFormat)
{
    return priv->core->file(fileName, fileFormat);
}

void Quill::setDefaultTileSize(const QSize &defaultTileSize)
{
    priv->core->setDefaultTileSize(defaultTileSize);
}

void Quill::setTileCacheSize(int size)
{
    priv->core->setTileCacheSize(size);
}

void Quill::setSaveBufferSize(int size)
{
    priv->core->setSaveBufferSize(size);
}

void Quill::setFileLimit(int level, int limit)
{
    priv->core->setFileLimit(level, limit);
}

int Quill::fileLimit(int level) const
{
    return priv->core->fileLimit(level);
}

void Quill::setEditHistoryCacheSize(int level, int limit)
{
    priv->core->setEditHistoryCacheSize(level, limit);
}

int Quill::editHistoryCacheSize(int level) const
{
    return priv->core->editHistoryCacheSize(level);
}

void Quill::setPreviewLevelCount(int count)
{
    priv->core->setPreviewLevelCount(count);
}

int Quill::previewLevelCount() const
{
    return priv->core->previewLevelCount();
}

void Quill::setPreviewSize(int level, QSize size)
{
    priv->core->setPreviewSize(level, size);
}

QSize Quill::previewSize(int level) const
{
    return priv->core->previewSize(level);
}

QByteArray Quill::dump()
{
    return priv->core->dump();
}

void Quill::recover(QByteArray history)
{
    priv->core->recover(history);
}

void Quill::setEditHistoryDirectory(const QString &directory)
{
    priv->core->setEditHistoryDirectory(directory);
}

void Quill::setThumbnailDirectory(int level, const QString &directory)
{
    priv->core->setThumbnailDirectory(level, directory);
}

void Quill::setThumbnailExtension(const QString &extension)
{
    priv->core->setThumbnailExtension(extension);
}

void Quill::setThumbnailCreationEnabled(bool enabled)
{
    priv->core->setThumbnailCreationEnabled(enabled);
}

bool Quill::isThumbnailCreationEnabled() const
{
    return priv->core->isThumbnailCreationEnabled();
}

void Quill::releaseAndWait()
{
    priv->core->releaseAndWait();
}

void Quill::setDebugDelay(int delay)
{
    priv->core->setDebugDelay(delay);
}

void Quill::setTemporaryFilePath(const QString tmpFilePath)
{
    priv->core->setTemporaryFileDirectory(tmpFilePath);
}
