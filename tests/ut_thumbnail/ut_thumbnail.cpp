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

#include <QImageWriter>
#include <QtTest/QtTest>
#include <QuillImageFilter>
#include <QuillImageFilterFactory>

#include <Quill>
#include <QuillFile>
#include "file.h"
#include "ut_thumbnail.h"
#include "unittests.h"
#include "../../src/unix_platform.h"
#include "../../src/strings.h"

ut_thumbnail::ut_thumbnail()
{
}

void ut_thumbnail::initTestCase()
{
    QDir().mkpath("/tmp/quill/thumbnails");
    QDir().mkpath("/tmp/quill/thumbnails/normal");
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
    QuillFile file("/tmp/test.png", "");

    // This should be safe to run in any environment as this test does
    // not really create, or check existence of, any files.

    Quill::setThumbnailBasePath("/home/user");
    Quill::setThumbnailFlavorName(0, "normal");
    Quill::setThumbnailExtension(Strings::jpeg);

    QCOMPARE(file.thumbnailFileName(0),
             QString("/home/user/normal/6756f54a791d53a4ece8ebb70471b573.jpeg"));
    QCOMPARE(file.failedThumbnailFileName(),
             QString("/home/user/fail/quill/6756f54a791d53a4ece8ebb70471b573.jpeg"));
}

void ut_thumbnail::testInvalid()
{
    QTemporaryFile testFile;
    testFile.open();
    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    QuillFile file(testFile.fileName(), Strings::png);

    Quill::setThumbnailBasePath("/tmp/quill/thumbnails");
    Quill::setThumbnailFlavorName(0, "normal");
    Quill::setThumbnailExtension(Strings::png);

    QImage image = Unittests::generatePaletteImage();
    image.save(file.thumbnailFileName(0));

    // A thumbnail with a creation date in the future is invalid
    FileSystem::setFileModificationDateTime(file.thumbnailFileName(0),
                                            QDateTime().addSecs(1));

    QVERIFY(!file.hasThumbnail(0));
}

void ut_thumbnail::testValid()
{
    QTemporaryFile testFile;
    testFile.open();
    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    QuillFile file(testFile.fileName(), Strings::png);

    Quill::setThumbnailBasePath("/tmp/quill/thumbnails");
    Quill::setThumbnailFlavorName(0, "normal");
    Quill::setThumbnailExtension(Strings::png);

    QImage image = Unittests::generatePaletteImage();
    image.save(file.thumbnailFileName(0));

    FileSystem::setFileModificationDateTime(file.thumbnailFileName(0),
                                            file.lastModified());

    QVERIFY(file.hasThumbnail(0));
}

void ut_thumbnail::testLoad()
{
    QTemporaryFile testFile;
    testFile.open();
    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill::setPreviewSize(0, QSize(4, 1));
    Quill::setThumbnailBasePath("/tmp/quill/thumbnails");
    Quill::setThumbnailFlavorName(0, "normal");
    Quill::setThumbnailExtension(Strings::png);

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

    Quill::setThumbnailBasePath("/tmp/quill/thumbnails");
    Quill::setThumbnailFlavorName(0, "normal");
    Quill::setThumbnailExtension(Strings::png);

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

    Quill::setPreviewSize(0, QSize(8, 2));

    Quill::setEditHistoryPath("/tmp/quill/history");
    Quill::setThumbnailBasePath("/tmp/quill/thumbnails");
    Quill::setThumbnailFlavorName(0, "normal");
    Quill::setThumbnailExtension(Strings::png);

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    QString thumbName = file->thumbnailFileName(0);
    image.save(thumbName);

    file->setDisplayLevel(0);
    Quill::releaseAndWait();

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(16));
    QuillImageFilter *filterb =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filterb);
    filterb->setOption(QuillImageFilter::Brightness, QVariant(16));

    file->runFilter(filter);
    Quill::releaseAndWait(); // preview up to date

    file->save();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait(); // save, should also clear thumbnails

    QVERIFY(Unittests::compareImage(filterb->apply(image),
                                    QImage(testFile.fileName())));
    QVERIFY(!QFile::exists(thumbName));

    Quill::releaseAndWait(); // thumbnail created

    QVERIFY(QFile::exists(thumbName));
    QVERIFY(Unittests::compareImage(filterb->apply(image), QImage(thumbName)));

    delete file;
    delete filterb;
}

void ut_thumbnail::testExternalUpdate()
{
    QTemporaryFile testFile;
    testFile.open();
    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    Quill::setPreviewSize(0, QSize(8, 2));

    Quill::setEditHistoryPath("/tmp/quill/history");
    Quill::setThumbnailBasePath("/tmp/quill/thumbnails");
    Quill::setThumbnailFlavorName(0, "normal");
    Quill::setThumbnailExtension(Strings::png);

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);

    file->setDisplayLevel(0);
    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // thumb save

    QVERIFY(file->hasThumbnail(0));

    delete file;

    // modify the file
    QImage blankImage(QSize(8, 2), QImage::Format_RGB32);
    blankImage.fill(qRgb(255, 255, 255));
    blankImage.save(testFile.fileName(), "png");

    // make sure that the file datetime will be different so that old
    // thumbnails get invalidated!
    FileSystem::setFileModificationDateTime(testFile.fileName(),
                                            QDateTime().addSecs(1));

    file = new QuillFile(testFile.fileName(), Strings::png);

    // thumbnail should be treated as invalid
    QVERIFY(!file->hasThumbnail(0));

    file->setDisplayLevel(0);
    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // thumb save

    // thumbnail should be recreated
    QVERIFY(file->hasThumbnail(0));

    QVERIFY(Unittests::compareImage(blankImage,
                                    QImage(file->thumbnailFileName(0))));
    delete file;
}

void ut_thumbnail::testLoadUnsupported()
{
    // Even if the original image is thrashed, we should still get the
    // thumbnail.

    QTemporaryFile testFile;
    testFile.open();
    testFile.write("/FF/D8");
    testFile.flush();
    QString fileName = testFile.fileName();

    QImage image = Unittests::generatePaletteImage();

    Quill::setPreviewSize(0, QSize(4, 1));
    Quill::setThumbnailBasePath("/tmp/quill/thumbnails");
    Quill::setThumbnailFlavorName(0, "normal");
    Quill::setThumbnailExtension(Strings::png);

    QString thumbName = "/tmp/quill/thumbnails/normal/" +
        File::filePathHash(fileName) + ".png";
    image.save(thumbName, "png");

    QuillFile *file = new QuillFile(fileName, "");
    QVERIFY(file->exists());

    file->setDisplayLevel(0);
    QVERIFY(file->exists());
    Quill::releaseAndWait();

    // We should now see the "thumbnail" even if the original file
    // does not exist.

    QVERIFY(file->exists());
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

    // Should ensure that a directory will not be created.
    QFile dummyFile("/tmp/invalid");
    dummyFile.open(QIODevice::WriteOnly);

    Quill::setThumbnailBasePath("/tmp/");
    Quill::setThumbnailFlavorName(0, "invalid");
    Quill::setThumbnailExtension(Strings::png);

    QuillFile *file = new QuillFile(testFile.fileName());

    file->setDisplayLevel(0);
    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // thumbnail save

    // Verify save failure
    QVERIFY(!Unittests::compareImage(QImage(file->thumbnailFileName(0)),
                                     image));

    QVERIFY(!Quill::isThumbnailCreationEnabled());

    // Change the thumbnail directory into something reasonable
    Quill::setThumbnailBasePath("/tmp/quill/thumbnails");
    Quill::setThumbnailFlavorName(0, "normal");

    Quill::setThumbnailCreationEnabled(true);
    Quill::releaseAndWait();
    // Thumbnail creation must not be disabled now
    QVERIFY(Quill::isThumbnailCreationEnabled());

    // Thumbnail must be saved now
    QVERIFY(Unittests::compareImage(QImage(file->thumbnailFileName(0)),
                                    image));

    delete file;
}

void ut_thumbnail::testFromSetImage()
{
    QTemporaryFile testFile;
    testFile.open();
    testFile.write("AVI");
    testFile.flush();

    QuillImage image = Unittests::generatePaletteImage();

    Quill::setPreviewLevelCount(2);
    Quill::setPreviewSize(0, QSize(2, 2));
    Quill::setMinimumPreviewSize(0, QSize(2, 2));

    Quill::setThumbnailBasePath("/tmp/quill/thumbnails");
    Quill::setThumbnailFlavorName(0, "normal");
    Quill::setThumbnailFlavorName(1, "large");
    Quill::setThumbnailExtension(Strings::png);

    // Not testing D-Bus thumbnailer here
    Quill::setDBusThumbnailingEnabled(false);

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::aviMimeType);
    QVERIFY(file);

    file->setDisplayLevel(1);
    file->setImage(1, image);

    QString thumbName = file->thumbnailFileName(0);
    QString thumbName1 = file->thumbnailFileName(1);

    Quill::releaseAndWait(); // try and fail load
    Quill::releaseAndWait(); // create lv0
    Quill::releaseAndWait(); // save lv0
    Quill::releaseAndWait(); // save lv1

    // We should not have a thumbnail.

    QVERIFY(!QFile::exists(thumbName));
    QVERIFY(!QFile::exists(thumbName1));

    delete file;
}

void ut_thumbnail::testCreationAfterQuillFileRemoval()
{
    QTemporaryFile testFile;
    testFile.open();
    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    Quill::setPreviewSize(0, QSize(4, 1));

    Quill::setThumbnailBasePath("/tmp/quill/thumbnails");
    Quill::setThumbnailFlavorName(0, "normal");
    Quill::setThumbnailExtension(Strings::png);

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::pngMimeType);
    QVERIFY(file);
    QString thumbName = file->thumbnailFileName(0);
    QVERIFY(!QFile::exists(thumbName));

    file->setDisplayLevel(0);
    Quill::releaseAndWait(); // load
    delete file; // delete immediately afterwards
    Quill::releaseAndWait(); // save should still happen

    QVERIFY(QFile::exists(thumbName));
}

void ut_thumbnail::testFullImageSize()
{
    QTemporaryFile testFile1;
    testFile1.open();
    Unittests::generatePaletteImage().save(testFile1.fileName(), "png");

    Quill::setPreviewSize(0, QSize(4, 1));
    Quill::setThumbnailBasePath("/tmp/quill/thumbnails");
    Quill::setThumbnailFlavorName(0, "normal");
    Quill::setThumbnailExtension(Strings::png);
    QuillFile *file = new QuillFile(testFile1.fileName());
    QVERIFY(file);
    QVERIFY(file->exists());
    QuillImage image = QuillImage(QImage(QSize(4, 1), QImage::Format_ARGB32));
    image.fill(qRgb(255, 255, 255));
    QString thumbName = file->thumbnailFileName(0);
    image.save(thumbName);

    file->setDisplayLevel(0);
    Quill::releaseAndWait();
    QVERIFY(!file->image(0).isNull());
    //the full image size is not calculated yet.
    QCOMPARE(file->image(0).fullImageSize(),QSize());
    // run a brightness filter
    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(16));
    file->runFilter(filter);
    Quill::releaseAndWait();
    QVERIFY(!file->image(0).isNull());
    //The full image size should be available
    QCOMPARE(file->image(0).fullImageSize(),QSize(8,2));
    delete file;
}

void ut_thumbnail::testFailedThumbnail()
{
    QTemporaryFile testFile;
    testFile.open();

    // Construct a corrupt PNG image
    QuillImage image = Unittests::generatePaletteImage();

    QByteArray buffer;
    QBuffer device(&buffer);
    QImageWriter writer(&device, QByteArray("png"));
    writer.write(image);

    buffer.chop(8); // last 8 bytes needs to be removed to recognize as corrupt
    testFile.write(buffer);
    testFile.close();

    Quill::setPreviewSize(0, QSize(4, 1));
    Quill::setThumbnailBasePath("/tmp/quill/thumbnails");
    Quill::setThumbnailFlavorName(0, "normal");
    Quill::setThumbnailExtension(Strings::png);

    QuillFile *file = new QuillFile(testFile.fileName(), "image/png");
    file->setDisplayLevel(0);

    QVERIFY(file);
    QVERIFY(file->exists());
    QVERIFY(file->supportsViewing());

    Quill::releaseAndWait();

    QVERIFY(!file->supportsViewing());

    // The failed thumbnail should be in place now
    QVERIFY(QFile::exists(file->failedThumbnailFileName()));

    delete file;

    QuillFile *file2 = new QuillFile(testFile.fileName(), "image/png");
    file2->setDisplayLevel(0);

    QVERIFY(file2);
    QVERIFY(file2->exists());

    // The file should now instantly classify as failed, without a
    // releaseAndWait() call.
    QVERIFY(!file2->supportsViewing());

    delete file2;
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_thumbnail test;
    return QTest::qExec( &test, argc, argv );
}
