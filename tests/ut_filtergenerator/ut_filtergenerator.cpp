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

#include <QtTest/QtTest>
#include <QImage>

#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include <QuillImageFilterGenerator>

#include "unittests.h"
#include "ut_filtergenerator.h"
#include "quillfile.h"
#include <Quill>

ut_filtergenerator::ut_filtergenerator()
{
}

void ut_filtergenerator::initTestCase()
{
    QuillImageFilter::registerAll();
    QDir().mkpath("/tmp/quill/thumbnails");
}

void ut_filtergenerator::cleanupTestCase()
{
}

/*!
  Test use through quill.
*/

void ut_filtergenerator::testAutoContrast()
{
    QTemporaryFile testFile;
    testFile.open();
    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    QuillImageFilter::registerAll();

    Quill *quill = new Quill(QSize(8, 2), Quill::ThreadingTest);

    QuillFile *file =
        quill->file(testFile.fileName());
    file->setDisplayLevel(0);

    quill->releaseAndWait();
    QCOMPARE((QImage)file->image(), Unittests::generatePaletteImage());

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("org.maemo.composite.brightness.contrast");
    QVERIFY(filter);

    filter->setOption(QuillImageFilter::Contrast, -50);

    file->runFilter(filter);

    // Also update big picture
    quill->releaseAndWait();

    QuillImageFilter *filterGenerator =
        QuillImageFilterFactory::createImageFilter("org.maemo.auto.contrast");

    file->runFilter(filterGenerator);
    // Generator
    quill->releaseAndWait();
    // Generated
    quill->releaseAndWait();

    QImage image = file->image();
    QImage refImage = Unittests::generatePaletteImage();

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

    delete quill;
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

    Quill *quill = new Quill(QSize(2, 2), Quill::ThreadingTest);
    QVERIFY(quill);

    quill->setThumbnailDirectory(0, "/tmp/quill/thumbnails");
    quill->setThumbnailExtension("png");

    // Create blank thumbnail
    QuillImage blankImage(QImage(QSize(2, 2), QImage::Format_ARGB32));
    blankImage.fill(qRgb(255, 255, 255));

    QuillFile *file = quill->file(testFile.fileName());
    QVERIFY(file->exists());

    blankImage.save(file->thumbnailFileName(0));

    file->setDisplayLevel(0);
    quill->releaseAndWait();

    QVERIFY(Unittests::compareImage(file->image(), blankImage));

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("org.maemo.red-eye-detection");

    filter->setOption(QuillImageFilter::Center, QVariant(QPoint(1, 1)));
    filter->setOption(QuillImageFilter::Radius, QVariant(2));

    quill->releaseAndWait(); // preview

    file->runFilter(filter);
    file->setDisplayLevel(1);
    quill->releaseAndWait(); // preview - generator
    quill->releaseAndWait(); // preview - filter
    quill->releaseAndWait(); // full - load
    quill->releaseAndWait(); // full - filter

    // We should see no effect on the full now
    QVERIFY(Unittests::compareImage(file->image(), image));

    QuillImageFilter *filter2 =
        QuillImageFilterFactory::createImageFilter("org.maemo.red-eye-detection");
    filter2->setOption(QuillImageFilter::Center, QVariant(QPoint(1, 1)));
    filter2->setOption(QuillImageFilter::Radius, QVariant(2));

    file->runFilter(filter2);

    quill->releaseAndWait(); // full - generator!
    quill->releaseAndWait(); // preview - filter
    quill->releaseAndWait(); // full - filter

    // We should see the effect on the full now
    QVERIFY(!Unittests::compareImage(file->image(), image));

    delete quill;
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_filtergenerator test;
    return QTest::qExec( &test, argc, argv );

}
