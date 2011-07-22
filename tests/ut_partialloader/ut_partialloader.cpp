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

#include <QtTest/QtTest>
#include <QImage>
#include <QDebug>
#include <Quill>
#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include "quillfile.h"
#include "tilemap.h"
#include "tilecache.h"
#include "unittests.h"
#include "ut_partialloader.h"
#include "../../src/strings.h"


ut_partialloader::ut_partialloader()
{
}

void ut_partialloader::initTestCase()
{
}

void ut_partialloader::cleanupTestCase()
{
}

void ut_partialloader::init()
{
    Quill::initTestingMode();
    Quill::setPreviewSize(0, QSize(4, 1));
}

void ut_partialloader::cleanup()
{
    Quill::cleanup();
}

void ut_partialloader::testFilter()
{
    QTemporaryFile testFile;
    testFile.open();

    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    TileCache *tileCache = new TileCache(100);
    QVERIFY(tileCache);

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Role_Load);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::FileName,
                      testFile.fileName());

    TileMap tileMap(QSize(8, 2), QSize(2, 2), tileCache);

    for (int i=0; i<4; i++) {
        QCOMPARE(tileMap.tile(i).fullImageSize(), QSize(8, 2));
        QuillImage tile = filter->apply(tileMap.tile(i));
        QCOMPARE(tile.fullImageSize(), QSize(8, 2));;
        tileMap.setTile(i, tile);
        QCOMPARE(tileMap.tile(i).fullImageSize(), QSize(8, 2));
    }

    QImage image = Unittests::generatePaletteImage();

    QCOMPARE(tileMap.tile(0).area(), QRect(0, 0, 2, 2));
    QCOMPARE((QImage)tileMap.tile(0), image.copy(0, 0, 2, 2));

    QCOMPARE(tileMap.tile(1).area(), QRect(2, 0, 2, 2));
    QCOMPARE((QImage)tileMap.tile(1), image.copy(2, 0, 2, 2));

    QCOMPARE(tileMap.tile(2).area(), QRect(4, 0, 2, 2));
    QCOMPARE((QImage)tileMap.tile(2), image.copy(4, 0, 2, 2));

    QCOMPARE(tileMap.tile(3).area(), QRect(6, 0, 2, 2));
    QCOMPARE((QImage)tileMap.tile(3), image.copy(6, 0, 2, 2));

    delete filter;
    delete tileCache;
}

void ut_partialloader::testQuill()
{
    QTemporaryFile testFile;
    testFile.open();

    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    QImage referenceImage = Unittests::generatePaletteImage();

    Quill::setDefaultTileSize(QSize(2, 2));

    QuillFile *file =
        new QuillFile(testFile.fileName());

    file->setViewPort(QRect(-8, -2, 16, 4));

    file->setDisplayLevel(1);

    Quill::releaseAndWait(); // preview

    QCOMPARE(file->allImageLevels().count(), 1);

    QuillImage previewImage = file->allImageLevels().at(0);

    QCOMPARE(previewImage.size(), QSize(4, 1));
    QCOMPARE(previewImage.fullImageSize(), QSize(8, 2));

    Quill::releaseAndWait();

    QCOMPARE(file->allImageLevels().count(), 2);
    QuillImage fragment = file->allImageLevels().at(1);

    QCOMPARE(fragment.size(), QSize(2, 2));
    QCOMPARE(fragment.fullImageSize(), QSize(8, 2));
    QCOMPARE(fragment.area(), QRect(0, 0, 2, 2));
    QCOMPARE((QImage)fragment, referenceImage.copy(0, 0, 2, 2));

    Quill::releaseAndWait();

    QCOMPARE(file->allImageLevels().count(), 3);
    fragment = file->allImageLevels().at(2);

    QCOMPARE(fragment.size(), QSize(2, 2));
    QCOMPARE(fragment.fullImageSize(), QSize(8, 2));
    QCOMPARE(fragment.area(), QRect(2, 0, 2, 2));
    QCOMPARE((QImage)fragment, referenceImage.copy(2, 0, 2, 2));

    Quill::releaseAndWait();

    QCOMPARE(file->allImageLevels().count(), 4);
    fragment = file->allImageLevels().at(3);

    QCOMPARE(fragment.size(), QSize(2, 2));
    QCOMPARE(fragment.fullImageSize(), QSize(8, 2));
    QCOMPARE(fragment.area(), QRect(4, 0, 2, 2));
    QCOMPARE((QImage)fragment, referenceImage.copy(4, 0, 2, 2));

    Quill::releaseAndWait();

    QCOMPARE(file->allImageLevels().count(), 5);
    fragment = file->allImageLevels().at(4);

    QCOMPARE(fragment.size(), QSize(2, 2));
    QCOMPARE(fragment.fullImageSize(), QSize(8, 2));
    QCOMPARE(fragment.area(), QRect(6, 0, 2, 2));
    QCOMPARE((QImage)fragment, referenceImage.copy(6, 0, 2, 2));

    delete file;
}

// Load + brightness

void ut_partialloader::testMultiOperation()
{
    QTemporaryFile testFile;
    testFile.open();

    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    QImage originalImage = Unittests::generatePaletteImage();

    Quill::setDefaultTileSize(QSize(2, 2));

    QuillFile *file =
        new QuillFile(testFile.fileName());

    file->setViewPort(QRect(-8, -2, 16, 4));
    file->setDisplayLevel(1);

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    filter->setOption(QuillImageFilter::Brightness,
                      QVariant(20));

    QImage referenceImage = filter->apply(originalImage);

    file->runFilter(filter);
    Quill::releaseAndWait(); // preview load
    Quill::releaseAndWait(); // preview filter

    QCOMPARE(file->allImageLevels().count(), 1);

    QuillImage previewImage = file->allImageLevels().at(0);

    QCOMPARE(previewImage.size(), QSize(4, 1));
    QCOMPARE(previewImage.fullImageSize(), QSize(8, 2));

    Quill::releaseAndWait();
    QCOMPARE(file->allImageLevels().count(), 1);

    Quill::releaseAndWait();
    QCOMPARE(file->allImageLevels().count(), 2);
    QuillImage fragment = file->allImageLevels().at(1);

    QCOMPARE(fragment.size(), QSize(2, 2));
    QCOMPARE(fragment.fullImageSize(), QSize(8, 2));
    QCOMPARE(fragment.area(), QRect(0, 0, 2, 2));
    QCOMPARE((QImage)fragment, referenceImage.copy(0, 0, 2, 2));

    Quill::releaseAndWait();
    QCOMPARE(file->allImageLevels().count(), 2);

    Quill::releaseAndWait();
    QCOMPARE(file->allImageLevels().count(), 3);
    fragment = file->allImageLevels().at(2);

    QCOMPARE(fragment.size(), QSize(2, 2));
    QCOMPARE(fragment.fullImageSize(), QSize(8, 2));
    QCOMPARE(fragment.area(), QRect(2, 0, 2, 2));
    QCOMPARE((QImage)fragment, referenceImage.copy(2, 0, 2, 2));

    Quill::releaseAndWait();
    QCOMPARE(file->allImageLevels().count(), 3);

    Quill::releaseAndWait();
    QCOMPARE(file->allImageLevels().count(), 4);
    fragment = file->allImageLevels().at(3);

    QCOMPARE(fragment.size(), QSize(2, 2));
    QCOMPARE(fragment.fullImageSize(), QSize(8, 2));
    QCOMPARE(fragment.area(), QRect(4, 0, 2, 2));
    QCOMPARE((QImage)fragment, referenceImage.copy(4, 0, 2, 2));

    Quill::releaseAndWait();
    QCOMPARE(file->allImageLevels().count(), 4);

    Quill::releaseAndWait();
    QCOMPARE(file->allImageLevels().count(), 5);
    fragment = file->allImageLevels().at(4);

    QCOMPARE(fragment.size(), QSize(2, 2));
    QCOMPARE(fragment.fullImageSize(), QSize(8, 2));
    QCOMPARE(fragment.area(), QRect(6, 0, 2, 2));
    QCOMPARE((QImage)fragment, referenceImage.copy(6, 0, 2, 2));

    delete file;
}

// Test case: tiling approach with just a single tile
void ut_partialloader::testSingleTile()
{
    QTemporaryFile testFile;
    testFile.open();

    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    QImage originalImage = Unittests::generatePaletteImage();

    Quill::setDefaultTileSize(QSize(10, 10));

    QuillFile *file =
        new QuillFile(testFile.fileName());
    file->setViewPort(QRect(0, 0, 8, 2));
    file->setDisplayLevel(1);

    Quill::releaseAndWait();
    Quill::releaseAndWait();

    QCOMPARE(file->allImageLevels().count(), 2);

    QuillImage image = file->allImageLevels().at(1);

    QVERIFY(Unittests::compareImage(image,
            QuillImage(Unittests::generatePaletteImage())));

    delete file;
}

void ut_partialloader::testCenterTilePriority()
{
    QTemporaryFile testFile;
    testFile.open();

    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    QImage originalImage = Unittests::generatePaletteImage();

    Quill::setDefaultTileSize(QSize(1, 1));

    QuillFile *file =
        new QuillFile(testFile.fileName());
    file->setViewPort(QRect(0, 0, 3, 2));
    file->setDisplayLevel(1);

    Quill::releaseAndWait();
    Quill::releaseAndWait();

    QCOMPARE(file->allImageLevels().count(), 2);
    QCOMPARE(file->allImageLevels().at(1).area(), QRect(1, 0, 1, 1));

    // Immediate surroundings

    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    QCOMPARE(file->allImageLevels().count(), 5);
    QCOMPARE(file->allImageLevels().at(1).area(), QRect(0, 0, 1, 1));
    QCOMPARE(file->allImageLevels().at(2).area(), QRect(1, 0, 1, 1));
    QCOMPARE(file->allImageLevels().at(3).area(), QRect(2, 0, 1, 1));
    QCOMPARE(file->allImageLevels().at(4).area(), QRect(1, 1, 1, 1));

    // Whole image

    Quill::releaseAndWait();
    Quill::releaseAndWait();

    QCOMPARE(file->allImageLevels().count(), 7);
    QCOMPARE(file->allImageLevels().at(1).area(), QRect(0, 0, 1, 1));
    QCOMPARE(file->allImageLevels().at(2).area(), QRect(1, 0, 1, 1));
    QCOMPARE(file->allImageLevels().at(3).area(), QRect(2, 0, 1, 1));
    QCOMPARE(file->allImageLevels().at(4).area(), QRect(0, 1, 1, 1));
    QCOMPARE(file->allImageLevels().at(5).area(), QRect(1, 1, 1, 1));
    QCOMPARE(file->allImageLevels().at(6).area(), QRect(2, 1, 1, 1));

    delete file;
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_partialloader test;
    return QTest::qExec( &test, argc, argv );

}
