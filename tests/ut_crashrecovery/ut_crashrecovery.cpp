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
#include <QDebug>
#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include "unittests.h"
#include "ut_crashrecovery.h"
#include "quillundostack.h"
#include "quillundocommand.h"
#include "quillfile.h"

ut_crashrecovery::ut_crashrecovery()
{
}

void ut_crashrecovery::initTestCase()
{
    QuillImageFilter::registerAll();
}

void ut_crashrecovery::cleanupTestCase()
{
}

/* Test recovery for simple brightness */

void ut_crashrecovery::testBasicRecovery()
{
    QTemporaryFile testFile;
    testFile.open();

    QTemporaryFile originFile;
    originFile.open();

    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill *quill = new Quill(QSize(4, 1), Quill::ThreadingTest);

    QuillFile *file =
        quill->file(testFile.fileName(), "png");
    file->setDisplayLevel(1);

    QuillImageFilter *brightnessFilter =
        QuillImageFilterFactory::createImageFilter("org.maemo.composite.brightness.contrast");
    brightnessFilter->setOption(QuillImageFilter::Brightness,
                                QVariant(10));
    QImage image = Unittests::generatePaletteImage();
    QImage targetImage = brightnessFilter->apply(image);

    quill->releaseAndWait();
    quill->releaseAndWait();

    QVERIFY(Unittests::compareImage(file->image(), image));

    file->runFilter(brightnessFilter);
    quill->releaseAndWait();
    quill->releaseAndWait();

    QVERIFY(Unittests::compareImage(file->image(), targetImage));

    QByteArray dump = quill->dump();

    // Delete and create a new one to make sure that no data is kept

    delete quill;

    quill = new Quill(QSize(4, 1), Quill::ThreadingTest);
    file = quill->file(testFile.fileName(), "png");
    QSignalSpy spy(file, SIGNAL(saved()));

    quill->recover(dump);

    quill->releaseAndWait(); // load
    quill->releaseAndWait(); // brightness
    quill->releaseAndWait(); // save (auto)

    QCOMPARE(spy.count(), 1);

    QVERIFY(Unittests::compareImage(QImage(testFile.fileName()),
                                    targetImage));

    file = quill->file(testFile.fileName(), "png");

    quill->releaseAndWait(); // load (preview)
    quill->releaseAndWait(); // load (full)
    QVERIFY(Unittests::compareImage(file->image(), targetImage));

    delete quill;
}

/* Test recovery for simple brightness, open file in question
   immediately after crash */

void ut_crashrecovery::testRecoveryImmediateReOpen()
{
    /*    QTemporaryFile testFile;
    testFile.open();

    QTemporaryFile originFile;
    originFile.open();

    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill *quill = new Quill(QSize(4, 1), Quill::ThreadingTest);

    QImage image = Unittests::generatePaletteImage();

    quill->openFile(testFile.fileName(),
                    originFile.fileName(),
                    "", "png");

    QuillImageFilter *brightnessFilter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    brightnessFilter->setOption(QuillImageFilter::Brightness,
                                QVariant(10));

    QImage targetImage = brightnessFilter->apply(image);

    quill->releaseAndWait();
    quill->releaseAndWait();

    QVERIFY(Unittests::compareImage(quill->image(), image));

    quill->runFilter(brightnessFilter);
    quill->releaseAndWait();
    quill->releaseAndWait();

    QVERIFY(Unittests::compareImage(quill->image(), targetImage));

    QByteArray dump = quill->dump();

    // Delete and create a new one to make sure that no data is kept

    delete quill;

    quill = new Quill(QSize(4, 1), Quill::ThreadingTest);

    quill->recover(dump);

    // Re-insert empty edit history as it should be ignored anyway
    quill->openFile(testFile.fileName(),
                    originFile.fileName(),
                    "", "png");

    quill->releaseAndWait(); // load
    quill->releaseAndWait(); // load 2, since stack is lost (for preview)
    quill->releaseAndWait(); // load 2 (full image)
    quill->releaseAndWait(); // brightness (preview)
    quill->releaseAndWait(); // brightness (full)

    QVERIFY(Unittests::compareImage(quill->image(),
                                    targetImage));

    // Verify stack structure

    quill->undo();

    QVERIFY(Unittests::compareImage(quill->image(),
                                    image));

    QVERIFY(!quill->canUndo());

    delete quill;*/
}

/* Test recovery on stacks with redo history */

void ut_crashrecovery::testRecoveryAfterUndo()
{
    /*    QTemporaryFile testFile;
    testFile.open();

    QTemporaryFile originFile;
    originFile.open();

    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill *quill = new Quill(QSize(4, 1), Quill::ThreadingTest);

    QImage image = Unittests::generatePaletteImage();

    quill->openFile(testFile.fileName(),
                    originFile.fileName(),
                    "", "png");

    QuillImageFilter *brightnessFilter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    brightnessFilter->setOption(QuillImageFilter::Brightness,
                                QVariant(10));

    QuillImageFilter *contrastFilter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    brightnessFilter->setOption(QuillImageFilter::Contrast,
                                QVariant(10));

    QImage targetImage = brightnessFilter->apply(image);
    QImage finalImage = contrastFilter->apply(targetImage);

    quill->releaseAndWait();
    quill->releaseAndWait();

    QVERIFY(Unittests::compareImage(quill->image(), image));

    quill->runFilter(brightnessFilter);
    quill->releaseAndWait();
    quill->releaseAndWait();

    QVERIFY(Unittests::compareImage(quill->image(), targetImage));

    quill->runFilter(contrastFilter);
    quill->releaseAndWait();
    quill->releaseAndWait();

    QVERIFY(Unittests::compareImage(quill->image(), finalImage));

    quill->undo();

    QByteArray dump = quill->dump();

    // Delete and create a new one to make sure that no data is kept

    delete quill;

    quill = new Quill(QSize(4, 1), Quill::ThreadingTest);
    QSignalSpy spy(quill, SIGNAL(imageSaved(const QString, const QByteArray)));

    quill->recover(dump);

    quill->releaseAndWait(); // load
    quill->releaseAndWait(); // brightness
    quill->releaseAndWait(); // save (auto)

    QCOMPARE(spy.count(), 1);
    QByteArray editHistory = spy.takeFirst().at(1).toByteArray();
    QVERIFY(!editHistory.isEmpty());

    QVERIFY(Unittests::compareImage(QImage(testFile.fileName()),
                                    targetImage));

    quill->openFile(testFile.fileName(),
                    originFile.fileName(),
                    editHistory, "png");

    quill->releaseAndWait(); // load
    quill->releaseAndWait(); // load
    QVERIFY(Unittests::compareImage(quill->image(), targetImage));

    quill->redo();
    quill->releaseAndWait(); // load
    quill->releaseAndWait(); // contrast

    QVERIFY(Unittests::compareImage(quill->image(), finalImage));

    quill->undo();
    quill->undo();
    quill->releaseAndWait(); // release load (preview)
    quill->releaseAndWait(); // original load (preview)
    quill->releaseAndWait(); // original load (full)

    QVERIFY(Unittests::compareImage(quill->image(), image));

    delete quill;*/
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_crashrecovery test;
    return QTest::qExec( &test, argc, argv );
}
