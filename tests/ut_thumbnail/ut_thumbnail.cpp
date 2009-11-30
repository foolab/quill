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

#include <QDebug>
#include <QtTest/QtTest>
#include <QuillImageFilter>
#include <QuillImageFilterFactory>

#include <Quill>
#include <QuillFile>
#include "file.h"
#include "ut_thumbnail.h"
#include "unittests.h"

ut_thumbnail::ut_thumbnail()
{
}

void ut_thumbnail::initTestCase()
{
    QDir().mkpath("/tmp/quill/thumbnails");
    Unittests::generatePaletteImage().save("/tmp/test.png");
}

void ut_thumbnail::cleanupTestCase()
{
}

void ut_thumbnail::init()
{
    Quill::initTestingMode();
}

void ut_thumbnail::cleanup()
{
    Quill::cleanup();
}

void ut_thumbnail::testName()
{
    QuillFile *file = new QuillFile("/tmp/test.png", "");
    QVERIFY(file);

    // This should be safe to run in any environment as this test does
    // not really create, or check existence of, any files.

    Quill::setThumbnailDirectory(0, "/home/user");
    Quill::setThumbnailExtension("jpeg");

    QCOMPARE(file->thumbnailFileName(0),
             QString("/home/user/6756f54a791d53a4ece8ebb70471b573.jpeg"));

    delete file;
}

void ut_thumbnail::testLoad()
{
    QTemporaryFile testFile;
    testFile.open();
    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill::setPreviewSize(0, QSize(4, 1));
    Quill::setThumbnailDirectory(0, "/tmp/quill/thumbnails");
    Quill::setThumbnailExtension("png");

    QuillFile *file = new QuillFile(testFile.fileName());
    QVERIFY(file);
    QVERIFY(file->exists());

    QuillImage image = QuillImage(QImage(QSize(4, 1), QImage::Format_ARGB32));
    image.fill(qRgb(255, 255, 255));

    QString thumbName = file->thumbnailFileName(0);
    image.save(thumbName);

    file->setDisplayLevel(0);
    Quill::releaseAndWait();

    // We should now see the "thumbnail" that we just created,
    // opposed to a downscaled version of the full image.

    QVERIFY(Unittests::compareImage(file->image(), image));

    delete file;
}

void ut_thumbnail::testSave()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    Quill::setPreviewSize(0, QSize(4, 1));

    Quill::setThumbnailDirectory(0, "/tmp/quill/thumbnails");
    Quill::setThumbnailExtension("png");

    QuillFile *file = new QuillFile(testFile.fileName());
    QVERIFY(file);
    QVERIFY(file->exists());

    QString thumbName = file->thumbnailFileName(0);

    file->setDisplayLevel(0);
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    // We should now have a newly created thumbnail.

    QVERIFY(Unittests::compareImage(QImage(thumbName),
                                    image.scaled(QSize(4, 1),
                                                 Qt::IgnoreAspectRatio,
                                                 Qt::SmoothTransformation)));

    delete file;
}

void ut_thumbnail::testUpdate()
{
    QTemporaryFile testFile;
    testFile.open();
    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    Quill::setEditHistoryDirectory("/tmp/quill/history");
    Quill::setThumbnailDirectory(0, "/tmp/quill/thumbnails");
    Quill::setThumbnailExtension("png");

    QuillFile *file = new QuillFile(testFile.fileName());
    QString thumbName = file->thumbnailFileName(0);
    image.save(thumbName);

    file->setDisplayLevel(0);
    Quill::releaseAndWait();

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("org.maemo.composite.brightness.contrast");
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(16));

    file->runFilter(filter);
    Quill::releaseAndWait(); // preview up to date

    file->save();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait(); // save, should also clear thumbnails

    QVERIFY(!QFile::exists(thumbName));

    Quill::releaseAndWait(); // thumbnail created

    QVERIFY(QFile::exists(thumbName));
    QVERIFY(Unittests::compareImage(filter->apply(image), QImage(thumbName)));

    delete file;
}

void ut_thumbnail::testLoadUnsupported()
{
    // Even if the original image is thrashed, we should still get the
    // thumbnail.

    QTemporaryFile testFile;
    testFile.open();

    QImage image = Unittests::generatePaletteImage();

    Quill::setPreviewSize(0, QSize(4, 1));
    Quill::setThumbnailDirectory(0, "/tmp/quill/thumbnails");
    Quill::setThumbnailExtension("png");

    QString thumbName = "/tmp/quill/thumbnails/" +
        File::fileNameHash(testFile.fileName()) + ".png";
    image.save(thumbName, "png");

    QuillFile *file = new QuillFile(testFile.fileName());
    QVERIFY(file->exists());

    file->setDisplayLevel(0);
    Quill::releaseAndWait();

    // We should now see the "thumbnail" even if the original file
    // does not exist.

    QVERIFY(Unittests::compareImage(file->image(), image));

    delete file;
}

void ut_thumbnail::testFailedWrite()
{
    // Test thumbnail writing with an invalid directory

    QTemporaryFile testFile;
    testFile.open();
    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QVERIFY(Quill::isThumbnailCreationEnabled());

    Quill::setThumbnailDirectory(0, "/invalid");
    Quill::setThumbnailExtension("png");

    QuillFile *file = new QuillFile(testFile.fileName());

    file->setDisplayLevel(0);
    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // thumbnail save

    // Verify save failure
    QVERIFY(!Unittests::compareImage(QImage(file->thumbnailFileName(0)),
                                     image));

    QVERIFY(!Quill::isThumbnailCreationEnabled());

    // Change the thumbnail directory into something reasonable
    Quill::setThumbnailDirectory(0, "/tmp/quill/thumbnails");

    Quill::setThumbnailCreationEnabled(true);
    Quill::releaseAndWait();
    // Thumbnail creation must not be disabled now
    QVERIFY(Quill::isThumbnailCreationEnabled());

    // Thumbnail must be saved now
    QVERIFY(Unittests::compareImage(QImage(file->thumbnailFileName(0)),
                                    image));

    delete file;
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_thumbnail test;
    return QTest::qExec( &test, argc, argv );
}
