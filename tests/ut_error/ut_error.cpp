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
#include <QSignalSpy>
#include <QMetaType>
#include <QTemporaryFile>
#include <QImageWriter>
#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include <Quill>

#include "quillerror.h"
#include "quill.h"
#include "quillfile.h"
#include "file.h"
#include "core.h"
#include "ut_error.h"
#include "unittests.h"
#include "../../src/strings.h"

ut_error::ut_error() : editHistoryPath("/tmp/quill/history"),
                       thumbnailBasePath("/tmp/quill/thumbnails"),
                       thumbnailFlavorName("normal"),
                       thumbnailFullPath(thumbnailBasePath+"/"+
                                         thumbnailFlavorName+"/")
{
}

void ut_error::initTestCase()
{
    QDir().mkpath(editHistoryPath);
    QDir().mkpath(thumbnailBasePath+"/"+thumbnailFlavorName);
}

void ut_error::cleanupTestCase()
{
}

void ut_error::init()
{
    Quill::initTestingMode();
}

void ut_error::cleanup()
{
    Quill::cleanup();
}

void ut_error::testOverwritingCopyFailed()
{
    File *file = new File();
    QTemporaryFile tempFile;
    tempFile.open();

    QuillError error = file->overwritingCopy(QString(), tempFile.fileName());

    QCOMPARE(error.errorCode(), QuillError::FileOpenForReadError);

    delete file;
}

void ut_error::testEditHistoryReadFailed()
{
    /*
    QuillError error;

    File *file = File::readFromEditHistory(QString(), QString(), &error);

    QVERIFY(!file);
    QCOMPARE(error.errorCode(), QuillError::FileNotFoundError);
    QCOMPARE(error.errorSource(), QuillError::ImageOriginalErrorSource);
    delete file;
    */
}

void ut_error::testEditHistoryWriteFailed()
{
    // Should ensure that a directory will not be created.
    QFile dummyFile("/tmp/invalid");
    dummyFile.open(QIODevice::WriteOnly);

    Quill::setEditHistoryPath("/tmp/invalid");

    QuillError error;

    File *file = new File;

    file->writeEditHistory(QString(), &error);

    QCOMPARE(error.errorCode(), QuillError::DirCreateError);
    QCOMPARE(error.errorSource(), QuillError::EditHistoryErrorSource);
    delete file;
}

void ut_error::testFileNotFound()
{
    QuillFile *file = new QuillFile(QString());
    file->setDisplayLevel(0);
    Quill::releaseAndWait();

    QuillError error = file->error();

    QCOMPARE(error.errorCode(), QuillError::FileNotFoundError);
    QCOMPARE(error.errorSource(), QuillError::ImageFileErrorSource);
    QCOMPARE(error.errorData(), QString());

    QVERIFY(!file->exists());
    delete file;
}

void ut_error::testForbiddenRead()
{
    if (Unittests::isRoot()) {
        qDebug() << "Running as root, disabling file permissions test!";
        return;
    }

    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");
    testFile.setPermissions(0);

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);

    file->setDisplayLevel(0);
    Quill::releaseAndWait();

    QVERIFY(file->image().isNull());
    QuillError error = file->error();

    QEXPECT_FAIL("", "QImageReader does not differentiate between nonexistent and unreadable files", Continue);
    QCOMPARE(error.errorCode(), QuillError::FileOpenForReadError);
    QCOMPARE(error.errorSource(), QuillError::ImageFileErrorSource);
    QCOMPARE(error.errorData(), testFile.fileName());
    delete file;
}

void ut_error::testEmptyFileRead()
{
    QTemporaryFile testFile;
    testFile.open();

    // Disable this or we will not get the error message as Quill will
    // wait for the D-Bus thumbnailer to finish first.
    Quill::setDBusThumbnailingEnabled(false);

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    file->setDisplayLevel(0);
    Quill::releaseAndWait();

    QVERIFY(file->image().isNull());
    QCOMPARE(spy.count(), 1);
    QuillError error = spy.first().first().value<QuillError>();

    QCOMPARE(error.errorCode(), QuillError::FileCorruptError);
    QCOMPARE(error.errorSource(), QuillError::ImageFileErrorSource);
    QCOMPARE(error.errorData(), testFile.fileName());

    error = file->error();
    QCOMPARE(error.errorCode(), QuillError::FileCorruptError);
    QCOMPARE(error.errorSource(), QuillError::ImageFileErrorSource);
    QCOMPARE(error.errorData(), testFile.fileName());

    QCOMPARE(file->supportsThumbnails(), false);
    QCOMPARE(file->supportsViewing(), false);
    QCOMPARE(file->supportsEditing(), false);
    delete file;
}

void ut_error::testCorruptedImageWithoutMime()
{
    QTemporaryFile testFile;
    testFile.open();
    QString fileName = testFile.fileName();

    // Construct a corrupt image
    QuillImage image = Unittests::generatePaletteImage();

    QByteArray buffer;
    QBuffer device(&buffer);
    QImageWriter writer(&device, QByteArray("jpg"));
    writer.write(image);

    buffer.remove(0, 2); // remove first two bytes (JPEG magic number)
    testFile.write(buffer);
    testFile.close();

    // MIME is empty
    QuillFile *file = new QuillFile(fileName, "");
    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    file->setDisplayLevel(0);
    Quill::releaseAndWait();

    QVERIFY(file->image().isNull());
    QCOMPARE(spy.count(), 1);
    QuillError error = spy.first().first().value<QuillError>();

    QCOMPARE(error.errorCode(), QuillError::FileFormatUnsupportedError);
    QCOMPARE(error.errorSource(), QuillError::ImageFileErrorSource);
    QCOMPARE(error.errorData(), fileName);

    error = file->error();
    QCOMPARE(error.errorCode(), QuillError::FileFormatUnsupportedError);
    QCOMPARE(error.errorSource(), QuillError::ImageFileErrorSource);
    QCOMPARE(error.errorData(), fileName);

    delete file;
}

void ut_error::testCorruptRead()
{
    qDebug() << "Test disabled!";
    return;

    QTemporaryFile testFile;
    testFile.open();
    QString fileName = testFile.fileName();

    // Construct a corrupt PNG image
    QuillImage image = Unittests::generatePaletteImage();

    QByteArray buffer;
    QBuffer device(&buffer);
    QImageWriter writer(&device, QByteArray("png"));
    writer.write(image);

//    buffer.chop(2); // remove 2 bytes from end
    buffer.remove(0, 2); // remove first two bytes
    testFile.write(buffer);
    testFile.close();

    QuillFile *file = new QuillFile(fileName, Strings::png);
    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    file->setDisplayLevel(0);
    Quill::releaseAndWait();

    QVERIFY(file->image().isNull());
    QCOMPARE(spy.count(), 1);
    QuillError error = spy.first().first().value<QuillError>();

    QCOMPARE(error.errorCode(), QuillError::FileCorruptError);
    QCOMPARE(error.errorSource(), QuillError::ImageFileErrorSource);
    QCOMPARE(error.errorData(), fileName);

    error = file->error();
    QCOMPARE(error.errorCode(), QuillError::FileCorruptError);
    QCOMPARE(error.errorSource(), QuillError::ImageFileErrorSource);
    QCOMPARE(error.errorData(), fileName);

    delete file;
}

void ut_error::testWriteProtectedFile()
{
    if (Unittests::isRoot()) {
        qDebug() << "Running as root, disabling file permissions test!";
        return;
    }

    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    file->runFilter(filter);

    QVERIFY(file->supportsEditing());

    // Write protect before save is called
    testFile.setPermissions(QFile::ReadOwner);

    file->save();

    Quill::releaseAndWait(); // load
    QCOMPARE(spy.count(), 0);

    Quill::releaseAndWait(); // filter
    QCOMPARE(spy.count(), 0);
    Quill::releaseAndWait(); // save

    QCOMPARE(spy.count(), 1);
    QuillError error = spy.first().first().value<QuillError>();

    QCOMPARE(error.errorCode(), QuillError::FileOpenForWriteError);
    QCOMPARE(error.errorSource(), QuillError::ImageFileErrorSource);
    QCOMPARE(error.errorData(), testFile.fileName());

    error = file->error();
    QCOMPARE(error.errorCode(), QuillError::FileOpenForWriteError);
    QCOMPARE(error.errorSource(), QuillError::ImageFileErrorSource);
    QCOMPARE(error.errorData(), testFile.fileName());

    QCOMPARE(QImage(testFile.fileName()), QImage(image));

    delete file;
}

void ut_error::testForbiddenOriginal()
{
    if (Unittests::isRoot()) {
        qDebug() << "Running as root, disabling file permissions test!";
        return;
    }

    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QImage targetImage = filter->apply(image);

    file->runFilter(filter);
    file->save();

    Quill::releaseAndWait(); // load
    QCOMPARE(spy.count(), 0);
    Quill::releaseAndWait(); // filter
    QCOMPARE(spy.count(), 0);
    Quill::releaseAndWait(); // save
    QCOMPARE(spy.count(), 0);

    QFileInfo fileInfo(testFile.fileName());
    QFile originalFile(fileInfo.path() + Strings::slashOriginal +
                       fileInfo.fileName());
    QVERIFY(originalFile.exists());
    originalFile.setPermissions(0);

    delete file;
    QSignalSpy spy2(Quill::instance(), SIGNAL(error(QuillError)));
    file = new QuillFile(testFile.fileName(), Strings::png);
    file->setDisplayLevel(0);
    Quill::releaseAndWait();
    QCOMPARE(file->image(), QuillImage(targetImage));

    // force the loading of original here
    file->undo();
    Quill::releaseAndWait();

    // Size calculation is actually made 3 times
    QCOMPARE(spy2.count(), 3);
    QuillError error = spy2.first().first().value<QuillError>();

    QEXPECT_FAIL("", "QImageReader does not differentiate between nonexistent and unreadable files", Continue);
    QCOMPARE(error.errorCode(), QuillError::FileOpenForReadError);
    QCOMPARE(error.errorSource(), QuillError::ImageOriginalErrorSource);
    QCOMPARE(error.errorData(), originalFile.fileName());

    error = file->error();
    QEXPECT_FAIL("", "QImageReader does not differentiate between nonexistent and unreadable files", Continue);
    QCOMPARE(error.errorCode(), QuillError::FileOpenForReadError);
    QCOMPARE(error.errorSource(), QuillError::ImageOriginalErrorSource);
    QCOMPARE(error.errorData(), originalFile.fileName());

    // Expect that the file can still be used,
    // just that no images before the current state can be viewed

    QVERIFY(!file->canUndo());
    QVERIFY(file->image().isNull());

    QVERIFY(file->canRedo());
    file->redo();
    QCOMPARE(spy2.count(), 3); // No further errors
    Quill::releaseAndWait();
    QCOMPARE(file->image(), QuillImage(targetImage));

    delete file;
}

void ut_error::testEmptyOriginal()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QImage targetImage = filter->apply(image);

    file->runFilter(filter);
    file->save();

    Quill::releaseAndWait(); // load
    QCOMPARE(spy.count(), 0);
    Quill::releaseAndWait(); // filter
    QCOMPARE(spy.count(), 0);
    Quill::releaseAndWait(); // save
    QCOMPARE(spy.count(), 0);

    QFileInfo fileInfo(testFile.fileName());
    QFile originalFile(fileInfo.path() + Strings::slashOriginal +
                       fileInfo.fileName());
    QVERIFY(originalFile.exists());
    originalFile.open(QIODevice::WriteOnly);
    originalFile.write("\xFD\x87");
    originalFile.close();

    delete file;
    QSignalSpy spy2(Quill::instance(), SIGNAL(error(QuillError)));
    file = new QuillFile(testFile.fileName(), Strings::png);
    file->setDisplayLevel(0);
    Quill::releaseAndWait();
    QCOMPARE(file->image(), QuillImage(targetImage));

    // force the loading of original here
    file->undo();
    Quill::releaseAndWait();

    // Size calculation is actually made 3 times
    QCOMPARE(spy2.count(), 3);
    QuillError error = spy2.first().first().value<QuillError>();

    QCOMPARE(error.errorCode(), QuillError::FileCorruptError);
    QCOMPARE(error.errorSource(), QuillError::ImageOriginalErrorSource);
    QCOMPARE(error.errorData(), originalFile.fileName());

    error = file->error();
    QCOMPARE(error.errorCode(), QuillError::FileCorruptError);
    QCOMPARE(error.errorSource(), QuillError::ImageOriginalErrorSource);
    QCOMPARE(error.errorData(), originalFile.fileName());

    // Expect that the file can still be used,
    // just that no images before the current state can be viewed

    QVERIFY(!file->canUndo());
    QVERIFY(file->image().isNull());

    QVERIFY(file->canRedo());
    file->redo();
    QCOMPARE(spy2.count(), 3); // No further errors
    Quill::releaseAndWait();
    QCOMPARE(file->image(), QuillImage(targetImage));

    delete file;
}

void ut_error::testCorruptOriginal()
{
    qDebug() << "Test disabled!";
    return;

    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QImage targetImage = filter->apply(image);

    file->runFilter(filter);
    file->save();

    Quill::releaseAndWait(); // load
    QCOMPARE(spy.count(), 0);
    Quill::releaseAndWait(); // filter
    QCOMPARE(spy.count(), 0);
    Quill::releaseAndWait(); // save
    QCOMPARE(spy.count(), 0);

    QFileInfo fileInfo(testFile.fileName());
    QFile originalFile(fileInfo.path() + Strings::slashOriginal +
                       fileInfo.fileName());
    QVERIFY(originalFile.exists());
    QVERIFY(originalFile.resize(originalFile.size()-2));

    delete file;
    QSignalSpy spy2(Quill::instance(), SIGNAL(error(QuillError)));
    file = new QuillFile(testFile.fileName(), Strings::png);
    QVERIFY(file->supportsEditing()); //create the error signal
    file->setDisplayLevel(0);
    Quill::releaseAndWait();
    QCOMPARE(file->image(), QuillImage(targetImage));

    // force the loading of original here
    file->undo();
    Quill::releaseAndWait();

    QCOMPARE(spy2.count(), 1);
    QuillError error = spy2.first().first().value<QuillError>();

    QCOMPARE(error.errorCode(), QuillError::FileCorruptError);
    QCOMPARE(error.errorSource(), QuillError::ImageOriginalErrorSource);
    QCOMPARE(error.errorData(), originalFile.fileName());

    error = file->error();
    QCOMPARE(error.errorCode(), QuillError::FileCorruptError);
    QCOMPARE(error.errorSource(), QuillError::ImageOriginalErrorSource);
    QCOMPARE(error.errorData(), originalFile.fileName());

    // Expect that the file can still be used,
    // just that no images before the current state can be viewed

    QVERIFY(!file->canUndo());
    QVERIFY(file->image().isNull());

    QVERIFY(file->canRedo());
    file->redo();
    Quill::releaseAndWait();
    QCOMPARE(spy2.count(), 1); // No further errors
    QCOMPARE(file->image(), QuillImage(targetImage));

    delete file;
}

void ut_error::testOriginalDirectoryCreateFailed()
{
    QDir().mkpath("/tmp/quill/no-original/");
    QFile dummyFile("/tmp/quill/no-original/.original");
    dummyFile.open(QIODevice::WriteOnly);

    QTemporaryFile testFile("/tmp/quill/no-original/XXXXXX.png");
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    Quill::setEditHistoryPath(editHistoryPath);

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QImage targetImage = filter->apply(image);

    file->runFilter(filter);
    file->save();
    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // filter
    QCOMPARE(spy.count(), 0);
    Quill::releaseAndWait(); // save
    QCOMPARE(spy.count(), 1);

    QuillError error = spy.first().first().value<QuillError>();

    QCOMPARE(error.errorCode(), QuillError::DirCreateError);
    QCOMPARE(error.errorSource(), QuillError::ImageOriginalErrorSource);
    QCOMPARE(error.errorData(), QString("/tmp/quill/no-original/.original"));

    error = file->error();
    QCOMPARE(error.errorCode(), QuillError::DirCreateError);
    QCOMPARE(error.errorSource(), QuillError::ImageOriginalErrorSource);
    QCOMPARE(error.errorData(), QString("/tmp/quill/no-original/.original"));

    QCOMPARE(QImage(testFile.fileName()), QImage(image));
    delete file;
    QFile::remove("/tmp/quill/no-original/test.png");
}

void ut_error::testForbiddenThumbnail()
{
    if (Unittests::isRoot()) {
        qDebug() << "Running as root, disabling file permissions test!";
        return;
    }

    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QString thumbFileName = File::filePathHash(testFile.fileName());
    thumbFileName.append(".");
    thumbFileName.append(Strings::png);
    thumbFileName.prepend(thumbnailFullPath);

    QImage thumbImage =
        image.scaled(QSize(4, 1), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    thumbImage.save(thumbFileName);

    QFile qFile(thumbFileName);
    qFile.setPermissions(0);

    Quill::setPreviewSize(0, QSize(4, 1));
    Quill::setThumbnailBasePath(thumbnailBasePath);
    Quill::setThumbnailFlavorName(0, thumbnailFlavorName);
    Quill::setThumbnailExtension(Strings::png);

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    file->setDisplayLevel(0);
    Quill::releaseAndWait();

    QCOMPARE(spy.count(), 1);
    QuillError error = spy.first().first().value<QuillError>();

    QEXPECT_FAIL("", "QImageReader does not differentiate between nonexistent and unreadable files", Continue);
    QCOMPARE(error.errorCode(), QuillError::FileOpenForReadError);
    QCOMPARE(error.errorSource(), QuillError::ThumbnailErrorSource);
    QCOMPARE(error.errorData(), thumbFileName);

    error = file->error();
    QEXPECT_FAIL("", "QImageReader does not differentiate between nonexistent and unreadable files", Continue);
    QCOMPARE(error.errorCode(), QuillError::FileOpenForReadError);
    QCOMPARE(error.errorSource(), QuillError::ThumbnailErrorSource);
    QCOMPARE(error.errorData(), thumbFileName);

    // Make sure that the offending thumbnail got deleted
    QVERIFY(!QFile::exists(thumbFileName));

    // Recreate the bad thumbnail (this is to simulate the situation
    // where the thumbnail cannot be deleted)

    thumbImage.save(thumbFileName);

    QFile qFile2(thumbFileName);
    qFile2.setPermissions(0);

    // Instead of reading the bad thumbnail, we should now downscale
    // from the original image

    Quill::releaseAndWait();
    QVERIFY(Unittests::compareImage(file->image(), thumbImage));
    delete file;
}

void ut_error::testCorruptThumbnail()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QImage thumbImage =
        image.scaled(QSize(4, 1), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QString thumbFileName = File::filePathHash(testFile.fileName());
    thumbFileName.append(".");
    thumbFileName.append(Strings::png);
    thumbFileName.prepend(thumbnailFullPath);

    QFile qFile(thumbFileName);
    qFile.open(QIODevice::WriteOnly);
    qFile.write("\xFD\x87");
    qFile.close();

    Quill::setPreviewSize(0, QSize(4, 1));
    Quill::setThumbnailBasePath(thumbnailBasePath);
    Quill::setThumbnailFlavorName(0, thumbnailFlavorName);
    Quill::setThumbnailExtension(Strings::png);

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    file->setDisplayLevel(0);
    Quill::releaseAndWait();

    QCOMPARE(spy.count(), 1);
    QuillError error = spy.first().first().value<QuillError>();

    QCOMPARE(error.errorCode(), QuillError::FileCorruptError);
    QCOMPARE(error.errorSource(), QuillError::ThumbnailErrorSource);
    QCOMPARE(error.errorData(), thumbFileName);

    error = file->error();
    QCOMPARE(error.errorCode(), QuillError::FileCorruptError);
    QCOMPARE(error.errorSource(), QuillError::ThumbnailErrorSource);
    QCOMPARE(error.errorData(), thumbFileName);

    // Make sure that the offending thumbnail got deleted
    QVERIFY(!QFile::exists(thumbFileName));

    // Recreate the bad thumbnail (this is to simulate the situation
    // where the thumbnail cannot be deleted)

    thumbImage.save(thumbFileName);

    qFile.open(QIODevice::WriteOnly);
    qFile.write("\xFD\x87");
    qFile.close();

    // Instead of reading the bad thumbnail, we should now downscale
    // from the original image

    Quill::releaseAndWait();
    QVERIFY(Unittests::compareImage(file->image(), thumbImage));
    delete file;
}

void ut_error::testThumbnailDirectoryCreateFailed()
{
    QFile dummyFile("/tmp/invalid");
    dummyFile.open(QIODevice::WriteOnly);

    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QString thumbFileName = File::filePathHash(testFile.fileName());
    thumbFileName.append(".");
    thumbFileName.append(Strings::png);
    thumbFileName.prepend("/tmp/invalid/");

    Quill::setPreviewLevelCount(1);
    Quill::setPreviewSize(0, QSize(4, 1));
    Quill::setThumbnailBasePath("/tmp");
    Quill::setThumbnailFlavorName(0, "invalid");

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    file->setDisplayLevel(0);
    Quill::releaseAndWait(); // load

    QuillError error = spy.first().first().value<QuillError>();

    QCOMPARE(error.errorCode(), QuillError::DirCreateError);
    QCOMPARE(error.errorSource(), QuillError::ThumbnailErrorSource);
    QCOMPARE(error.errorData(), QString("/tmp/invalid"));

    error = file->error();
    QCOMPARE(error.errorCode(), QuillError::DirCreateError);
    QCOMPARE(error.errorSource(), QuillError::ThumbnailErrorSource);
    QCOMPARE(error.errorData(), QString("/tmp/invalid"));

    QVERIFY(!QFile(thumbFileName).exists());
    QVERIFY(Quill::isThumbnailCreationEnabled());
    delete file;
}

void ut_error::testTemporaryFileDirectoryCreateFailed()
{
    QFile dummyFile("/tmp/invalid");
    dummyFile.open(QIODevice::WriteOnly);

    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    Quill::setTemporaryFilePath("/tmp/invalid");
    Quill::setEditHistoryPath(editHistoryPath);

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QImage targetImage = filter->apply(image);

    file->runFilter(filter);
    QCOMPARE(spy.count(), 0);
    file->save();
    QCOMPARE(spy.count(), 1);

    QuillError error = spy.first().first().value<QuillError>();

    QCOMPARE(error.errorCode(), QuillError::DirCreateError);
    QCOMPARE(error.errorSource(), QuillError::TemporaryFileErrorSource);
    QCOMPARE(error.errorData(), QString("/tmp/invalid"));

    error = file->error();
    QCOMPARE(error.errorCode(), QuillError::DirCreateError);
    QCOMPARE(error.errorSource(), QuillError::TemporaryFileErrorSource);
    QCOMPARE(error.errorData(), QString("/tmp/invalid"));

    QCOMPARE(QImage(testFile.fileName()), QImage(image));
    delete file;
}

void ut_error::testUnreadableEditHistory()
{
    if (Unittests::isRoot()) {
        qDebug() << "Running as root, disabling file permissions test!";
        return;
    }

    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    // Create original since edit history will be ignored without it
    QFileInfo fileInfo(testFile.fileName());
    QString originalFileName =
        fileInfo.path() + Strings::slashOriginal + fileInfo.fileName();
    QDir().mkpath(fileInfo.path() + Strings::slashOriginal);
    image.save(originalFileName, "png");

    Quill::setEditHistoryPath(editHistoryPath);
    const QString editHistoryFileName =
        File::editHistoryFileName(testFile.fileName(), editHistoryPath);

    QFile editHistoryFile(editHistoryFileName);
    editHistoryFile.open(QIODevice::WriteOnly);
    editHistoryFile.close();

    editHistoryFile.setPermissions(0);

    QSignalSpy spy(Quill::instance(), SIGNAL(error(QuillError)));

    QuillFile *file = new QuillFile(testFile.fileName());
    QVERIFY(!file->supportsEditing()); //Generates the error signal
    QCOMPARE(spy.count(), 1);
    QuillError error = spy.first().first().value<QuillError>();

    QCOMPARE(error.errorCode(), QuillError::FileOpenForReadError);
    QCOMPARE(error.errorSource(), QuillError::EditHistoryErrorSource);
    QCOMPARE(error.errorData(), editHistoryFileName);

    editHistoryFile.remove();
    delete file;
}

void ut_error::testEmptyEditHistory()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    // Create original since edit history will be ignored without it
    QFileInfo fileInfo(testFile.fileName());
    QString originalFileName =
        fileInfo.path() + Strings::slashOriginal + fileInfo.fileName();
    QDir().mkpath(fileInfo.path() + Strings::slashOriginal);
    image.save(originalFileName, "png");

    Quill::setEditHistoryPath(editHistoryPath);
    const QString editHistoryFileName =
        File::editHistoryFileName(testFile.fileName(), editHistoryPath);

    QFile editHistoryFile(editHistoryFileName);
    editHistoryFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
    editHistoryFile.close();

    QSignalSpy spy(Quill::instance(), SIGNAL(error(QuillError)));

    QuillFile *file = new QuillFile(testFile.fileName());
    QVERIFY(!file->supportsEditing());//Generates the error signal
    QCOMPARE(spy.count(), 1);
    QuillError error = spy.first().first().value<QuillError>();

    QCOMPARE(error.errorCode(), QuillError::FileReadError);
    QCOMPARE(error.errorSource(), QuillError::EditHistoryErrorSource);
    QCOMPARE(error.errorData(), editHistoryFileName);
    delete file;
}

void ut_error::testCorruptEditHistory()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    // Create original since edit history will be ignored without it
    QFileInfo fileInfo(testFile.fileName());
    QString originalFileName =
        fileInfo.path() + Strings::slashOriginal + fileInfo.fileName();
    QDir().mkpath(fileInfo.path() + Strings::slashOriginal);
    image.save(originalFileName, "png");

    Quill::setEditHistoryPath(editHistoryPath);
    const QString editHistoryFileName =
        File::editHistoryFileName(testFile.fileName(), editHistoryPath);

    QFile editHistoryFile(editHistoryFileName);
    editHistoryFile.open(QIODevice::WriteOnly);
    editHistoryFile.write("unintelligible");
    editHistoryFile.close();

    QSignalSpy spy(Quill::instance(), SIGNAL(error(QuillError)));

    QuillFile *file = new QuillFile(testFile.fileName());
    QVERIFY(!file->supportsEditing()); //Generates the error signal
    QCOMPARE(spy.count(), 1);
    QuillError error = spy.first().first().value<QuillError>();

    QCOMPARE(error.errorCode(), QuillError::FileCorruptError);
    QCOMPARE(error.errorSource(), QuillError::EditHistoryErrorSource);
    QCOMPARE(error.errorData(), editHistoryFileName);
    delete file;
}

void ut_error::testEditHistoryDirectoryCreateFailed()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    // Should ensure that a directory will not be created.
    QFile dummyFile("/tmp/invalid");
    dummyFile.open(QIODevice::WriteOnly);

    Quill::setEditHistoryPath("/tmp/invalid");

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);

    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    file->runFilter(filter);
    file->save();

    Quill::releaseAndWait(); // load
    QCOMPARE(spy.count(), 0);
    Quill::releaseAndWait(); // filter
    QCOMPARE(spy.count(), 0);
    Quill::releaseAndWait(); // save

    QCOMPARE(spy.count(), 1);
    QuillError error = spy.first().first().value<QuillError>();

    QCOMPARE(error.errorCode(), QuillError::DirCreateError);
    QCOMPARE(error.errorSource(), QuillError::EditHistoryErrorSource);
    delete file;
}

void ut_error::testWriteProtectedEditHistory()
{
    if (Unittests::isRoot()) {
        qDebug() << "Running as root, disabling file permissions test!";
        return;
    }

    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    Quill::setEditHistoryPath(editHistoryPath);
    const QString editHistoryFileName =
        File::editHistoryFileName(testFile.fileName(), editHistoryPath);

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    file->runFilter(filter);
    file->save();

    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // filter
    Quill::releaseAndWait(); // save

    // Make the file write protected

    QFile editHistoryFile(editHistoryFileName);
    QVERIFY(editHistoryFile.exists());
    editHistoryFile.setPermissions(QFile::ReadOwner);

    QuillImageFilter *filter2 =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter2);
    filter2->setOption(QuillImageFilter::Brightness, QVariant(30));

    file->runFilter(filter2);
    file->save();

    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // filter
    Quill::releaseAndWait(); // filter2
    Quill::releaseAndWait(); // save

    QCOMPARE(spy.count(), 1);
    QuillError error = spy.first().first().value<QuillError>();

    QCOMPARE(error.errorCode(), QuillError::FileOpenForWriteError);
    QCOMPARE(error.errorSource(), QuillError::EditHistoryErrorSource);
    QCOMPARE(error.errorData(), editHistoryFile.fileName());

    delete file;

    // The file should be immediately be recognized as write protected
    QSignalSpy spy2(Quill::instance(), SIGNAL(error(QuillError)));
    file = new QuillFile(testFile.fileName(), Strings::png);
    QVERIFY(!file->supportsEditing()); //generates error sigal
    QCOMPARE(spy2.count(), 1);
    QVERIFY(!file->supportsEditing());

    delete file;
}

void ut_error::testDestructiveFilter()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_Crop);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::CropRectangle, QVariant(QRect()));

    file->runFilter(filter);

    // Verify that the filter was rejected
    QVERIFY(!file->canUndo());
    QVERIFY(!file->canRedo());

    delete file;
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_error test;
    return QTest::qExec( &test, argc, argv );
}
