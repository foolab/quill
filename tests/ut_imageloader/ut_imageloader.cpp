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
#include "ut_imageloader.h"
#include <Quill>
#include <QuillFile>
#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include "unittests.h"

ut_imageloader::ut_imageloader()
{
}

void ut_imageloader::initTestCase()
{
}

void ut_imageloader::cleanupTestCase()
{
}

void ut_imageloader::init()
{
    Quill::initTestingMode();
}

void ut_imageloader::cleanup()
{
    Quill::cleanup();
}

// Test standard Qt image loading
//
// If this test fails, it probably means that there is something wrong
// with the test image file or the reference table palette16.

void ut_imageloader::testStandardPNGLoader()
{
    QTemporaryFile testFile;
    testFile.open();
    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    QImage image(testFile.fileName());
    image = image.convertToFormat(QImage::Format_RGB32);

    QCOMPARE(image, Unittests::generatePaletteImage());
}

void ut_imageloader::testLoadFilter()
{
    QTemporaryFile testFile;
    testFile.open();
    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    QuillImageFilter *filter =

        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Role_Load);

    QVERIFY(filter);

    QVERIFY(filter->supportsOption(QuillImageFilter::FileName));
    QVERIFY(filter->setOption(QuillImageFilter::FileName,
                              QVariant(testFile.fileName())));

    QCOMPARE(filter->option(QuillImageFilter::FileName).toString(),
             QString(testFile.fileName()));

    QVERIFY(filter->supportsOption(QuillImageFilter::DisableUndo));
    QCOMPARE(filter->option(QuillImageFilter::DisableUndo).toBool(),
             true);

    QImage image = filter->apply(QImage());

    QCOMPARE(image, Unittests::generatePaletteImage());

    delete filter;
}

// Test libquill image loading on setup, size equal with target

void ut_imageloader::testSetupSameSize()
{
    QTemporaryFile testFile;
    testFile.open();
    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill::setPreviewSize(0, QSize(8, 2));

    QuillFile *file =
        new QuillFile(testFile.fileName());
    file->setDisplayLevel(1);

    QSignalSpy spy(file, SIGNAL(imageAvailable(const QuillImageList)));
    QCOMPARE(spy.count(), 0);
    QCOMPARE(file->image(), QuillImage());

    Quill::releaseAndWait();

    QCOMPARE(spy.count(), 1);

    QuillImageList imageList = spy.first().first().
        value<QuillImageList>();

    QCOMPARE(imageList.at(0), QuillImage(Unittests::generatePaletteImage()));
    QCOMPARE(file->image(), QuillImage(Unittests::generatePaletteImage()));

    Quill::releaseAndWait();

    delete file;
}


// Test libquill image loading on setup, smaller size than target

void ut_imageloader::testSetupSmallerSize()
{
    QTemporaryFile testFile;
    testFile.open();
    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill::setPreviewSize(0, QSize(4, 1));

    QuillFile *file =
        new QuillFile(testFile.fileName());
    file->setDisplayLevel(1);

    QSignalSpy spy(file, SIGNAL(imageAvailable(const QuillImageList)));
    QCOMPARE(spy.count(), 0);
    QCOMPARE(file->image(), QuillImage());

    Quill::releaseAndWait();

    QCOMPARE(spy.count(), 1);

    QuillImageList imageList = spy.first().first().
        value<QuillImageList>();

    QuillImage image = file->image();
    QCOMPARE(image, imageList.at(0));

    QCOMPARE(image.height(), 1);
    QCOMPARE(image.width(), 4);
    QCOMPARE(image, QuillImage(Unittests::generatePaletteImage().
                               scaled(QSize(4, 1), Qt::IgnoreAspectRatio,
                                      Qt::SmoothTransformation)));

    Quill::releaseAndWait();

    delete file;
}

// Test libquill image loading on setup, bigger size than target

void ut_imageloader::testSetupBiggerSize()
{
    QTemporaryFile testFile;
    testFile.open();
    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill::setPreviewSize(0, QSize(16, 4));

    QuillFile *file =
        new QuillFile(testFile.fileName());
    file->setDisplayLevel(1);

    QSignalSpy spy(file, SIGNAL(imageAvailable(const QuillImageList)));
    QCOMPARE(spy.count(), 0);
    QCOMPARE(file->image(), QuillImage());

    Quill::releaseAndWait();

    QCOMPARE(spy.count(), 1);

    QuillImageList imageList = spy.first().first().
        value<QuillImageList>();

    QuillImage image = file->image();
    QCOMPARE(image, imageList.at(0));

    QCOMPARE(image.height(), 2);
    QCOMPARE(image.width(), 8);
    QCOMPARE(image, QuillImage(Unittests::generatePaletteImage()));

    Quill::releaseAndWait();

    delete file;
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_imageloader test;
    return QTest::qExec( &test, argc, argv );

}

