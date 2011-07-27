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
#include <QCache>

#include "tilemap.h"
#include "tilecache.h"

int TileMap::m_nextId = 1;

TileMap::TileMap(const QSize &fullImageSize, const QSize &tileSize,
                 TileCache* tileCache) :
    m_fullImageSize(fullImageSize), m_tiles(tileCache)
{
    m_id = m_nextId;
    m_nextId++;

    // Always rounds up
    int xTiles = (fullImageSize.width() - 1) / tileSize.width() + 1;
    int yTiles = (fullImageSize.height() - 1) / tileSize.height() + 1;
    m_tileAreas = QVector<QRect>(xTiles*yTiles);

    for (int y=0; y<yTiles; y++)
        for (int x=0; x<xTiles; x++) {
            QRect area =
                QRect(QPoint(tileSize.width()*x, tileSize.height()*y), tileSize)
                .intersected(QRect(QPoint(0, 0), fullImageSize));
            m_tileAreas[y*xTiles+x] = area;
        }
}

TileMap::TileMap(TileMap *previousMap, QuillImageFilter *filter)
{
    m_id = m_nextId;
    m_nextId++;

    m_fullImageSize =
        filter->newFullImageSize(previousMap->fullImageSize());

    int tileCount = previousMap->tileAreasCount();

    m_tileAreas = QVector<QRect>(tileCount);

    m_tiles = previousMap->tileCache();

    for (int t = 0; t<tileCount; t++)
        m_tileAreas[t] = filter->newArea(previousMap->fullImageSize(),
                                         previousMap->tileArea(t));
}

TileMap::~TileMap()
{
}

QSize TileMap::fullImageSize() const
{
    return m_fullImageSize;
}

QuillImage TileMap::tile(int index) const
{
    QuillImage image = m_tiles->tile(index, m_id);

    if (image.isNull()) {
        image.setFullImageSize(m_fullImageSize);
        image.setArea(m_tileAreas.at(index));
        return image;
    }
    else
        return image;
}

void TileMap::setTile(int index, const QuillImage &tile)
{
    m_tiles->setTile(index, m_id, tile);
}

QRect TileMap::tileArea(int index) const
{
    return m_tileAreas[index];
}

int TileMap::count() const
{
    int result = 0;

    for (int index=0; index<m_tileAreas.count(); index++)
        if (!m_tiles->tile(index, m_id).isNull())
            result++;

    return result;
}

int TileMap::find(const QPoint &point) const
{
    for (int i=0; i<m_tileAreas.count(); i++)
        if (m_tileAreas[i].contains(point))
            return i;

    return -1;
}

QList<int> TileMap::findArea(const QRect &area) const
{
    QList<int> indices;

    // Only include non-empty tile areas.
    for (int i=0; i<m_tileAreas.count(); i++)
        if (isValid(i) && (m_tileAreas[i].intersects(area)))
            indices.append(i);

    return indices;
}

static QPoint referencePoint;

int TileMap::proximity(const QRect &rect, const QPoint &point) const
{
    int xProximity = 0, yProximity = 0;

    if (point.x() < rect.left())
        xProximity = rect.left() - point.x();
    else if (point.x() > rect.right())
        xProximity = point.x() - rect.right();

    if (point.y() < rect.top())
        yProximity = rect.top() - point.y();
    else if (point.y() > rect.bottom())
        yProximity = point.y() - rect.bottom();

    return xProximity + yProximity;
}

QList<int> TileMap::sortByProximity(QList<int> indices, const QPoint &point) const
{
    for (int i = 0; i<indices.count(); i++)
        for (int j = i+1; j<indices.count(); j++)
        {
            if (proximity(m_tileAreas[indices[j]], point)
                < proximity (m_tileAreas[indices[i]], point))
                indices.swap(i, j);
    }
    return indices;
}

int TileMap::prioritize(const QRect &area) const
{
    QList<int> indices = findArea(area);
    indices = sortByProximity(indices, area.center());
    int result = -1;

    // When the result has been found,
    // we should still continue to the end
    // to prevent the others from falling out of the cache.

    for (int i = indices.count()-1; i>=0; i--)
        if (tile(indices[i]).isNull())
            result = indices[i];

    return result;
}

int TileMap::first(const QRect &area) const
{
    for (int i = 0; i<m_tileAreas.count(); i++)
        if ((!tile(i).isNull()) && (tile(i).area().intersects(area)))
            return i;
    return -1;
}

QList<QuillImage> TileMap::nonEmptyTiles(const QRect &area) const
{
    QList<QuillImage> images;
    QList<int> indices = findArea(area);

    for (int i = 0; i<indices.count(); i++)
        if (!tile(indices[i]).isNull())
            images.append(tile(indices[i]));

    return images;
}

QList<QuillImage> TileMap::newTiles(const QRect &oldArea, const QRect &newArea) const
{
    QList<QuillImage> images;
    QList<int> indices = findArea(newArea);

    for (int i = 0; i<indices.count(); i++)
        if ((!m_tileAreas[indices[i]].intersects(oldArea))
	    && (!tile(indices[i]).isNull()))
 	    images.append(tile(indices[i]));

    return images;
}


void TileMap::resizeCache(const int cost)
{
    m_tiles->resizeCache(cost);
}

int TileMap::cacheCost() const
{
    return m_tiles->cacheCost();
}

bool TileMap::searchKey(const int key) const
{
    return m_tiles->searchKey(key);
}

void TileMap::clearTileCache()
{
    m_tiles->clear();
}

int TileMap::tileAreasCount() const
{
    return m_tileAreas.size();
}

bool TileMap::isValid(int index) const
{
    QRect area = m_tileAreas[index];
    return (area.width() > 0) && (area.height() > 0);
}

int TileMap::nonEmptyTileAreasCount() const
{
    int count = 0;
    for (int i=0; i<m_tileAreas.count(); i++)
        if (isValid(i))
            count++;

    return count;
}

TileCache* TileMap::tileCache() const
{
    return m_tiles;
}
