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

#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include <QuillImageFilterGenerator>

#include "unittests.h"
#include "ut_filtergenerator.h"
#include "quillfile.h"
#include "../../src/strings.h"
#include <Quill>

ut_filtergenerator::ut_filtergenerator()
{
}

void ut_filtergenerator::initTestCase()
{
    QDir().mkpath("/tmp/quill/thumbnails/normal");
}

void ut_filtergenerator::cleanupTestCase()
{
}

void ut_filtergenerator::init()
{
    Quill::initTestingMode();
}

void ut_filtergenerator::cleanup()
{
    Quill::cleanup();
}

/*!
  Test use through quill.
*/

void ut_filtergenerator::testAutoContrast()
{
    QTemporaryFile testFile;
    testFile.open();
    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    QuillFile file(testFile.fileName());
    file.setDisplayLevel(0);

    Quill::releaseAndWait();
    QCOMPARE((QImage)file.image(), Unittests::generatePaletteImage());

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    //QCOMPARE(filter->name(), QLatin1String("com.meego.composite.brightness.contrast"));

    filter->setOption(QuillImageFilter::Contrast, -50);

    file.runFilter(filter);

    // Also update big picture
    Quill::releaseAndWait();


    QuillImageFilter *filterGenerator =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_AutoContrast);
    //QCOMPARE(filterGenerator->name(), QLatin1String("com.meego.auto.contrast"));
    QVERIFY(dynamic_cast<QuillImageFilterGenerator*>(filterGenerator));

    file.runFilter(filterGenerator);


    // Generator
    Quill::releaseAndWait();
    // Generated
    Quill::releaseAndWait();

    QImage image = file.image();
    QImage refImage = Unittests::generatePaletteImage();

    QCOMPARE(image.size(), refImage.size());

    // Original image should now be restored - An offset of +-1 is
    // tolerated.

    for (int p=0; p<16; p++)
    {
        int rgb = image.pixel(p%8, p/8);
        int rgb2 = refImage.pixel(p%8, p/8);

        QVERIFY(abs(qRed(rgb)-qRed(rgb2)) <= 1);
        QVERIFY(abs(qGreen(rgb)-qGreen(rgb2)) <= 1);
        QVERIFY(abs(qBlue(rgb)-qBlue(rgb2)) <= 1);
    }
}

/*!
  Test use through quill.
*/

void ut_filtergenerator::testAutoLevels()
{
    QTemporaryFile testFile;
    testFile.open();
    Unittests::generatePaletteImage(20, 235).save(testFile.fileName(), "png");

    QuillFile file(testFile.fileName());
    file.setDisplayLevel(0);

    Quill::releaseAndWait();
    QCOMPARE((QImage)file.image(), Unittests::generatePaletteImage(20, 235));


    QuillImageFilter *filterGenerator =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_AutoLevels);
    //QCOMPARE(filterGenerator->name(), QLatin1String("com.meego.auto.levels"));
    QVERIFY(dynamic_cast<QuillImageFilterGenerator*>(filterGenerator));

    file.runFilter(filterGenerator);


    // Generator
    Quill::releaseAndWait();
    // Generated
    Quill::releaseAndWait();

    QImage image = file.image();
    QImage refImage = Unittests::generatePaletteImage();

    QCOMPARE(image.size(), refImage.size());

    // Original image should now be restored - An offset of +-20 is
    // tolerated. For autofix, the range [-20, 20] is normal.

    for (int p=0; p<16; p++)
    {
        int rgb = image.pixel(p%8, p/8);
        int rgb2 = refImage.pixel(p%8, p/8);
        QVERIFY(abs(qRed(rgb)-qRed(rgb2)) <= 20);
        QVERIFY(abs(qGreen(rgb)-qGreen(rgb2)) <= 20);
        QVERIFY(abs(qBlue(rgb)-qBlue(rgb2)) <= 20);
    }
}

/*!
  Make sure that RER bases its operation on the best available preview.
*/

void ut_filtergenerator::testRedEyeRemoval()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image(QImage(QSize(4, 4), QImage::Format_ARGB32));
    image.fill(qRgb(0, 0, 255));

    // Create red "eye" in the center

    image.setPixel(QPoint(1, 1), qRgb(255, 0, 0));
    image.setPixel(QPoint(2, 1), qRgb(255, 0, 0));
    image.setPixel(QPoint(1, 2), qRgb(255, 0, 0));
    image.setPixel(QPoint(2, 2), qRgb(255, 0, 0));

    image.save(testFile.fileName(), "png");

    Quill::setPreviewSize(0, QSize(2, 2));
    Quill::setThumbnailBasePath("/tmp/quill/thumbnails");
    Quill::setThumbnailFlavorName(0, "normal");
    Quill::setThumbnailExtension(Strings::png);

    // Create blank thumbnail
    QuillImage blankImage(QImage(QSize(2, 2), QImage::Format_ARGB32));
    blankImage.fill(qRgb(255, 255, 255));

    QuillFile file(testFile.fileName());
    QVERIFY(file.exists());

    blankImage.save(file.thumbnailFileName(0));

    file.setDisplayLevel(0);
    Quill::releaseAndWait();

    QVERIFY(Unittests::compareImage(file.image(), blankImage));

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_RedEyeDetection);

    filter->setOption(QuillImageFilter::Center, QVariant(QPoint(1, 1)));
    filter->setOption(QuillImageFilter::Radius, QVariant(2));

    Quill::releaseAndWait(); // preview

    file.runFilter(filter);
    file.setDisplayLevel(1);
    Quill::releaseAndWait(); // preview - generator
    Quill::releaseAndWait(); // preview - filter
    Quill::releaseAndWait(); // full - load
    Quill::releaseAndWait(); // full - filter

    // We should see no effect on the full now
    QVERIFY(Unittests::compareImage(file.image(), image));

    QuillImageFilter *filter2 =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_RedEyeDetection);
    filter2->setOption(QuillImageFilter::Center, QVariant(QPoint(1, 1)));
    filter2->setOption(QuillImageFilter::Radius, QVariant(2));

    file.runFilter(filter2);

    Quill::releaseAndWait(); // full - generator!
    Quill::releaseAndWait(); // preview - filter
    Quill::releaseAndWait(); // full - filter

    // We should see the effect on the full now
    QVERIFY(!Unittests::compareImage(file.image(), image));
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_filtergenerator test;
    return QTest::qExec( &test, argc, argv );

}
