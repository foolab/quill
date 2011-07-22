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
#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include <QSignalSpy>

#include "quillfile.h"
#include "unittests.h"
#include "ut_tiling.h"
#include "quillundocommand.h"
#include "quillundostack.h"
#include "../../src/strings.h"

ut_tiling::ut_tiling()
{
}

void ut_tiling::initTestCase()
{
}

void ut_tiling::cleanupTestCase()
{
}

void ut_tiling::init()
{
    Quill::initTestingMode();
    Quill::setPreviewSize(0, QSize(4, 1));
}

void ut_tiling::cleanup()
{
    Quill::cleanup();
}

// Test a simple case of tiled saving
void ut_tiling::testTiledSaving()
{
    QTemporaryFile testFile;
    testFile.open();

    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill::setEditHistoryPath("/tmp/quill/history");
    Quill::setDefaultTileSize(QSize(2, 2));

    QImage image(testFile.fileName());
    QCOMPARE(image, Unittests::generatePaletteImage());

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    file->setDisplayLevel(1);

    QCOMPARE(file->viewPort(), QRect());
    Quill::releaseAndWait(); // preview

    file->setViewPort(QRect(0, 0, 8, 2));
    QCOMPARE(file->viewPort(), QRect(0, 0, 8, 2));

    // Load 4 tiles
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    QCOMPARE(file->allImageLevels().count(), 5);

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImage targetImage =
        filter->apply(Unittests::generatePaletteImage());

    file->runFilter(filter);
    Quill::releaseAndWait(); // preview

    // Apply for 4 tiles
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    QCOMPARE(file->allImageLevels().count(), 5);

    file->save();

    // Overlay 4 tiles
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    // Save
    Quill::releaseAndWait();

    QImage resultImage(testFile.fileName());
    QVERIFY(Unittests::compareImage(resultImage, targetImage));

    delete file;
}

// Test tiled saving with not enough cache
// No preview processing!
void ut_tiling::testTiledSavingSmallCache()
{
    QTemporaryFile testFile;
    testFile.open();

    QTemporaryFile originFile;
    originFile.open();

    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill::setEditHistoryPath("/tmp/quill/history");
    Quill::setDefaultTileSize(QSize(2, 2));
    Quill::setTileCacheSize(1);

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);

    file->setViewPort(QRect(0, 0, 8, 2));

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImage targetImage =
        filter->apply(Unittests::generatePaletteImage());

    file->runFilter(filter);
    file->save();

    // Tile 1
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    // Tile 2
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    // Tile 3
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    // Tile 4
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    // Saving
    Quill::releaseAndWait();

    QImage resultImage(testFile.fileName());
    QVERIFY(Unittests::compareImage(resultImage, targetImage));

    delete file;
}

// Test tiled viewport changes
void ut_tiling::testTiledSwipe()
{
    QTemporaryFile testFile;
    testFile.open();

    QTemporaryFile originFile;
    originFile.open();

    QTemporaryFile testFile2;
    testFile2.open();

    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    QImage image2(QSize(4, 4), QImage::Format_RGB32);
    image2.fill(qRgb(255, 255, 255));
    image2.save(testFile2.fileName(), "png");

    Quill::setEditHistoryPath("/tmp/quill/history");
    Quill::setDefaultTileSize(QSize(2, 2));

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    file->setViewPort(QRect(0, 0, 8, 8));
    file->setDisplayLevel(1);

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImage targetImage = filter->apply(Unittests::generatePaletteImage());

    file->runFilter(filter);
    file->save();
    file->setDisplayLevel(-1);

    QuillFile *file2 = new QuillFile(testFile2.fileName(), Strings::png);
    file2->setViewPort(QRect(0, 0, 8, 8));
    file2->setDisplayLevel(1);

    Quill::releaseAndWait(); // preview from first file (let run)
    Quill::releaseAndWait(); // preview for second

    QCOMPARE(file2->image().size(), QSize(1, 1));
    QCOMPARE(file2->image().fullImageSize(), QSize(4, 4));
    QCOMPARE(file2->image().pixel(0, 0), qRgb(255, 255, 255));

    // Save progress: Tile 1
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    // Tile 2
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    // Tile 3
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    // Tile 4
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    // Saving
    Quill::releaseAndWait();

    QImage resultImage(testFile.fileName());
    QVERIFY(Unittests::compareImage(resultImage, targetImage));

    // Load
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    QCOMPARE(file2->allImageLevels().count(), 5);

    delete file;
    delete file2;
}

// Test saving and loading
void ut_tiling::testTiledSaveLoad()
{
    QTemporaryFile testFile;
    testFile.open();

    QTemporaryFile originFile;
    originFile.open();

    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill::setEditHistoryPath("/tmp/quill/history");
    Quill::setDefaultTileSize(QSize(2, 2));

    QImage image(testFile.fileName());
    QCOMPARE(image, Unittests::generatePaletteImage());

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    QSignalSpy spy(file, SIGNAL(saved()));

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImage targetImage = filter->apply(Unittests::generatePaletteImage());

    file->setViewPort(QRect(0, 0, 8, 2));
    file->setDisplayLevel(-1);
    file->runFilter(filter);
    file->save();

    // Load 4 tiles
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    // Apply for 4 tiles
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    // Overlay 4 tiles
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    // Save
    Quill::releaseAndWait();

    QImage resultImage(testFile.fileName());
    QVERIFY(Unittests::compareImage(resultImage, targetImage));

    QCOMPARE(spy.count(), 1);

    file->setViewPort(QRect(0, 0, 8, 2));
    file->setDisplayLevel(1);

    // preview
    Quill::releaseAndWait();

    // Get the tiles
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    // Tiles still loaded from the original
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    QCOMPARE(file->allImageLevels().count(), 5);

    delete file;
}

// Test save buffers
void ut_tiling::testSaveBuffer()
{
    QTemporaryFile testFile;
    testFile.open();

    QTemporaryFile originFile;
    originFile.open();

    QImage originalImage =
        Unittests::generatePaletteImage().transformed(QTransform().rotate(90));

    originalImage.save(testFile.fileName(), "png");

    Quill::setEditHistoryPath("/tmp/quill/history");
    Quill::setDefaultTileSize(QSize(2, 2));
    Quill::setSaveBufferSize(4);

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    file->setViewPort(QRect(0, 0, 2, 8));
    file->setDisplayLevel(1);

    Quill::releaseAndWait(); // preview

    // Load 4 tiles
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    QCOMPARE(file->allImageLevels().count(), 5);

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImage targetImage = filter->apply(originalImage);

    file->runFilter(filter);

    Quill::releaseAndWait(); // preview

    // Apply for 4 tiles
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    QCOMPARE(file->allImageLevels().count(), 5);

    file->save();

    // Overlay + save (4 times)
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    Quill::releaseAndWait();
    Quill::releaseAndWait();

    Quill::releaseAndWait();
    Quill::releaseAndWait();

    Quill::releaseAndWait();
    Quill::releaseAndWait();

    QImage resultImage(testFile.fileName());

    QVERIFY(Unittests::compareImage(resultImage, targetImage));

    delete file;
}

void ut_tiling::testSaveBufferUnequal()
{
    QTemporaryFile testFile;
    testFile.open();

    QTemporaryFile originFile;
    originFile.open();

    QImage originalImage =
        Unittests::generatePaletteImage().transformed(QTransform().rotate(90));

    originalImage.save(testFile.fileName(), "png");

    Quill::setEditHistoryPath("/tmp/quill/history");

    Quill::setDefaultTileSize(QSize(2, 2));
    Quill::setSaveBufferSize(6);

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    file->setViewPort(QRect(0, 0, 2, 8));
    file->setDisplayLevel(1);

    Quill::releaseAndWait(); // preview

    // Load 4 tiles
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    QCOMPARE(file->allImageLevels().count(), 5);

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImage targetImage = filter->apply(originalImage);

    file->runFilter(filter);

    Quill::releaseAndWait(); // preview

    // Apply for 4 tiles
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    QCOMPARE(file->allImageLevels().count(), 5);

    file->save();

    // Overlay + save (2+1 for first 3 rows)
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    // Overlay + save (2+1 for second 3 rows)
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    // Overlay + save (1+1 for last 2 rows)
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    QImage resultImage(testFile.fileName());
    QVERIFY(Unittests::compareImage(resultImage, targetImage));

    delete file;
}

// Test panning
void ut_tiling::testPan()
{
    QTemporaryFile testFile;
    testFile.open();

    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill::setDefaultTileSize(QSize(2, 2));

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    QSignalSpy spy(file, SIGNAL(imageAvailable(QuillImageList)));
    file->setDisplayLevel(1);

    Quill::releaseAndWait();

    // No new tiles
    QCOMPARE(spy.count(), 1);
    file->setViewPort(QRect(0, 0, 8, 2));
    QCOMPARE(spy.count(), 1);

    // Load 4 tiles
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    // Remove viewport
    QCOMPARE(spy.count(), 5);
    file->setViewPort(QRect(0, 0, 0, 0));
    QCOMPARE(spy.count(), 5);

    // Get 1 tile
    file->setViewPort(QRect(0, 0, 2, 2));
    QCOMPARE(spy.count(), 6);
    QList<QuillImage> tileList = spy.at(5).first().value<QuillImageList>();
    QCOMPARE(tileList.count(), 1);
    QCOMPARE(tileList.at(0).area(), QRect(0, 0, 2, 2));

    // Get 1 new tile
    file->setViewPort(QRect(0, 0, 4, 2));
    QCOMPARE(spy.count(), 7);
    tileList = spy.at(6).first().value<QuillImageList>();
    QCOMPARE(tileList.count(), 1);
    QCOMPARE(tileList.at(0).area(), QRect(2, 0, 2, 2));

    // Forget 1, get 2
    file->setViewPort(QRect(2, 0, 6, 2));
    QCOMPARE(spy.count(), 8);
    tileList = spy.at(7).first().value<QuillImageList>();
    QCOMPARE(tileList.count(), 2);
    QCOMPARE(tileList.at(0).area(), QRect(4, 0, 2, 2));
    QCOMPARE(tileList.at(1).area(), QRect(6, 0, 2, 2));

    delete file;
}

void ut_tiling::testPreviewSizeChanges()
{
    QTemporaryFile testFile;
    testFile.open();

    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill::setPreviewSize(0, QSize(2, 1));
    Quill::setPreviewLevelCount(2);

    Quill::setDefaultTileSize(QSize(4, 4));

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    file->setDisplayLevel(1);

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_Rotate);
    filter->setOption(QuillImageFilter::Angle, QVariant(90));

    // preview level 0
    Quill::releaseAndWait();
    QCOMPARE(file->image(0).size(), QSize(2, 1));
    Quill::releaseAndWait(); // level 1

    file->runFilter(filter);
    Quill::releaseAndWait(); // level 0

    QCOMPARE(file->image(0).size(), QSize(1, 2));

    // preview level 1
    Quill::releaseAndWait();

    // preview level 0 regeneration
    Quill::releaseAndWait();

    QCOMPARE(file->image(0).size(), QSize(1, 1));

    delete file;
}


// Viewport contains more tiles than the cache
// This should reach a stable state.

void ut_tiling::testViewPortBiggerThanCache()
{
    QTemporaryFile testFile;
    testFile.open();

    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill::setPreviewSize(0, QSize(2, 1));
    Quill::setDefaultTileSize(QSize(2, 2));
    Quill::setTileCacheSize(3);

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    file->setDisplayLevel(1);

    Quill::releaseAndWait(); // preview

    file->setViewPort(QRect(-8, -2, 16, 4));

    Quill::releaseAndWait();
    QCOMPARE(file->allImageLevels().count(), 2);

    Quill::releaseAndWait();
    QCOMPARE(file->allImageLevels().count(), 3);

    Quill::releaseAndWait();
    QCOMPARE(file->allImageLevels().count(), 4);

    QCOMPARE(file->allImageLevels().at(1).area(), QRect(0, 0, 2, 2));
    QCOMPARE(file->allImageLevels().at(2).area(), QRect(2, 0, 2, 2));
    QCOMPARE(file->allImageLevels().at(3).area(), QRect(4, 0, 2, 2));

    // Now that we have 3 tiles aready,
    // this should not be doing anything anymore.

    Quill::releaseAndWait();

    QCOMPARE(file->allImageLevels().count(), 4);

    QCOMPARE(file->allImageLevels().at(1).area(), QRect(0, 0, 2, 2));
    QCOMPARE(file->allImageLevels().at(2).area(), QRect(2, 0, 2, 2));
    QCOMPARE(file->allImageLevels().at(3).area(), QRect(4, 0, 2, 2));

    delete file;
}

// Test saving which is interrupted when in progress
// Current expectation: cancel save
void ut_tiling::testSaveInterrupted()
{
    QTemporaryFile testFile, originFile;
    testFile.open();
    originFile.open();

    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill::setPreviewSize(0, QSize(4, 2));
    Quill::setEditHistoryPath("/tmp/quill/history");
    Quill::setDefaultTileSize(QSize(4, 4));

    QImage image(testFile.fileName());
    QCOMPARE(image, Unittests::generatePaletteImage());

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    file->setDisplayLevel(-1);

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QImage targetImage = filter->apply(image);

    file->runFilter(filter);
    file->save();

    // load both tiles
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    // filter for both tiles
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    // overlay 1/2
    Quill::releaseAndWait();

    file->undo();

    // overlay 2/2 - should do nothing
    Quill::releaseAndWait();
    // save - should do nothing
    Quill::releaseAndWait();

    QVERIFY(Unittests::compareImage(QImage(testFile.fileName()), image));

    file->redo();
    file->save();

    // overlay + save
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();    
    QVERIFY(Unittests::compareImage(QImage(testFile.fileName()), targetImage));

    delete file;
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_tiling test;
    return QTest::qExec( &test, argc, argv );

}
