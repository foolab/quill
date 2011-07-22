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

#include "imagecache.h"
#include "displaylevel.h"

DisplayLevel::DisplayLevel(const QSize &size) : m_size(size),
                                                m_imageCache(new ImageCache(0))
{
}

DisplayLevel::~DisplayLevel()
{
    delete m_imageCache;
}

void DisplayLevel::setSize(const QSize &size)
{
    m_size = size;
}

QSize DisplayLevel::size() const
{
    return m_size;
}

void DisplayLevel::setMinimumSize(const QSize &size)
{
    m_minimumSize = size;
}

QSize DisplayLevel::minimumSize() const
{
    return m_minimumSize;
}

bool DisplayLevel::isCropped() const
{
    return m_minimumSize.isValid();
}

void DisplayLevel::setThumbnailFlavorName(const QString &name)
{
    m_thumbnailFlavorName = name;
}

QString DisplayLevel::thumbnailFlavorName() const
{
    return m_thumbnailFlavorName;
}

ImageCache *DisplayLevel::imageCache() const
{
    return m_imageCache;
}

QSize DisplayLevel::targetSize(const QSize &fullImageSize) const
{
    return scaleBounding(fullImageSize, m_size, m_minimumSize).
        boundedTo(fullImageSize);
}

QSize DisplayLevel::scaleBounding(const QSize &size, const QSize &boundingBox,
                                  const QSize &minimum)
{
    QSize targetSize;

    int targetWidth = (boundingBox.height() * size.width()
                       + size.height() - 1) / size.height();

    if (targetWidth <= boundingBox.width() && targetWidth >= minimum.width())
        targetSize = QSize(targetWidth, boundingBox.height());
    else if (targetWidth < minimum.width())
        targetSize = QSize(minimum.width(), boundingBox.height());
    else {
        int targetHeight = (boundingBox.width() * size.height()
                            + size.width() - 1) / size.width();
        if (targetHeight >= minimum.height())
            targetSize = QSize(boundingBox.width(), targetHeight);
        else
            targetSize = QSize(boundingBox.width(), minimum.height());
    }
    return targetSize;
}

QRect DisplayLevel::targetArea(const QSize &targetSize,
                               const QSize &fullImageSize) const
{
    if (!m_minimumSize.isValid())
        return QRect(QPoint(0, 0), fullImageSize);

    QSize size;
    //If the full image size is smaller than the mini size, we will not scale it when generating thumbail
    if(fullImageSize.width()<= m_minimumSize.width()&&fullImageSize.height()<= m_minimumSize.height())
        return QRect(QPoint(0,0),fullImageSize);
    //If only one direction is less than mini size, we will not scale it either, we just take an area 172xX or Xx172
    if(fullImageSize.width()<=m_minimumSize.width()||fullImageSize.height()<= m_minimumSize.height()){
        if(fullImageSize.width()<=m_minimumSize.width())
            return QRect(0,fullImageSize.height()/2-m_minimumSize.height()/2,fullImageSize.width(),m_minimumSize.height());
        else
            return QRect(fullImageSize.width()/2-m_minimumSize.width()/2,0,m_minimumSize.width(),fullImageSize.height());
    }
    if (targetSize.width() * fullImageSize.height() >
        targetSize.height() * fullImageSize.width())
        size = QSize(fullImageSize.width(),
                     (targetSize.height() * fullImageSize.width()
                      + m_size.width() - 1) / m_size.width());
    else
        size = QSize((targetSize.width() * fullImageSize.height()
                      + m_size.height() - 1) / m_size.height(),
                     fullImageSize.height());

    return QRect((fullImageSize.width() - size.width()) / 2,
                 (fullImageSize.height() - size.height()) / 2,
                 size.width(), size.height());
}
