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
#include <QuillImage>
#include <QDebug>
#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include "unittests.h"
#include "ut_stack.h"
#include "quillundocommand.h"
#include "quillundostack.h"
#include "core.h"
#include "quillfile.h"

class CorePrivate;

ut_stack::ut_stack()
{
}

void ut_stack::initTestCase()
{
    QuillImageFilter::registerAll();
    QDir().mkpath("/tmp/quill/history");
}

void ut_stack::cleanupTestCase()
{
}

void ut_stack::testSessionSetup()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImageFilter *filter2 =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Contrast, QVariant(20));

    Quill *quill = new Quill(QSize(8, 2), Quill::ThreadingTest);
    QVERIFY(quill);
    QuillFile *file = quill->file(testFile.fileName(), "png");

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

    delete quill;
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
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImageFilter *filter2 =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Contrast, QVariant(20));

    Quill *quill = new Quill(QSize(8, 2), Quill::ThreadingTest);
    QVERIFY(quill);
    QuillFile *file = quill->file(testFile.fileName(), "png");

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
}

void ut_stack::testSessionUndoRedo()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));
    QuillImage resultImage = filter->apply(image);

    QuillImageFilter *filter2 =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    QVERIFY(filter);
    filter2->setOption(QuillImageFilter::Contrast, QVariant(20));
    QuillImage resultImage2 = filter2->apply(resultImage);

    QuillImageFilter *filter3 =
        QuillImageFilterFactory::createImageFilter("Flip");
    QVERIFY(filter);
    QuillImage resultImage3 = filter3->apply(resultImage2);

    QuillImageFilter *filter4 =
        QuillImageFilterFactory::createImageFilter("Rotate");
    QVERIFY(filter);
    filter4->setOption(QuillImageFilter::Angle, QVariant(90));
    QuillImage resultImage4 = filter4->apply(resultImage3);

    Quill *quill = new Quill(QSize(8, 2), Quill::ThreadingTest);
    QVERIFY(quill);
    quill->setEditHistoryCacheSize(0, 5);
    QuillFile *file = quill->file(testFile.fileName(), "png");

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
    quill->releaseAndWait();
    quill->releaseAndWait();
    quill->releaseAndWait();
    quill->releaseAndWait();
    quill->releaseAndWait();

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

    delete quill;
}

void ut_stack::testSessionSaveLoad()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));
    QuillImage resultImage = filter->apply(image);

    QuillImageFilter *filter2 =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    QVERIFY(filter);
    filter2->setOption(QuillImageFilter::Contrast, QVariant(20));
    QuillImage resultImage2 = filter2->apply(resultImage);

    QuillImageFilter *filter3 =
        QuillImageFilterFactory::createImageFilter("Flip");
    QVERIFY(filter);
    QuillImage resultImage3 = filter3->apply(resultImage2);

    QuillImageFilter *filter4 =
        QuillImageFilterFactory::createImageFilter("Rotate");
    QVERIFY(filter);
    filter4->setOption(QuillImageFilter::Angle, QVariant(90));
    QuillImage resultImage4 = filter4->apply(resultImage3);

    Quill *quill = new Quill(QSize(8, 2), Quill::ThreadingTest);
    QVERIFY(quill);
    quill->setEditHistoryDirectory("/tmp/quill/history");
    quill->setEditHistoryCacheSize(0, 5);

    QuillFile *file = quill->file(testFile.fileName(), "png");

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

    quill->releaseAndWait(); // load
    quill->releaseAndWait(); // filter1
    quill->releaseAndWait(); // filter2
    quill->releaseAndWait(); // save

    QVERIFY(Unittests::compareImage(QImage(testFile.fileName()), resultImage2));

    delete quill;

    Quill *quill2 = new Quill(QSize(8, 2), Quill::ThreadingTest);
    QVERIFY(quill2);
    quill2->setEditHistoryDirectory("/tmp/quill/history");
    quill2->setEditHistoryCacheSize(0, 5);

    QuillFile *file2 = quill2->file(testFile.fileName(), "png");
    file2->setDisplayLevel(0);

    quill2->releaseAndWait(); // load

    QVERIFY(Unittests::compareImage(file2->image(), resultImage2));

    file2->redo();
    quill2->releaseAndWait(); // filter3
    quill2->releaseAndWait(); // filter4

    QVERIFY(Unittests::compareImage(file2->image(), resultImage4));

    file2->undo();
    QVERIFY(Unittests::compareImage(file2->image(), resultImage2));

    file2->undo();
    quill2->releaseAndWait(); // load
    QVERIFY(Unittests::compareImage(file2->image(), image));

    delete quill2;
}

void ut_stack::testSetImage()
{
    QTemporaryFile testFile;
    testFile.open();

    QImage image = Unittests::generatePaletteImage();

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));
    QuillImage resultImage = filter->apply(image);

    Quill *quill = new Quill(QSize(8, 2), Quill::ThreadingTest);
    QVERIFY(quill);
    quill->setEditHistoryDirectory("/tmp/quill/history");
    //    quill->setEditHistoryCacheSize(0, 2);

    QuillFile *file = quill->file(testFile.fileName(), "png");

    file->setDisplayLevel(0);
    file->setImage(0, image);

    QVERIFY(Unittests::compareImage(file->image(), image));

    file->runFilter(filter);
    quill->releaseAndWait();

    QVERIFY(Unittests::compareImage(file->image(), resultImage));

    delete quill;
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_stack test;
    return QTest::qExec( &test, argc, argv );
}
