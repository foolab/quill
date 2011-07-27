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
  \class TileCache

  \brief A cache used to store tiles.

TileCache should be used in a way that it would only contain tiles
from one image at a time.

TileCache can store a maximum of one tile by tile Id (see TileMap) at
a time. This also means that TileCache does not support undo. Tile
cache has an upper size limit, and any items can be removed at any time.

If there is a simultaneous request of more tiles than the tile cache
has space for, tile cache will not load new tiles.

Due to different cache policies, TileCache is not used to store
preview images - instead, ImageCache is used for that.
 */

#ifndef __QUILL_TILE_CACHE_H__
#define __QUILL_TILE_CACHE_H__

#include <QCache>

class QuillImage;
class TileCachePrivate;
class ImageTile;

class TileCache
{
public:

    /*!
      Create a tile cache.
      @param cost the maximum cache size in tiles (not bytes as in the
      preview cache)
     */
    TileCache(int cost = 100);

    /*!
      Change the maximum cache size
      @param cost the maximum cache size in tiles (not bytes as in the
      preview cache)
     */
    void resizeCache(const int cost);

    /*!
      Sets an individual tile
      @param tileId the id of the tile within its tile map
      @param tileMapId the unique id of the tile map
      @param tile the tile as a QuillImage
     */
    void setTile(int tileId, int tileMapId, const QuillImage &tile);

    /*!
      Get the total cache size
      @return cost the maximum cache size in tiles (not bytes as in the
      preview cache)
    */
    int cacheCost() const;

    /*!
      Search a key from the cache - internal use
    */
    bool searchKey(const int key) const;

    /*!
      Returns an individual tile
      @param tileId the id of the tile within its tile map
      @param tileMapId the unique id of the tile map
     */
    QuillImage tile(int tileId, int tileMapId) const;

    /*!
      Clears the tile cache.
     */
    void clear();

    ~TileCache();

private:
    QCache<int, ImageTile> m_cache;
};





#endif //__QUILL_TILE_CACHE_H__
