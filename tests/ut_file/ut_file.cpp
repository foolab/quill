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
#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include <Quill>

#include "quillerror.h"
#include "core.h"
#include "file.h"
#include "ut_file.h"
#include "unittests.h"

ut_file::ut_file()
{
}

void ut_file::initTestCase()
{
}

void ut_file::cleanupTestCase()
{
}

void ut_file::init()
{
    Quill::initTestingMode();
}

void ut_file::cleanup()
{
    Quill::cleanup();
}

void ut_file::testRemove()
{
    QTemporaryFile testFile;
    testFile.open();
    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    QuillFile *file = new QuillFile(testFile.fileName(), "png");
    QVERIFY(file);
    QVERIFY(file->exists());

    QString originalFileName = file->originalFileName();

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("org.maemo.composite.brightness.contrast");
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    file->runFilter(filter);
    file->save();
    Quill::releaseAndWait(); // for runFilter()

    file->remove();

    QVERIFY(!file->exists());
    QVERIFY(!QFile::exists(testFile.fileName()));
    QVERIFY(!QFile::exists(originalFileName));

    Quill::releaseAndWait(); // for save()

    QVERIFY(!file->exists());
    QVERIFY(!QFile::exists(testFile.fileName()));
    QVERIFY(!QFile::exists(originalFileName));

    delete file;
}

void ut_file::testOriginal()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    Quill::setFileLimit(0, 2);
    QuillFile *file = new QuillFile(testFile.fileName(), "png");

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("org.maemo.composite.brightness.contrast");
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImage processedImage = filter->apply(image);

    file->runFilter(filter);

    QuillFile *original = file->original();
    QCOMPARE(file->fileName(), original->fileName());
    QCOMPARE(file->originalFileName(), original->originalFileName());
    QVERIFY(file->canUndo());
    QVERIFY(!original->canUndo());

    file->setDisplayLevel(0);
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    original->setDisplayLevel(0);
    Quill::releaseAndWait();

    QCOMPARE(file->image(), processedImage);
    QCOMPARE(original->image(), image);

    file->remove();
    QVERIFY(!file->exists());
    QVERIFY(!original->exists());

    delete file;
    delete original;
}

void ut_file::testFileLimit()
{
    QTemporaryFile testFile;
    testFile.open();

    QTemporaryFile testFile2;
    testFile2.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");
    image.save(testFile2.fileName(), "png");

    QuillFile *file = new QuillFile(testFile.fileName(), "png");
    QuillFile *file2 = new QuillFile(testFile2.fileName(), "png");

    QVERIFY(file->setDisplayLevel(0));
    QVERIFY(!file2->setDisplayLevel(0));

    Quill::setFileLimit(0, 2);
    QVERIFY(file2->setDisplayLevel(0));

    Quill::releaseAndWait();
    Quill::releaseAndWait();

    delete file;
    delete file2;
}

void ut_file::testMultipleAccess()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("org.maemo.composite.brightness.contrast");
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImage imageAfter = filter->apply(image);

    QuillFile *file = new QuillFile(testFile.fileName(), "png");
    QuillFile *file2 = new QuillFile(testFile.fileName(), "png");

    QVERIFY(file != file2);

    QVERIFY(file->setDisplayLevel(0));
    QVERIFY(file2->setDisplayLevel(0));

    file->runFilter(filter);
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    QVERIFY(Unittests::compareImage(file->image(), imageAfter));
    QVERIFY(Unittests::compareImage(file2->image(), imageAfter));

    file2->undo();
    Quill::releaseAndWait();

    QVERIFY(Unittests::compareImage(file->image(), image));
    QVERIFY(Unittests::compareImage(file2->image(), image));

    delete file2;

    file->redo();
    Quill::releaseAndWait();
    QVERIFY(Unittests::compareImage(file->image(), imageAfter));

    delete file;
}

void ut_file::testDifferentPreviewLevels()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("org.maemo.composite.brightness.contrast");
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImage imageAfter = filter->apply(image);

    Quill::setPreviewSize(0, QSize(4, 1));

    QuillFile *file = new QuillFile(testFile.fileName(), "png");
    QuillFile *file2 = new QuillFile(testFile.fileName(), "png");

    QSignalSpy spy(file, SIGNAL(imageAvailable(const QuillImageList)));
    QSignalSpy spy2(file2, SIGNAL(imageAvailable(const QuillImageList)));

    QVERIFY(file != file2);

    QVERIFY(file2->setDisplayLevel(1));
    QVERIFY(file->setDisplayLevel(0));

    file->runFilter(filter);
    Quill::releaseAndWait(); // load level 0
    Quill::releaseAndWait(); // brightness level 0

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy2.count(), 1);

    QCOMPARE(file->image().size(), QSize(4, 1));
    QCOMPARE(file2->image().size(), QSize(4, 1));

    Quill::releaseAndWait(); // load level 1
    Quill::releaseAndWait(); // brightness level 1

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy2.count(), 2);

    QCOMPARE(file->image().size(), QSize(4, 1));
    QVERIFY(Unittests::compareImage(file2->image(), imageAfter));

    delete file2;

    // Ensure that the display level is kept even if the other image reference
    // is removed.
    QCOMPARE(file->image().size(), QSize(4, 1));

    delete file;
}

void ut_file::testSaveAfterDelete()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("org.maemo.composite.brightness.contrast");
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImage imageAfter = filter->apply(image);

    QuillFile *file = new QuillFile(testFile.fileName(), "png");

    file->runFilter(filter);

    QVERIFY(!Quill::isSaveInProgress());
    QVERIFY(!file->isSaveInProgress());

    file->save();

    QVERIFY(Quill::isSaveInProgress());
    QVERIFY(file->isSaveInProgress());

    delete file;

    QVERIFY(Quill::isSaveInProgress());

    Quill::releaseAndWait(); // load
    QVERIFY(Quill::isSaveInProgress());

    Quill::releaseAndWait(); // b/c
    QVERIFY(Quill::isSaveInProgress());

    Quill::releaseAndWait(); // save
    QVERIFY(!Quill::isSaveInProgress());

    Unittests::compareImage(QImage(testFile.fileName()), imageAfter);
}

void ut_file::testReadOnly()
{
    if (Unittests::isRoot()) {
        qDebug() << "Running as root, disabling file permissions test!";
        return;
    }

    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QFile qFile(testFile.fileName());
    qFile.setPermissions(QFile::ReadOwner);

    QuillFile *file = new QuillFile(testFile.fileName(), "png");
    QVERIFY(file->isReadOnly());
    delete file;
}


void ut_file::testActivateDBusThumbnailer()
{
    QuillFile *file = new QuillFile("/usr/share/libquill-tests/video/Alvin_2.mp4","video/mp4");
    Core::instance()->suggestNewTask();
    delete file;
    }

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_file test;
    return QTest::qExec( &test, argc, argv );
}
