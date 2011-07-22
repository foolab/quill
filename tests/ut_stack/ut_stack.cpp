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
#include <QuillImage>
#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include "unittests.h"
#include "ut_stack.h"
#include "quillundocommand.h"
#include "quillundostack.h"
#include "core.h"
#include "quillfile.h"
#include "../../src/strings.h"

class CorePrivate;

ut_stack::ut_stack()
{
}

void ut_stack::initTestCase()
{
    QDir().mkpath("/tmp/quill/history");
}

void ut_stack::cleanupTestCase()
{
}

void ut_stack::init()
{
    Quill::initTestingMode();
    Quill::setPreviewSize(0, QSize(8, 2));
}

void ut_stack::cleanup()
{
    Quill::cleanup();
}

void ut_stack::testSessionSetup()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImageFilter *filter2 =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Contrast, QVariant(20));

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);

    file->runFilter(filter);
    file->runFilter(filter2);
    file->undo();

    QVERIFY(!file->isSession());
    QVERIFY(file->canUndo());
    QVERIFY(file->canRedo());

    file->startSession();

    // We are in a session now, cannot undo or redo
    QVERIFY(file->isSession());
    QVERIFY(!file->canUndo());
    QVERIFY(!file->canRedo());

    // If we end the session now, we should still be able to undo and redo
    file->endSession();
    QVERIFY(!file->isSession());
    QVERIFY(file->canUndo());
    QVERIFY(file->canRedo());

    delete file;
}

// Make sure that undo/redo during the recording of a session does not undo
// more than one command at a time

void ut_stack::testUndoRedoDuringSessionRecording()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImageFilter *filter2 =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Contrast, QVariant(20));

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);

    file->startSession();
    file->runFilter(filter);
    file->runFilter(filter2);

    file->undo();
    // Between commands, we should still be able to undo
    QVERIFY(file->canUndo());
    file->undo();

    QVERIFY(file->canRedo());

    // Between commands, we should still be able to redo
    file->redo();
    QVERIFY(file->canRedo());

    delete file;
}

void ut_stack::testSessionUndoRedo()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));
    QuillImage resultImage = filter->apply(image);

    QuillImageFilter *filter2 =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter2->setOption(QuillImageFilter::Contrast, QVariant(20));
    QuillImage resultImage2 = filter2->apply(resultImage);

    QuillImageFilter *filter3 =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_Flip);
    QVERIFY(filter);
    QuillImage resultImage3 = filter3->apply(resultImage2);

    QuillImageFilter *filter4 =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_Rotate);
    QVERIFY(filter);
    filter4->setOption(QuillImageFilter::Angle, QVariant(90));
    QuillImage resultImage4 = filter4->apply(resultImage3);

    Quill::setEditHistoryCacheSize(0, 5);
    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);

    file->runFilter(filter);

    file->startSession();
    file->runFilter(filter2);

    // Inside a session, we should be able to undo
    QVERIFY(file->canUndo());
    QVERIFY(!file->canRedo());

    file->runFilter(filter3);
    file->endSession();

    // Should still be able to undo
    QVERIFY(file->canUndo());

    file->runFilter(filter4);

    // Get up to date
    file->setDisplayLevel(0);
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    QVERIFY(Unittests::compareImage(file->image(), resultImage4));

    // Undo - single command

    file->undo();
    QVERIFY(Unittests::compareImage(file->image(), resultImage3));

    // Undo - session command

    file->undo();
    QVERIFY(Unittests::compareImage(file->image(), resultImage));

    QVERIFY(file->canUndo());
    QVERIFY(file->canRedo());

    // Redo - session command

    file->redo();
    QVERIFY(Unittests::compareImage(file->image(), resultImage3));

    delete file;
}

void ut_stack::testSessionSaveLoad()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));
    QuillImage resultImage = filter->apply(image);

    QuillImageFilter *filter2 =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter2->setOption(QuillImageFilter::Contrast, QVariant(20));
    QuillImage resultImage2 = filter2->apply(resultImage);

    QuillImageFilter *filter3 =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_Flip);
    QVERIFY(filter);
    QuillImage resultImage3 = filter3->apply(resultImage2);

    QuillImageFilter *filter4 =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_Rotate);
    QVERIFY(filter);
    filter4->setOption(QuillImageFilter::Angle, QVariant(90));
    QuillImage resultImage4 = filter4->apply(resultImage3);

    Quill::setEditHistoryPath("/tmp/quill/history");
    Quill::setEditHistoryCacheSize(0, 5);

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);

    file->startSession();
    file->runFilter(filter);
    file->runFilter(filter2);
    file->endSession();
    file->startSession();
    file->runFilter(filter3);
    file->runFilter(filter4);
    file->endSession();
    file->undo();

    file->save();

    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // filter1
    Quill::releaseAndWait(); // filter2
    Quill::releaseAndWait(); // save

    QVERIFY(Unittests::compareImage(QImage(testFile.fileName()), resultImage2));

    Quill::cleanup();
    Quill::initTestingMode();
    Quill::setPreviewSize(0, QSize(8, 2));

    Quill::setEditHistoryPath("/tmp/quill/history");
    Quill::setEditHistoryCacheSize(0, 5);

    QuillFile *file2 = new QuillFile(testFile.fileName(), Strings::png);
    file2->setDisplayLevel(0);

    Quill::releaseAndWait(); // load

    QVERIFY(Unittests::compareImage(file2->image(), resultImage2));

    file2->redo();
    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // filter3
    Quill::releaseAndWait(); // filter4
    QVERIFY(Unittests::compareImage(file2->image(), resultImage4));

    file2->undo();
    QVERIFY(Unittests::compareImage(file2->image(), resultImage2));

    file2->undo();
    Quill::releaseAndWait(); // load
    QVERIFY(Unittests::compareImage(file2->image(), image));

    delete file;
    delete file2;
}

void ut_stack::testSetImage()
{
    Quill::setPreviewLevelCount(3);

    QImage image = Unittests::generatePaletteImage();

    Quill::setEditHistoryPath("/tmp/quill/history");
    Quill::setEditHistoryCacheSize(0, 2);

    QuillFile *file = new QuillFile("/tmp/quill/invalid", Strings::jpegMimeType);
    QSignalSpy spy(file, SIGNAL(error(QuillError)));

    QuillImage quillImage(image);
    quillImage.setFullImageSize(QSize(8, 2));
    file->setImage(0, quillImage);

    QCOMPARE(file->displayLevel(), 0);

    // There should be no errors from setting an image
    QCOMPARE(spy.count(), 0);
    QVERIFY(file->error().errorCode() == QuillError::NoError);
    // Check that data is correct
    QVERIFY(Unittests::compareImage(file->image(), image));

    // Check that higher display levels are also set
    file->setImage(1, quillImage);
    QCOMPARE(file->displayLevel(), 1);

    file->setImage(2, quillImage);
    QCOMPARE(file->displayLevel(), 2);

    delete file;
}

void ut_stack::testRefresh()
{
    QString fileName = "/tmp/quill/pctest.png";

    QFile::remove(fileName);
    QVERIFY(!QFile::exists(fileName));

    QImage image = Unittests::generatePaletteImage();

    QuillFile *file = new QuillFile(fileName, Strings::png);
    QVERIFY(!file->exists());

    file->setImage(0, image);

    QVERIFY(Unittests::compareImage(file->image(), image));

    image.save(fileName, "png");

    file->refresh();
    QVERIFY(file->exists());
    QCOMPARE(file->displayLevel(), 0);
    QVERIFY(file->image(0).isNull()); // Temporary images are thrown away

    Quill::releaseAndWait(); // 0

    QVERIFY(Unittests::compareImage(file->image(0), image));

    delete file;
    QFile::remove(fileName);
}

void ut_stack::testEditAfterSetImage()
{
    QString fileName = "/tmp/quill/pctest.png";

    QFile::remove(fileName);
    QVERIFY(!QFile::exists(fileName));

    QTemporaryFile testFile;
    testFile.open();

    QImage image = Unittests::generatePaletteImage();

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));
    QuillImage resultImage = filter->apply(image);

    Quill::setEditHistoryPath("/tmp/quill/history");
    Quill::setEditHistoryCacheSize(0, 2);

    QuillFile *file = new QuillFile(fileName, Strings::pngMimeType);

    file->setDisplayLevel(0);
    QuillImage quillImage(image);
    quillImage.setFullImageSize(QSize(8, 2));
    file->setImage(0, quillImage);

    QVERIFY(Unittests::compareImage(file->image(), image));
    QCOMPARE(file->fullImageSize(), QSize(8, 2));

    file->runFilter(filter);
    Quill::releaseAndWait();

    QVERIFY(Unittests::compareImage(file->image(), resultImage));

    image.save(fileName, "png");

    file->refresh();
    Quill::releaseAndWait(); // edit (recreate)
    QVERIFY(Unittests::compareImage(file->image(), resultImage));

    file->save();
    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // edit
    Quill::releaseAndWait(); // save

    QVERIFY(Unittests::compareImage(QImage(fileName), resultImage));
    delete file;
}

void ut_stack::testImmediateSizeQuery()
{
    QTemporaryFile testFile;
    testFile.open();

    QImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    QCOMPARE(file->fullImageSize(), QSize(8, 2));

    delete file;
}

void ut_stack::testDropRedoHistory()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));
    QuillImage resultImage = filter->apply(image);

    QuillImageFilter *filter2 =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter2->setOption(QuillImageFilter::Contrast, QVariant(20));
    QuillImage resultImage2 = filter2->apply(resultImage);

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);

    file->runFilter(filter);
    file->runFilter(filter2);
    file->undo();
    QVERIFY(file->canRedo());
    file->dropRedoHistory();
    QVERIFY(!file->canRedo());

    delete file;
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_stack test;
    return QTest::qExec( &test, argc, argv );
}
