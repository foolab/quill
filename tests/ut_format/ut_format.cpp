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
#include <QFile>
#include <QImage>
#include <QDebug>
#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include <QuillError>
#include <QSignalSpy>

#include "unittests.h"
#include "ut_format.h"
#include "quillfile.h"
#include "../../src/strings.h"

ut_format::ut_format()
{
}

Q_DECLARE_METATYPE(QuillImage);

void ut_format::initTestCase()
{
}

void ut_format::cleanupTestCase()
{
}

void ut_format::init()
{
    Quill::initTestingMode();
    // To make it easier to detect supported()
    Quill::setDBusThumbnailingEnabled(false);
}

void ut_format::cleanup()
{
    Quill::cleanup();
}

void ut_format::testSizeLimit()
{
    QTemporaryFile testFile;
    testFile.open();

    QSignalSpy spy(Quill::instance(), SIGNAL(error(QuillError)));

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    Quill::setImageSizeLimit(QSize(4, 4));
    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);

    file->setDisplayLevel(0);

    QCOMPARE(spy.count(), 1);
    QuillError error = spy.first().first().value<QuillError>();
    QCOMPARE(error.errorCode(), QuillError::ImageSizeLimitError);
    QCOMPARE(error.errorSource(), QuillError::ImageFileErrorSource);

    QVERIFY(!file->supportsViewing());

    delete file;
}

void ut_format::testPixelsLimit()
{
    QTemporaryFile testFile;
    testFile.open();

    QSignalSpy spy(Quill::instance(), SIGNAL(error(QuillError)));

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    Quill::setImagePixelsLimit(15);
    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);

    file->setDisplayLevel(0);

    QCOMPARE(spy.count(), 1);
    QuillError error = spy.first().first().value<QuillError>();
    QCOMPARE(error.errorCode(), QuillError::ImageSizeLimitError);
    QCOMPARE(error.errorSource(), QuillError::ImageFileErrorSource);

    QVERIFY(!file->supportsViewing());
    delete file;
}

void ut_format::testNonTiledPixelsLimit()
{
    QTemporaryFile testFile;
    testFile.open();

    QTemporaryFile testFile2;
    testFile2.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");
    image.save(testFile2.fileName(), "jpg");

    Quill::setImagePixelsLimit(24);
    Quill::setNonTiledImagePixelsLimit(12);
    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);

    file->setDisplayLevel(0);
    QVERIFY(!file->supportsViewing());

    QuillFile *file2 = new QuillFile(testFile2.fileName(), Strings::jpg);

    file2->setDisplayLevel(0);
    QVERIFY(file2->supportsViewing());

    delete file;
    delete file2;
}

void ut_format::testMultipleLimits()
{
    QTemporaryFile testFile;
    testFile.open();

    Quill::setImageSizeLimit(QSize(4, 4));
    Quill::setImagePixelsLimit(24);

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);

    file->setDisplayLevel(0);
    QVERIFY(!file->supportsViewing());

    delete file;
}

void ut_format::testReadOnlyFormat()
{
    QTemporaryFile testFile;
    testFile.open();

    QFile originalFile("/usr/share/libquill-tests/images/image_16x4.gif");
    originalFile.open(QIODevice::ReadOnly);
    QByteArray buffer = originalFile.readAll();
    testFile.write(buffer);
    testFile.flush();

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::gifMimeType);
    file->supportsEditing();
    QVERIFY(!file->supportsEditing());
    delete file;
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_format test;
    return QTest::qExec( &test, argc, argv );

}
