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

#include <QuillImage>
#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include <QCache>
#include <QDebug>

#include "tilemap.h"
#include "savemap.h"

class SaveMapPrivate
{
public:
    QSize fullImageSize;
    int bufferHeight;
    int bufferId;
    QuillImage buffer;
    QList<QList<int> > tileRows;
};

SaveMap::SaveMap(const QSize &fullImageSize, int bufferSize, TileMap *tileMap)
{
    priv = new SaveMapPrivate;

    priv->fullImageSize = fullImageSize;
    priv->bufferHeight = bufferSize / fullImageSize.width();
    if (priv->bufferHeight == 0)
        priv->bufferHeight = 1;
    priv->buffer = QuillImage();
    priv->buffer.setArea(bufferArea(0));
    priv->bufferId = 0;

    for (int i=0; i<fullImageSize.height(); i+=priv->bufferHeight)
    {
        priv->tileRows.append(tileMap->findArea(QRect(0, i,
                                                      fullImageSize.width(),
                                                      priv->bufferHeight)));
    }
}

SaveMap::~SaveMap()
{
    delete priv;
}

int SaveMap::processNext(TileMap *tileMap)
{
    QListIterator<int> iterator(priv->tileRows.at(0));
    while (iterator.hasNext())
    {
        int id = iterator.next();
        if (tileMap->tile(id) != QuillImage())
            return id;
    }
    return -1;
}

int SaveMap::prioritize()
{
    return priv->tileRows.at(0).at(0);
}

QuillImageFilter *SaveMap::addToBuffer(int index)
{
    if (!priv->tileRows.at(0).contains(index))
        return 0;

    priv->tileRows[0].removeOne(index);

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("Overlay");

    filter->setOption(QuillImageFilter::CropRectangle,
                      QVariant(priv->buffer.area()));
    filter->setOption(QuillImageFilter::Background,
                      QVariant(QImage(priv->buffer)));

    return filter;
}

QuillImage SaveMap::buffer() const
{
    return priv->buffer;
}

void SaveMap::setBuffer(const QuillImage &buffer)
{
    priv->buffer = buffer;
}

void SaveMap::nextBuffer()
{
    priv->tileRows.removeAt(0);
    priv->bufferId++;
    priv->buffer = QuillImage();
    priv->buffer.setArea(bufferArea(priv->bufferId));
    priv->buffer.setFullImageSize(priv->fullImageSize);
}

bool SaveMap::isBufferComplete() const
{
    return priv->tileRows.at(0).isEmpty();
}

bool SaveMap::isSaveComplete() const
{
    return (priv->tileRows.isEmpty() ||
            ((priv->tileRows.count() == 1) &&
             (priv->tileRows.at(0).isEmpty())));
}

QRect SaveMap::bufferArea(int bufferId) const
{
    int top = bufferId*priv->bufferHeight;
    int bottom = top + priv->bufferHeight;
    if (bottom > priv->fullImageSize.height())
        bottom = priv->fullImageSize.height();
    return QRect(0, top, priv->fullImageSize.width(), bottom - top);
}

int SaveMap::bufferCount() const
{
    return (priv->fullImageSize.height() - 1) / priv->bufferHeight + 1;
}

