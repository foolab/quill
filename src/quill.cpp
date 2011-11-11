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

#include <QImage>
#include <QImageReader>
#include <QCryptographicHash>
#include <QByteArray>
#include <QColor>
#include <QDir>
#include <QuillImageFilter>
#include "quill.h"
#include "core.h"
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
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO)+Logger::qsizeToString(defaultTileSize));
    }

void Quill::setTileCacheSize(int size)
{
    Core::instance()->setTileCacheSize(size);
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO)+Logger::intToString(size));
}

void Quill::setSaveBufferSize(int size)
{
    Core::instance()->setSaveBufferSize(size);
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO)+Logger::intToString(size));
}

void Quill::setEditHistoryCacheSize(int level, int limit)
{
    Core::instance()->setEditHistoryCacheSize(level, limit);
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO)+Logger::intToString(level)+Logger::intToString(limit));
}

int Quill::editHistoryCacheSize(int level)
{
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO)+Logger::intToString(level));
    return Core::instance()->editHistoryCacheSize(level);
}

void Quill::setPreviewLevelCount(int count)
{
    Core::instance()->setPreviewLevelCount(count);
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO)+Logger::intToString(count));
}

int Quill::previewLevelCount()
{
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO));
    return Core::instance()->previewLevelCount();
}

void Quill::setPreviewSize(int level, const QSize &size)
{
    Core::instance()->setPreviewSize(level, size);
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO)+Logger::intToString(level)+Logger::qsizeToString(size));
}

QSize Quill::previewSize(int level)
{
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO)+Logger::intToString(level));
    return Core::instance()->previewSize(level);
}

void Quill::setMinimumPreviewSize(int level, const QSize &size)
{
    Core::instance()->setMinimumPreviewSize(level, size);
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO)+Logger::intToString(level)+Logger::qsizeToString(size));
}

QSize Quill::minimumPreviewSize(int level)
{
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO)+Logger::intToString(level));
    return Core::instance()->minimumPreviewSize(level);
}

void Quill::setImageSizeLimit(const QSize &size)
{
    Core::instance()->setImageSizeLimit(size);
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO)+Logger::qsizeToString(size));
}

QSize Quill::imageSizeLimit()
{
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO));
    return Core::instance()->imageSizeLimit();
}

void Quill::setImagePixelsLimit(int pixels)
{
    Core::instance()->setImagePixelsLimit(pixels);
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO)+Logger::intToString(pixels));
}

int Quill::imagePixelsLimit()
{
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO));
    return Core::instance()->imagePixelsLimit();
}

void Quill::setNonTiledImagePixelsLimit(int pixels)
{
    Core::instance()->setNonTiledImagePixelsLimit(pixels);
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO)+Logger::intToString(pixels));
}

int Quill::nonTiledImagePixelsLimit()
{
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO));
    return Core::instance()->nonTiledImagePixelsLimit();
}

void Quill::setEditHistoryPath(const QString &path)
{
    Core::instance()->setEditHistoryPath(path);
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO)+path);
}

void Quill::setThumbnailBasePath(const QString &path)
{
    Core::instance()->setThumbnailBasePath(path);
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO)+path);
}

void Quill::setThumbnailFlavorName(int level, const QString &name)
{
    Core::instance()->setThumbnailFlavorName(level, name);
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO)+Logger::intToString(level)+" "+name);
}

void Quill::setThumbnailExtension(const QString &extension)
{
    Core::instance()->setThumbnailExtension(extension);
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO)+extension);
}

void Quill::setThumbnailCreationEnabled(bool enabled)
{
    Core::instance()->setThumbnailCreationEnabled(enabled);
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO)+Logger::boolToString(enabled));
}

bool Quill::isThumbnailCreationEnabled()
{
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO));
    return Core::instance()->isThumbnailCreationEnabled();
}

void Quill::setDBusThumbnailingEnabled(bool enabled)
{
    Core::instance()->setDBusThumbnailingEnabled(enabled);
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO)+Logger::boolToString(enabled));
}

bool Quill::isDBusThumbnailingEnabled()
{
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO));
    return Core::instance()->isDBusThumbnailingEnabled();
}

void Quill::setBackgroundRenderingColor(const QColor &color)
{
    Core::instance()->setBackgroundRenderingColor(color);
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO));
}

QColor Quill::backgroundRenderingColor()
{
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO));
    return Core::instance()->backgroundRenderingColor();
}

void Quill::setVectorGraphicsRenderingSize(const QSize &size)
{
    Core::instance()->setVectorGraphicsRenderingSize(size);
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO));
}

QSize Quill::vectorGraphicsRenderingSize()
{
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO));
    return Core::instance()->vectorGraphicsRenderingSize();
}

void Quill::setTemporaryFilePath(const QString &path)
{
    Core::instance()->setTemporaryFilePath(path);
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO)+path);
}

QString Quill::temporaryFilePath()
{
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO));
    return Core::instance()->temporaryFilePath();
}

bool Quill::isCalculationInProgress()
{
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO));
    return Core::instance()->isCalculationInProgress();
}

bool Quill::isSaveInProgress()
{
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO)+Logger::boolToString(Core::instance()->isSaveInProgress()));
    return Core::instance()->isSaveInProgress();
}

QStringList Quill::saveInProgressList()
{
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO));
    return Core::instance()->saveInProgressList();
}

QStringList Quill::lockedFiles()
{
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO));
    return Core::instance()->lockedFiles();
}

bool Quill::waitUntilFinished(int msec)
{
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO));
    return Core::instance()->waitUntilFinished(msec);
}

void Quill::releaseAndWait()
{
    Core::instance()->releaseAndWait();
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO));
}

Quill* Quill::instance()
{
    if (g_instance == 0) {
        g_instance = new Quill;
        g_instance->connect(Core::instance(),
                            SIGNAL(saved(QString)),
                            SIGNAL(saved(QString)));
        g_instance->connect(Core::instance(),
                            SIGNAL(removed(QString)),
                            SIGNAL(removed(QString)));
        g_instance->connect(Core::instance(),
                            SIGNAL(error(QuillError)),
                            SIGNAL(error(QuillError)));
    }
    QUILL_LOG(Logger::Module_Quill, QString(Q_FUNC_INFO));
    return g_instance;
}

Quill::Quill() : priv(0)
{
}

Quill::~Quill()
{
}
