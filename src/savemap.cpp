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

#include <QuillImage>
#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include <QCache>

#include "tilemap.h"
#include "savemap.h"

SaveMap::SaveMap(const QSize &fullImageSize, int bufferSize, TileMap *tileMap) :
    m_fullImageSize(fullImageSize),
    m_bufferHeight(bufferSize / fullImageSize.width()),
    m_bufferId(0),
    m_buffer(QuillImage())
{
    if (m_bufferHeight == 0)
        m_bufferHeight = 1;

    m_buffer.setArea(bufferArea(0));

    for (int i=0; i<fullImageSize.height(); i+=m_bufferHeight)
    {
        m_tileRows.append(tileMap->findArea(QRect(0, i,
                                                  fullImageSize.width(),
                                                  m_bufferHeight)));
    }
}

SaveMap::~SaveMap()
{
}

int SaveMap::processNext(TileMap *tileMap)
{
    QListIterator<int> iterator(m_tileRows.at(0));
    while (iterator.hasNext())
    {
        int id = iterator.next();
        if (!tileMap->tile(id).isNull())
            return id;
    }
    return -1;
}

int SaveMap::prioritize()
{
    return m_tileRows.at(0).at(0);
}

QuillImageFilter *SaveMap::addToBuffer(int index)
{
    if (!m_tileRows.at(0).contains(index))
        return 0;

    m_tileRows[0].removeOne(index);

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Role_Overlay);

    filter->setOption(QuillImageFilter::CropRectangle,
                      QVariant(m_buffer.area()));
    filter->setOption(QuillImageFilter::Background,
                      QVariant(QImage(m_buffer)));

    // So that implicit sharing does not copy the buffer
    m_buffer = QImage();

    return filter;
}

QuillImage SaveMap::buffer() const
{
    return m_buffer;
}

void SaveMap::setBuffer(const QuillImage &buffer)
{
    m_buffer = buffer;
}

void SaveMap::nextBuffer()
{
    m_tileRows.removeAt(0);
    m_bufferId++;
    m_buffer = QuillImage();
    m_buffer.setArea(bufferArea(m_bufferId));
    m_buffer.setFullImageSize(m_fullImageSize);
}

bool SaveMap::isBufferComplete() const
{
    return (m_tileRows.isEmpty() ||
            m_tileRows.at(0).isEmpty());
}

bool SaveMap::isSaveComplete() const
{
    return (m_tileRows.isEmpty() ||
            ((m_tileRows.count() == 1) &&
             (m_tileRows.at(0).isEmpty())));
}

QRect SaveMap::bufferArea(int bufferId) const
{
    int top = bufferId*m_bufferHeight;
    int bottom = top + m_bufferHeight;
    if (bottom > m_fullImageSize.height())
        bottom = m_fullImageSize.height();
    return QRect(0, top, m_fullImageSize.width(), bottom - top);
}

int SaveMap::bufferCount() const
{
    return (m_fullImageSize.height() - 1) / m_bufferHeight + 1;
}

