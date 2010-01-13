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

ut_error::ut_error()
{
}

void ut_error::initTestCase()
{
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

    QCOMPARE((int)error.errorCode(), (int)QuillError::FileOpenForReadError);

    delete file;
}

void ut_error::testEditHistoryReadFailed()
{
    QuillError error;

    File *file = File::readFromEditHistory(QString(), &error);

    QVERIFY(!file);
    QCOMPARE((int)error.errorCode(), (int)QuillError::FileNotFoundError);
    QCOMPARE((int)error.errorSource(), (int)QuillError::EditHistoryErrorSource);
    delete file;
}

void ut_error::testEditHistoryWriteFailed()
{
    // Should ensure that a directory will not be created.
    QFile dummyFile("/tmp/invalid");
    dummyFile.open(QIODevice::WriteOnly);

    Quill::setEditHistoryDirectory("/tmp/invalid");

    QuillError error;

    File *file = new File;

    file->writeEditHistory(QString(), &error);

    QCOMPARE((int)error.errorCode(), (int)QuillError::DirCreateError);
    QCOMPARE((int)error.errorSource(), (int)QuillError::EditHistoryErrorSource);
    delete file;
}

void ut_error::testFileNotFound()
{
    QuillFile *file = new QuillFile(QString());
    QSignalSpy spy(file, SIGNAL(error(QuillError)));
    file->setDisplayLevel(0);
    Quill::releaseAndWait();

    QCOMPARE(spy.count(), 1);
    QuillError error = spy.first().first().value<QuillError>();

    QCOMPARE((int)error.errorCode(), (int)QuillError::FileNotFoundError);
    QCOMPARE((int)error.errorSource(), (int)QuillError::ImageFileErrorSource);
    QCOMPARE(error.errorData(), QString());

    QVERIFY(!file->exists());
    delete file;
}

void ut_error::testForbiddenRead()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");
    testFile.setPermissions(0);

    QuillFile *file = new QuillFile(testFile.fileName(), "png");
    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    file->setDisplayLevel(0);
    Quill::releaseAndWait();

    QVERIFY(file->image().isNull());
    QCOMPARE(spy.count(), 1);
    QuillError error = spy.first().first().value<QuillError>();

    QEXPECT_FAIL("", "QImageReader does not differentiate between nonexistent and unreadable files", Continue);
    QCOMPARE((int)error.errorCode(), (int)QuillError::FileOpenForReadError);
    QCOMPARE((int)error.errorSource(), (int)QuillError::ImageFileErrorSource);
    QCOMPARE(error.errorData(), testFile.fileName());
    delete file;
}

void ut_error::testEmptyFileRead()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillFile *file = new QuillFile(testFile.fileName(), "png");
    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    file->setDisplayLevel(0);
    Quill::releaseAndWait();

    QVERIFY(file->image().isNull());
    QCOMPARE(spy.count(), 1);
    QuillError error = spy.first().first().value<QuillError>();

    // This will provide an unsupported format error instead of a read error
    QCOMPARE((int)error.errorCode(), (int)QuillError::FileFormatUnsupportedError);
    QCOMPARE((int)error.errorSource(), (int)QuillError::ImageFileErrorSource);
    QCOMPARE(error.errorData(), testFile.fileName());

    QCOMPARE(file->supported(), false);
    delete file;
}

void ut_error::testCorruptRead()
{
    QTemporaryFile testFile;
    testFile.open();
    QString fileName = testFile.fileName();

    // Construct a corrupt PNG image
    QuillImage image = Unittests::generatePaletteImage();

    QByteArray buffer;
    QBuffer device(&buffer);
    QImageWriter writer(&device, QByteArray("png"));
    writer.write(image);

    buffer.chop(2); // remove 2 bytes from end
    testFile.write(buffer);
    testFile.close();

    QuillFile *file = new QuillFile(fileName, "png");
    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    file->setDisplayLevel(0);
    Quill::releaseAndWait();

    QVERIFY(file->image().isNull());
    QCOMPARE(spy.count(), 1);
    QuillError error = spy.first().first().value<QuillError>();

    QCOMPARE((int)error.errorCode(), (int)QuillError::FileCorruptError);
    QCOMPARE((int)error.errorSource(), (int)QuillError::ImageFileErrorSource);
    QCOMPARE(error.errorData(), fileName);
    delete file;
}

void ut_error::testForbiddenOriginal()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillFile *file = new QuillFile(testFile.fileName(), "png");
    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("org.maemo.composite.brightness.contrast");
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
    QFile originalFile(fileInfo.path() + "/.original/" +
                       fileInfo.fileName());
    QVERIFY(originalFile.exists());
    originalFile.setPermissions(0);

    delete file;
    QSignalSpy spy2(Quill::instance(), SIGNAL(error(QuillError)));
    file = new QuillFile(testFile.fileName(), "png");

    file->setDisplayLevel(0);
    Quill::releaseAndWait();
    QCOMPARE(file->image(), QuillImage(targetImage));

    // force the loading of original here
    file->undo();
    Quill::releaseAndWait();

    QCOMPARE(spy2.count(), 1);
    QuillError error = spy2.first().first().value<QuillError>();

    QEXPECT_FAIL("", "QImageReader does not differentiate between nonexistent and unreadable files", Continue);
    QCOMPARE((int)error.errorCode(), (int)QuillError::FileOpenForReadError);
    QCOMPARE((int)error.errorSource(), (int)QuillError::ImageOriginalErrorSource);
    QCOMPARE(error.errorData(), originalFile.fileName());

    // Expect that the file can still be used,
    // just that no images before the current state can be viewed

    QVERIFY(!file->canUndo());
    QVERIFY(file->image().isNull());

    QVERIFY(file->canRedo());
    file->redo();
    QCOMPARE(spy2.count(), 1); // No further errors
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

    QuillFile *file = new QuillFile(testFile.fileName(), "png");
    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("org.maemo.composite.brightness.contrast");
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
    QFile originalFile(fileInfo.path() + "/.original/" +
                       fileInfo.fileName());
    QVERIFY(originalFile.exists());
    originalFile.open(QIODevice::WriteOnly);
    originalFile.write("\xFD\x87");
    originalFile.close();

    delete file;
    QSignalSpy spy2(Quill::instance(), SIGNAL(error(QuillError)));
    file = new QuillFile(testFile.fileName(), "png");

    file->setDisplayLevel(0);
    Quill::releaseAndWait();
    QCOMPARE(file->image(), QuillImage(targetImage));

    // force the loading of original here
    file->undo();
    Quill::releaseAndWait();

    QCOMPARE(spy2.count(), 1);
    QuillError error = spy2.first().first().value<QuillError>();

    QCOMPARE((int)error.errorCode(), (int)QuillError::FileFormatUnsupportedError);
    QCOMPARE((int)error.errorSource(), (int)QuillError::ImageOriginalErrorSource);
    QCOMPARE(error.errorData(), originalFile.fileName());

    // Expect that the file can still be used,
    // just that no images before the current state can be viewed

    QVERIFY(!file->canUndo());
    QVERIFY(file->image().isNull());

    QVERIFY(file->canRedo());
    file->redo();
    QCOMPARE(spy2.count(), 1); // No further errors
    Quill::releaseAndWait();
    QCOMPARE(file->image(), QuillImage(targetImage));

    delete file;
}

void ut_error::testCorruptOriginal()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillFile *file = new QuillFile(testFile.fileName(), "png");
    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("org.maemo.composite.brightness.contrast");
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
    QFile originalFile(fileInfo.path() + "/.original/" +
                       fileInfo.fileName());
    QVERIFY(originalFile.exists());
    QVERIFY(originalFile.resize(originalFile.size()-2));

    delete file;
    QSignalSpy spy2(Quill::instance(), SIGNAL(error(QuillError)));
    file = new QuillFile(testFile.fileName(), "png");

    file->setDisplayLevel(0);
    Quill::releaseAndWait();
    QCOMPARE(file->image(), QuillImage(targetImage));

    // force the loading of original here
    file->undo();
    Quill::releaseAndWait();

    QCOMPARE(spy2.count(), 1);
    QuillError error = spy2.first().first().value<QuillError>();

    QCOMPARE((int)error.errorCode(), (int)QuillError::FileCorruptError);
    QCOMPARE((int)error.errorSource(), (int)QuillError::ImageOriginalErrorSource);
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

void ut_error::testTemporaryFileDirectoryCreateFailed()
{
    QFile dummyFile("/tmp/invalid");
    dummyFile.open(QIODevice::WriteOnly);

    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    Quill::setTemporaryFilePath("/tmp/invalid");
    Quill::setEditHistoryDirectory("/tmp/quill/history");

    QuillFile *file = new QuillFile(testFile.fileName(), "png");
    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("org.maemo.composite.brightness.contrast");
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QImage targetImage = filter->apply(image);

    file->runFilter(filter);
    QCOMPARE(spy.count(), 0);
    file->save();
    QCOMPARE(spy.count(), 1);

    QuillError error = spy.first().first().value<QuillError>();

    QCOMPARE((int)error.errorCode(), (int)QuillError::DirCreateError);
    QCOMPARE((int)error.errorSource(), (int)QuillError::TemporaryFileErrorSource);
    QCOMPARE(error.errorData(), QString("/tmp/invalid"));

    QCOMPARE(QImage(testFile.fileName()), QImage(image));
}

void ut_error::testUnreadableEditHistory()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    const QString editHistoryDirectory = "/tmp/quill/history";
    QDir().mkpath(editHistoryDirectory);

    Quill::setEditHistoryDirectory(editHistoryDirectory);
    const QString editHistoryFileName =
        File::editHistoryFileName(testFile.fileName(), editHistoryDirectory);

    QFile editHistoryFile(editHistoryFileName);
    editHistoryFile.open(QIODevice::WriteOnly);
    editHistoryFile.close();

    editHistoryFile.setPermissions(0);

    QSignalSpy spy(Quill::instance(), SIGNAL(error(QuillError)));

    QuillFile *file = new QuillFile(testFile.fileName());
    Q_UNUSED(file);

    QCOMPARE(spy.count(), 1);
    QuillError error = spy.first().first().value<QuillError>();

    QCOMPARE((int)error.errorCode(), (int)QuillError::FileOpenForReadError);
    QCOMPARE((int)error.errorSource(), (int)QuillError::EditHistoryErrorSource);
    QCOMPARE(error.errorData(), editHistoryFileName);
    delete file;
}

void ut_error::testEmptyEditHistory()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    const QString editHistoryDirectory = "/tmp/quill/history";
    QDir().mkpath(editHistoryDirectory);

    Quill::setEditHistoryDirectory(editHistoryDirectory);
    const QString editHistoryFileName =
        File::editHistoryFileName(testFile.fileName(), editHistoryDirectory);

    QFile editHistoryFile(editHistoryFileName);
    editHistoryFile.open(QIODevice::WriteOnly);
    editHistoryFile.close();

    QSignalSpy spy(Quill::instance(), SIGNAL(error(QuillError)));

    QuillFile *file = new QuillFile(testFile.fileName());
    Q_UNUSED(file);

    QCOMPARE(spy.count(), 1);
    QuillError error = spy.first().first().value<QuillError>();

    QCOMPARE((int)error.errorCode(), (int)QuillError::FileReadError);
    QCOMPARE((int)error.errorSource(), (int)QuillError::EditHistoryErrorSource);
    QCOMPARE(error.errorData(), editHistoryFileName);
    delete file;
}

void ut_error::testCorruptEditHistory()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    const QString editHistoryDirectory = "/tmp/quill/history";
    QDir().mkpath(editHistoryDirectory);

    Quill::setEditHistoryDirectory(editHistoryDirectory);
    const QString editHistoryFileName =
        File::editHistoryFileName(testFile.fileName(), editHistoryDirectory);

    QFile editHistoryFile(editHistoryFileName);
    editHistoryFile.open(QIODevice::WriteOnly);
    editHistoryFile.write("unintelligible");
    editHistoryFile.close();

    QSignalSpy spy(Quill::instance(), SIGNAL(error(QuillError)));

    QuillFile *file = new QuillFile(testFile.fileName());
    Q_UNUSED(file);

    QCOMPARE(spy.count(), 1);
    QuillError error = spy.first().first().value<QuillError>();

    QCOMPARE((int)error.errorCode(), (int)QuillError::FileCorruptError);
    QCOMPARE((int)error.errorSource(), (int)QuillError::EditHistoryErrorSource);
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

    Quill::setEditHistoryDirectory("/tmp/invalid");

    QuillFile *file = new QuillFile(testFile.fileName(), "png");

    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("org.maemo.composite.brightness.contrast");
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

    QCOMPARE((int)error.errorCode(), (int)QuillError::DirCreateError);
    QCOMPARE((int)error.errorSource(), (int)QuillError::EditHistoryErrorSource);
    delete file;
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_error test;
    return QTest::qExec( &test, argc, argv );
}
