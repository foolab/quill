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

#include <QDebug>
#include <QtTest/QtTest>
#include <QImage>
#include <QuillImageFilter>

#include "tilemap.h"
#include "tilecache.h"
#include "unittests.h"
#include "ut_tilemap.h"

ut_tilemap::ut_tilemap(): tileCache(0)
{
}

void ut_tilemap::initTestCase()
{
    tileCache = new TileCache(50);
}

void ut_tilemap::cleanupTestCase()
{
    delete tileCache;
}

// Create a 8x2 tile map of 2x2 tiles, verify correct tiles

void ut_tilemap::testTileMap()
{
    TileMap tileMap(QSize(8, 2), QSize(2, 2),tileCache);

    QCOMPARE(tileMap.fullImageSize(),QSize(8, 2));

    QCOMPARE(tileMap.tileAreasCount(), 4);
    QCOMPARE(tileMap.tileArea(0), QRect(0, 0, 2, 2));
    QCOMPARE(tileMap.tileArea(1), QRect(2, 0, 2, 2));
    QCOMPARE(tileMap.tileArea(2), QRect(4, 0, 2, 2));
    QCOMPARE(tileMap.tileArea(3), QRect(6, 0, 2, 2));
}

// Create a 8x4 tile map of 3x3 tiles, verify correct tiles, test find
void ut_tilemap::testFind()
{
    TileMap tileMap(QSize(8, 4), QSize(3, 3),tileCache);
    QCOMPARE(tileMap.fullImageSize(), QSize(8, 4));

    QCOMPARE(tileMap.tileAreasCount(), 6);
    QCOMPARE(tileMap.tileArea(0), QRect(0, 0, 3, 3));
    QCOMPARE(tileMap.tileArea(1), QRect(3, 0, 3, 3));
    QCOMPARE(tileMap.tileArea(2), QRect(6, 0, 2, 3));
    QCOMPARE(tileMap.tileArea(3), QRect(0, 3, 3, 1));
    QCOMPARE(tileMap.tileArea(4), QRect(3, 3, 3, 1));
    QCOMPARE(tileMap.tileArea(5), QRect(6, 3, 2, 1));

    QCOMPARE(tileMap.find(QPoint(0, 0)), 0);
    QCOMPARE(tileMap.find(QPoint(1, 1)), 0);
    QCOMPARE(tileMap.find(QPoint(2, 2)), 0);
    QCOMPARE(tileMap.find(QPoint(3, 3)), 4);
    QCOMPARE(tileMap.find(QPoint(4, 2)), 1);
    QCOMPARE(tileMap.find(QPoint(5, 1)), 1);
    QCOMPARE(tileMap.find(QPoint(6, 0)), 2);
    QCOMPARE(tileMap.find(QPoint(7, 1)), 2);

    QCOMPARE(tileMap.find(QPoint(-1, -1)), -1);
}

void ut_tilemap::testEmptyTiles()
{
    TileMap tileMap(QSize(8, 2), QSize(2, 2),tileCache);
    QCOMPARE(tileMap.tileAreasCount(), 4);

    QuillImage image1(QImage(QSize(2,2),QImage::Format_ARGB32));
    image1.setFullImageSize(QSize(8,2));
    image1.setArea(QRect(0,0,2,2));
    tileMap.setTile(0,image1);

    QuillImage image2(QImage(QSize(2,2),QImage::Format_ARGB32));
    image2.setFullImageSize(QSize(8,2));
    image2.setArea(QRect(2,0,2,2));
    tileMap.setTile(1,image2);

    QuillImage image3(QImage(QSize(2,2),QImage::Format_ARGB32));
    image3.setFullImageSize(QSize(8,2));
    image3.setArea(QRect(4,0,2,2));
    tileMap.setTile(2,image3);

    QuillImage image4(QImage(QSize(2,2),QImage::Format_ARGB32));
    image4.setFullImageSize(QSize(8,2));
    image4.setArea(QRect(6,0,2,2));
    tileMap.setTile(3,image4);

    QCOMPARE(tileMap.count(),4);

    QList<QuillImage> tiles = tileMap.nonEmptyTiles(QRect(0, 0, 8, 2));

    QCOMPARE(tiles.at(0).fullImageSize(),QSize(8,2));
    QCOMPARE(tiles.at(0).area(),QRect(0,0,2,2));
    QCOMPARE(tiles.at(0),image1);

    QCOMPARE(tiles.at(1).fullImageSize(),QSize(8,2));
    QCOMPARE(tiles.at(1).area(),QRect(2,0,2,2));
    QCOMPARE(tiles.at(1),image2);

    QCOMPARE(tiles.at(2).fullImageSize(),QSize(8,2));
    QCOMPARE(tiles.at(2).area(),QRect(4,0,2,2));
    QCOMPARE(tiles.at(2),image3);

    QCOMPARE(tiles.at(3).fullImageSize(),QSize(8,2));
    QCOMPARE(tiles.at(3).area(),QRect(6,0,2,2));
    QCOMPARE(tiles.at(3),image4);
}

void ut_tilemap::testSetTile()
{
    TileMap tileMap(QSize(8,2),QSize(2,2),tileCache);
    tileMap.clearTileCache();
    tileMap.resizeCache(3);
    QCOMPARE(tileMap.cacheCost(), 3);
    QCOMPARE(tileMap.first(QRect(0, 0, 8, 2)), -1);

    QuillImage image1(QImage(QSize(2,2),QImage::Format_ARGB32));
    image1.setFullImageSize(QSize(8,2));
    image1.setArea(QRect(0,0,2,2));
    tileMap.setTile(0,image1);
    QCOMPARE(tileMap.count(),1);
    QCOMPARE(tileMap.tile(0).fullImageSize(),QSize(8,2));
    QCOMPARE(tileMap.tile(0).area(),QRect(0,0,2,2));
    QCOMPARE(tileMap.tile(0),image1);

    QCOMPARE(tileMap.first(QRect(0, 0, 8, 2)), 0);

    QuillImage image2(QImage(QSize(2,2),QImage::Format_ARGB32));
    image2.setFullImageSize(QSize(8,2));
    image2.setArea(QRect(2,0,2,2));
    tileMap.setTile(1,image2);
    QCOMPARE(tileMap.count(),2);
    QCOMPARE(tileMap.tile(1).fullImageSize(),QSize(8,2));
    QCOMPARE(tileMap.tile(1).area(),QRect(2,0,2,2));
    QCOMPARE(tileMap.tile(1),image2);

    QuillImage image3(QImage(QSize(2,2),QImage::Format_ARGB32));
    image3.setFullImageSize(QSize(8,2));
    image3.setArea(QRect(4,0,2,2));
    tileMap.setTile(2,image3);
    QCOMPARE(tileMap.count(),3);
    QCOMPARE(tileMap.tile(2).fullImageSize(),QSize(8,2));
    QCOMPARE(tileMap.tile(2).area(),QRect(4,0,2,2));
    QCOMPARE(tileMap.tile(2),image3);

    // We put a fourth image to cache.
    // One should be automatically removed from the cache

    QuillImage image4(QImage(QSize(2,2),QImage::Format_ARGB32));
    image4.setFullImageSize(QSize(8,2));
    image4.setArea(QRect(6,0,2,2));
    tileMap.setTile(3,image4);
    QCOMPARE(tileMap.count(),3);
    QCOMPARE(tileMap.tile(3).fullImageSize(),QSize(8,2));
    QCOMPARE(tileMap.tile(3).area(),QRect(6,0,2,2));
    QCOMPARE(tileMap.tile(3),image4);

    QCOMPARE((QImage)tileMap.tile(0),QImage());

    QCOMPARE(tileMap.searchKey(0),false);
    QCOMPARE(tileMap.searchKey(1),true);
    QCOMPARE(tileMap.searchKey(2),true);
    QCOMPARE(tileMap.searchKey(3),true);
    QCOMPARE(tileMap.searchKey(4),false);
}

void ut_tilemap::testMultiple()
{
    TileMap tileMap(QSize(8,2), QSize(2,2), tileCache);
    tileMap.resizeCache(4);
    QCOMPARE(tileMap.cacheCost(), 4);

    TileMap tileMap2(QSize(8,2), QSize(2,2), tileCache);
    QCOMPARE(tileMap.cacheCost(), 4);

    QuillImage image1(QImage(QSize(2,2),QImage::Format_ARGB32));
    image1.setFullImageSize(QSize(8,2));
    image1.setArea(QRect(0,0,2,2));
    image1.fill(qRgba(0, 0, 0, 0));

    QuillImage image2(QImage(QSize(2,2),QImage::Format_ARGB32));
    image2.setFullImageSize(QSize(8,2));
    image2.setArea(QRect(2,0,2,2));
    image2.fill(qRgba(128, 0, 0, 0));

    QuillImage image3(QImage(QSize(2,2),QImage::Format_ARGB32));
    image3.setFullImageSize(QSize(8,2));
    image3.setArea(QRect(4,0,2,2));
    image3.fill(qRgba(256, 0, 0, 0));

    tileMap.setTile(0, image1);
    tileMap2.setTile(1, image2);

    // Now the first image should be accessible from map 1 and
    // the other one from map 2.

    QCOMPARE(tileMap.tile(0), image1);
    QCOMPARE((QImage)tileMap2.tile(0), QImage());

    QCOMPARE((QImage)tileMap.tile(1), QImage());
    QCOMPARE(tileMap2.tile(1), image2);

    tileMap2.setTile(0, image3);

    // Now, images 2 and 3 should be accessible from map 2,
    // and none from map 1.

    QCOMPARE((QImage)tileMap.tile(0), QImage());
    QCOMPARE(tileMap2.tile(0), image3);

    QCOMPARE((QImage)tileMap.tile(1), QImage());
    QCOMPARE(tileMap2.tile(1), image2);
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_tilemap test;
    return QTest::qExec( &test, argc, argv );
}
