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
#include <QuillImageFilter>
#include <QuillImageFilterFactory>

#include <Quill>
#include <QuillFile>
#include "file.h"
#include "ut_croppedthumbnail.h"
#include "unittests.h"
#include "../../src/strings.h"

ut_croppedthumbnail::ut_croppedthumbnail()
{
}

void ut_croppedthumbnail::initTestCase()
{
}

void ut_croppedthumbnail::cleanupTestCase()
{
}

void ut_croppedthumbnail::init()
{
    Quill::initTestingMode();
}

void ut_croppedthumbnail::cleanup()
{
    Quill::cleanup();
}

void ut_croppedthumbnail::testReplacementPolicy()
{
    QTemporaryFile testFile;
    testFile.open();
    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill::setPreviewLevelCount(3);
    Quill::setPreviewSize(0, QSize(2, 2));
    Quill::setMinimumPreviewSize(0, QSize(2, 2));
    Quill::setPreviewSize(1, QSize(4, 2));
    Quill::setPreviewSize(2, QSize(8, 4));

    QuillFile *file = new QuillFile(testFile.fileName());
    file->setDisplayLevel(3);

    Quill::releaseAndWait(); // 0 - should be masked out
    Quill::releaseAndWait(); // 1
    Quill::releaseAndWait(); // 2

    // Since we are now requiring an uncropped level, the image levels here
    // should now not contain the cropped level as a substitute.
    QCOMPARE(file->allImageLevels().count(), 2);
    QCOMPARE(file->allImageLevels().first().z(), 1);
    QCOMPARE(file->allImageLevels().at(1).z(), 2);

    delete file;
}

void ut_croppedthumbnail::testCroppedThumbnailSize()
{
    QTemporaryFile testFile;
    testFile.open();
    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill::setPreviewLevelCount(1);
    Quill::setPreviewSize(0, QSize(2, 2));
    Quill::setMinimumPreviewSize(0, QSize(2, 2));

    QuillFile *file = new QuillFile(testFile.fileName());
    file->setDisplayLevel(0);

    Quill::releaseAndWait();

    QCOMPARE(file->allImageLevels().count(), 1);
    QCOMPARE(file->allImageLevels().first().size(), QSize(2, 2));

    delete file;
}

void ut_croppedthumbnail::testCroppedThumbnailAfterEdit()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    Quill::setPreviewLevelCount(1);
    Quill::setPreviewSize(0, QSize(2, 2));
    Quill::setMinimumPreviewSize(0, QSize(2, 2));

    QuillFile *file = new QuillFile(testFile.fileName());
    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_Crop);
    filter->setOption(QuillImageFilter::CropRectangle, QRect(0, 0, 4, 2));
    file->runFilter(filter);

    file->setDisplayLevel(1);

    Quill::releaseAndWait(); // load 0
    Quill::releaseAndWait(); // crop 0 - bad version
    Quill::releaseAndWait(); // load 1
    Quill::releaseAndWait(); // crop 1
    Quill::releaseAndWait(); // reform 0

    file->setDisplayLevel(0);
    QCOMPARE(file->allImageLevels().count(), 1);
    QCOMPARE(file->allImageLevels().first().size(), QSize(2, 2));
    QVERIFY(Unittests::compareImage(file->allImageLevels().first(),
                                    image.copy(1, 0, 2, 2)));

    delete file;
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_croppedthumbnail test;
    return QTest::qExec( &test, argc, argv );

}
