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

void ut_crashrecovery::init()
{
    Quill::initTestingMode();
    Quill::setPreviewSize(0, QSize(4, 1));
}

void ut_crashrecovery::cleanup()
{
    Quill::cleanup();
}

void ut_crashrecovery::initTestCase()
{
}

void ut_crashrecovery::cleanupTestCase()
{
}

void ut_crashrecovery::testFileName()
{
    Quill::setCrashDumpPath("/tmp");
    QCOMPARE(Quill::crashDumpPath(), QString("/tmp"));
}

/* Test recovery for simple brightness */

void ut_crashrecovery::testBasicRecovery()
{
    QTemporaryFile testFile;
    testFile.open();

    QFile dumpFile("/tmp/dump.xml");

    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill::setCrashDumpPath("/tmp");

    QuillFile *file =
        new QuillFile(testFile.fileName(), "png");
    file->setDisplayLevel(1);

    QuillImageFilter *brightnessFilter =
        QuillImageFilterFactory::createImageFilter("org.maemo.composite.brightness.contrast");
    brightnessFilter->setOption(QuillImageFilter::Brightness,
                                QVariant(10));
    QImage image = Unittests::generatePaletteImage();
    QImage targetImage = brightnessFilter->apply(image);

    Quill::releaseAndWait(); // load
    Quill::releaseAndWait();

    QVERIFY(Unittests::compareImage(file->image(), image));

    file->runFilter(brightnessFilter);
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    QVERIFY(Unittests::compareImage(file->image(), targetImage));

    // Verify that the dump file has been created
    QVERIFY(dumpFile.size() > 0);

    // Simulate crash by calling cleanup here
    Quill::cleanup();

    Quill::initTestingMode();

    // This must be kept after cleanup to keep the changes from being discarded
    delete file;

    Quill::setPreviewSize(0, QSize(4, 1));

    // Dump file not set - recovery should fail
    QVERIFY(!Quill::canRecover());

    Quill::setCrashDumpPath("/tmp");

    // Now that dump file has been set, recovery should succeed
    QVERIFY(Quill::canRecover());

    Quill::recover();

    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // brightness
    Quill::releaseAndWait(); // save (auto)

    QVERIFY(Unittests::compareImage(QImage(testFile.fileName()),
                                    targetImage));

    // The dump file should now go away
    QCOMPARE((int)dumpFile.size(), 0);

    file = new QuillFile(testFile.fileName(), "png");
    file->setDisplayLevel(1);

    Quill::releaseAndWait(); // load (preview)
    Quill::releaseAndWait(); // load (full)
    QVERIFY(Unittests::compareImage(file->image(), targetImage));

    delete file;
}

/* Test recovery for simple brightness, open file in question
   immediately after crash */

void ut_crashrecovery::testRecoverSaveInProgress()
{
    QTemporaryFile testFile;
    testFile.open();

    QFile dumpFile("/tmp/dump.xml");

    QImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    Quill::setCrashDumpPath("/tmp");

    QuillFile *file =
        new QuillFile(testFile.fileName(), "png");
    file->setDisplayLevel(1);

    QuillImageFilter *brightnessFilter =
        QuillImageFilterFactory::createImageFilter("org.maemo.composite.brightness.contrast");
    brightnessFilter->setOption(QuillImageFilter::Brightness,
                                QVariant(10));

    QImage targetImage = brightnessFilter->apply(image);

    Quill::releaseAndWait(); // load
    Quill::releaseAndWait();

    QVERIFY(Unittests::compareImage(file->image(), image));

    file->runFilter(brightnessFilter);
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    QVERIFY(Unittests::compareImage(file->image(), targetImage));

    // Verify that the dump file has been created
    QVERIFY(dumpFile.size() > 0);

    file->save();
    delete file;

    // Simulate crash by calling cleanup here
    Quill::cleanup();

    Quill::initTestingMode();

    // This must be kept after cleanup to keep the changes from being discarded

    Quill::setPreviewSize(0, QSize(4, 1));

    // Dump file not set - recovery should fail
    QVERIFY(!Quill::canRecover());

    Quill::setCrashDumpPath("/tmp");

    // Now that dump file has been set, recovery should succeed
    QVERIFY(Quill::canRecover());

    Quill::recover();

    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // brightness

    // save should not yet be concluded at this point
    QVERIFY(Unittests::compareImage(QImage(testFile.fileName()),
                                    image));

    Quill::releaseAndWait(); // save

    QVERIFY(Unittests::compareImage(QImage(testFile.fileName()),
                                    targetImage));

    // The dump file should now go away
    QCOMPARE((int)dumpFile.size(), 0);

    file = new QuillFile(testFile.fileName(), "png");
    file->setDisplayLevel(1);

    Quill::releaseAndWait(); // load (preview)
    Quill::releaseAndWait(); // load (full)
    QVERIFY(Unittests::compareImage(file->image(), targetImage));

    delete file;
}

/* Test recovery on stacks with redo history */

void ut_crashrecovery::testRecoveryAfterUndo()
{
    QTemporaryFile testFile;
    testFile.open();

    QFile dumpFile("/tmp/dump.xml");

    QImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    Quill::setCrashDumpPath("/tmp");

    QuillFile *file = new QuillFile(testFile.fileName(), "png");

    QuillImageFilter *brightnessFilter =
        QuillImageFilterFactory::createImageFilter("org.maemo.composite.brightness.contrast");
    brightnessFilter->setOption(QuillImageFilter::Brightness,
                                QVariant(10));

    QuillImageFilter *contrastFilter =
        QuillImageFilterFactory::createImageFilter("org.maemo.composite.brightness.contrast");
    contrastFilter->setOption(QuillImageFilter::Contrast,
                              QVariant(10));

    QImage targetImage = brightnessFilter->apply(image);
    QImage finalImage = contrastFilter->apply(targetImage);

    file->setDisplayLevel(1);

    Quill::releaseAndWait();
    Quill::releaseAndWait();

    QVERIFY(Unittests::compareImage(file->image(), image));

    file->runFilter(brightnessFilter);
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    QVERIFY(Unittests::compareImage(file->image(), targetImage));

    file->runFilter(contrastFilter);
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    QVERIFY(Unittests::compareImage(file->image(), finalImage));

    file->undo();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    // Simulate shutdown
    Quill::cleanup();
    Quill::initTestingMode();
    delete file;

    Quill::setPreviewSize(0, QSize(4, 1));

    Quill::setCrashDumpPath("/tmp");
    QVERIFY(Quill::canRecover());

    Quill::recover();

    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // brightness
    Quill::releaseAndWait(); // save (auto)

    QVERIFY(Unittests::compareImage(QImage(testFile.fileName()),
                                    targetImage));

    Quill::setPreviewSize(0, QSize(4, 1));
    file = new QuillFile(testFile.fileName(), "png");
    file->setDisplayLevel(1);

    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // load
    QVERIFY(Unittests::compareImage(file->image(), targetImage));

    file->redo();
    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // contrast

    QVERIFY(Unittests::compareImage(file->image(), finalImage));

    file->undo();
    file->undo();
    Quill::releaseAndWait(); // release load (preview)
    Quill::releaseAndWait(); // original load (preview)
    Quill::releaseAndWait(); // original load (full)

    QVERIFY(Unittests::compareImage(file->image(), image));
    delete file;
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_crashrecovery test;
    return QTest::qExec( &test, argc, argv );
}
