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
#include "logger.h"

Quill* Quill:: g_instance = 0;

QSize Quill::defaultViewPortSize = QSize(640, 400);

void Quill::initTestingMode()
{
    Core::initTestingMode();
}

void Quill::cleanup()
{
    Core::cleanup();
    delete g_instance;
    g_instance = 0;
}

void Quill::setDefaultTileSize(const QSize &defaultTileSize)
{
    Core::instance()->setDefaultTileSize(defaultTileSize);
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO)+Logger::qsizeToString(defaultTileSize));
    }

void Quill::setTileCacheSize(int size)
{
    Core::instance()->setTileCacheSize(size);
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO)+Logger::intToString(size));
}

void Quill::setSaveBufferSize(int size)
{
    Core::instance()->setSaveBufferSize(size);
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO)+Logger::intToString(size));
}

void Quill::setFileLimit(int level, int limit)
{
    Core::instance()->setFileLimit(level, limit);
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO)+Logger::intToString(level)+Logger::intToString(limit));
}

int Quill::fileLimit(int level)
{
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO)+Logger::intToString(level));
    return Core::instance()->fileLimit(level);
}

void Quill::setEditHistoryCacheSize(int level, int limit)
{
    Core::instance()->setEditHistoryCacheSize(level, limit);
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO)+Logger::intToString(level)+Logger::intToString(limit));
}

int Quill::editHistoryCacheSize(int level)
{
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO)+Logger::intToString(level));
    return Core::instance()->editHistoryCacheSize(level);
}

void Quill::setPreviewLevelCount(int count)
{
    Core::instance()->setPreviewLevelCount(count);
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO)+Logger::intToString(count));
}

int Quill::previewLevelCount()
{
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO));
    return Core::instance()->previewLevelCount();
}

void Quill::setPreviewSize(int level, const QSize &size)
{
    Core::instance()->setPreviewSize(level, size);
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO)+Logger::intToString(level)+Logger::qsizeToString(size));
}

QSize Quill::previewSize(int level)
{
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO)+Logger::intToString(level));
    return Core::instance()->previewSize(level);
}

void Quill::setEditHistoryDirectory(const QString &directory)
{
    Core::instance()->setEditHistoryDirectory(directory);
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO)+directory);
}

void Quill::setThumbnailDirectory(int level, const QString &directory)
{
    Core::instance()->setThumbnailDirectory(level, directory);
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO)+Logger::intToString(level)+directory);
}

void Quill::setThumbnailExtension(const QString &extension)
{
    Core::instance()->setThumbnailExtension(extension);
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO)+extension);
}

void Quill::setThumbnailCreationEnabled(bool enabled)
{
    Core::instance()->setThumbnailCreationEnabled(enabled);
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO)+Logger::boolToString(enabled));
}

bool Quill::isThumbnailCreationEnabled()
{
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO));
    return Core::instance()->isThumbnailCreationEnabled();
}

void Quill::setTemporaryFilePath(const QString &tmpFilePath)
{
    Core::instance()->setTemporaryFileDirectory(tmpFilePath);
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO)+tmpFilePath);
}

QString Quill::temporaryFilePath()
{
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO));
    return Core::instance()->temporaryFileDirectory();
}

void Quill::setCrashDumpPath(const QString &fileName)
{
    Core::instance()->setCrashDumpPath(fileName);
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO)+fileName);
}

QString Quill::crashDumpPath()
{
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO));
    return Core::instance()->crashDumpPath();
}

bool Quill::canRecover()
{
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO));
    return Core::instance()->canRecover();
}

void Quill::recover()
{
    Core::instance()->recover();
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO));
}

bool Quill::isSaveInProgress()
{
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO));
    return Core::instance()->isSaveInProgress();
}

void Quill::releaseAndWait()
{
    Core::instance()->releaseAndWait();
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO));
}

void Quill::setDebugDelay(int delay)
{
    Core::instance()->setDebugDelay(delay);
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO)+Logger::intToString(delay));
}

Quill* Quill::instance()
{
    if (g_instance == 0) {
        g_instance = new Quill;
        g_instance->connect(Core::instance(),
                            SIGNAL(saved(QString)),
                            SIGNAL(saved(QString)));
        g_instance->connect(Core::instance(),
                            SIGNAL(error(QuillError)),
                            SIGNAL(error(QuillError)));
    }
    if(Logger::existLog())
        Logger::log("[Quill] "+QString(Q_FUNC_INFO));
    return g_instance;
}
