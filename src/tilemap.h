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

/*!
  \class TileMap

  \brief Contains the positions of individual tiles related to a
  QuillUndoCommand

For each image related to one state in an edit history (ie
QuillUndoCommand) there is a tile map which contains the tile
positions in full image coordinates. The tile is identified with its
tile id, which stays the same from a tile map to the next one in the
edit history. If we want to calculate a tile, we can calculate it
completely from a predecessor with the same tile id (as there is
currently no information transfer between different tiles).
 */

#ifndef __QUILL_TILE_MAP_H_
#define __QUILL_TILE_MAP_H_

#include <QSize>
#include <QRect>
#include <QuillImage>

class QImage;
class QuillImageFilter;

class TileMapPrivate;
class TileCache;

class TileMap
{
public:
    /*!
      Creates a tile map for a new stack prior to invoking load filter.

      @param fullImageSize the size of a full image.
      @param tileSize size of an individual tile.
     */

    TileMap(const QSize &fullImageSize, const QSize &tileSize, TileCache* tileCache);

    /*!
      Creates a tile map by applying a filter to a previous tile map.

      @param previousMap pointer to the previous tile map.
      @param filter the filter to be applied.
     */

    TileMap(TileMap *previousMap, QuillImageFilter *filter);

    /*!
     */

    ~TileMap();

    /*!
      Returns the size of the full image represented by the tile map.
     */

    QSize fullImageSize() const;

    /*!
      Returns an individual tile by its index.
    */

    QuillImage tile(int index) const;

    /*!
      Sets the value of a tile by index.
    */

    void setTile(int index, const QuillImage &tile);

    /*!
      Returns the area of a tile by index (internal use only).
    */

    QRect tileArea(int id) const;

    /*!
      Returns the number of non-empty tiles in the cache.
     */

    int count() const;

    /*!
      Finds the first tile to cover a given point.
      @return index of the tile.
     */

    int find(const QPoint &point) const;

    /*!
      Finds all tiles which cover parts of the given area.
     */

    QList<int> findArea(const QRect &area) const;

    /*!
      Finds the first tile in the area which is not in the cache.
      As a side-effect, raises the priority of all other tiles in the
      area so that they don't get removed from the cache.

      @param area the area which the tile must cover, at least partially.
      @return the index for the first empty tile.
     */

    int prioritize(const QRect &area) const;

    /*!
      Returns the index of the first element in the cache.
     */

    int first(const QRect &area) const;

    /*!
      Returns all non-empty tiles within the area.
      As a side-effect, raises the priority of all other tiles in the
      area so that they don't get removed from the cache.

      @param area the area which the tile must cover, at least partially.
    */

    QList<QuillImage> nonEmptyTiles(const QRect &area) const;

    /*!
      As in nonEmptyTiles(), but returned tiles may not be part of
      oldArea.
    */

    QList<QuillImage> newTiles(const QRect &oldArea, const QRect &newArea) const;

    /*!
      Resize the internal cache that contains the tiles.
    */

    void resizeCache(const int cost);

    /*!
      Get the maximum cache size.
    */
    int cacheCost() const;

    /*!
      Search for a given tile id.
      @param key the tile id.
    */
    bool searchKey(const int key) const;

    /*!
      Clears the whole internal cache.
     */
    void clearTileCache();

    /*!
      Count of all tiles (empty, non-empty, and invalid) in the tile map.
     */
    int tileAreasCount() const;

    /*
      Count of all valid tiles (empty and non-empty) in the tile map.
     */
    int nonEmptyTileAreasCount() const;

    /*!
      Shares the internal tile cache. Internal use (copy constructor) only.
     */

    TileCache* tileCache() const;

private:
    
    /*!
      If the corresponding tile area is valid (ie. has at least one pixel.)
     */

    bool isValid(int index) const;

    /*!
      Returns the distance of the point from the center of the rectangle,
      this is used for comparison purposes.
     */

    int proximity(const QRect &rect, const QPoint &point) const;

    /*!
      Sorts the list of indices by the proximity of their respective tile
      areas to the center point of the area.
     */

    QList<int> sortByProximity(QList<int> indices, const QPoint &point) const;

 private:
    QSize m_fullImageSize;
    TileCache* m_tiles;

    QVector<QRect> m_tileAreas;

    int m_id;
    static int m_nextId;
};

#endif // __QUILL_TILE_MAP_H_
