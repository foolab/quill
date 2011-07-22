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
#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include <Quill>

#include "quillerror.h"
#include "core.h"
#include "file.h"
#include "ut_file.h"
#include "unittests.h"
#include "quillfile.h"
#include "../../src/strings.h"

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

    QSignalSpy spy(Quill::instance(),
                   SIGNAL(removed(const QString)));

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    QVERIFY(file);
    QVERIFY(file->exists());

    QString originalFileName = file->originalFileName();

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
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

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toString(), testFile.fileName());

    delete file;
}

void ut_file::testOriginal()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
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

void ut_file::testOriginalAfterSave()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    QuillFile *original = file->original();
    // This makes us to setup the original's undo stack
    QCOMPARE(original->fullImageSize(), image.size());

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImage processedImage = filter->apply(image);

    file->runFilter(filter);

    file->save();

    Quill::releaseAndWait();
    Quill::releaseAndWait();
    Quill::releaseAndWait();

    original->setDisplayLevel(0);
    Quill::releaseAndWait();
    QVERIFY(Unittests::compareImage(original->image(), image));

    delete file;
    delete original;
}

void ut_file::testMultipleAccess()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImage imageAfter = filter->apply(image);

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    QuillFile *file2 = new QuillFile(testFile.fileName(), Strings::png);

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
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImage imageAfter = filter->apply(image);

    Quill::setPreviewSize(0, QSize(4, 1));

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    QuillFile *file2 = new QuillFile(testFile.fileName(), Strings::png);

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
    QCOMPARE(file->image().z(), 0);
    QCOMPARE(file2->image().size(), QSize(4, 1));
    QCOMPARE(file2->image().z(), 0);

    Quill::releaseAndWait(); // load level 1
    Quill::releaseAndWait(); // brightness level 1

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy2.count(), 2);

    QCOMPARE(file->image().size(), QSize(4, 1));
    QCOMPARE(file->image().z(), 0);
    QVERIFY(Unittests::compareImage(file2->image(), imageAfter));
    QCOMPARE(file2->image().z(), 1);

    delete file2;

    // Ensure that the display level is kept even if the other image reference
    // is removed.
    QCOMPARE(file->image().size(), QSize(4, 1));
    QCOMPARE(file->image().z(), 0);

    delete file;
}

void ut_file::testSaveAfterDelete()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImage imageAfter = filter->apply(image);

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);

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

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    QVERIFY(!file->supportsEditing());
    delete file;
}

void ut_file::testLastModified()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QDateTime lastModified = QFileInfo(testFile.fileName()).lastModified();

    QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
    QCOMPARE(file->lastModified(), lastModified);
    delete file;
}

void ut_file::testRevertRestore()
{
   QTemporaryFile testFile;
   testFile.open();

   QuillImage image = Unittests::generatePaletteImage();
   image.save(testFile.fileName(), "png");

   QuillImageFilter *filter =
       QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
   QVERIFY(filter);
   filter->setOption(QuillImageFilter::Brightness, QVariant(20));
   QuillImage imageAfter = filter->apply(image);

   QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
   QVERIFY(file->setDisplayLevel(0));
   QCOMPARE(file->canRestore(),false);
   QCOMPARE(file->canRevert(),false);
   file->runFilter(filter);
   Quill::releaseAndWait();
   Quill::releaseAndWait();
   QCOMPARE(file->canRestore(),false);
   QCOMPARE(file->canRevert(),true);
   QuillImageFilter *filter1 =
       QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_Rotate);

   filter1->setOption(QuillImageFilter::Angle, QVariant(-90));
   QuillImage imageAfter1 = filter1->apply(imageAfter);
   file->runFilter(filter1);
   Quill::releaseAndWait();
   Quill::releaseAndWait();
   QCOMPARE(file->canRestore(),false);
   QCOMPARE(file->canRevert(),true);

   //Test revert
   file->revert();
   Quill::releaseAndWait();
   Quill::releaseAndWait();
   QCOMPARE(file->canRestore(),true);
   QCOMPARE(file->canRevert(),false);
   QCOMPARE(file->internalFile()->m_stack->revertIndex(),3);
   QVERIFY(Unittests::compareImage(file->image(), image));
   file->save();
   Quill::releaseAndWait();
   Quill::releaseAndWait();

   //Test restore
   file->restore();
   Quill::releaseAndWait();
   Quill::releaseAndWait();
   QCOMPARE(file->canRestore(),false);
   QCOMPARE(file->canRevert(),true);
   QCOMPARE(file->internalFile()->m_stack->revertIndex(),0);
   QVERIFY(Unittests::compareImage(file->image(), imageAfter1));

   //Test redo with revert and restore
   file->revert();
   Quill::releaseAndWait();
   Quill::releaseAndWait();
   QCOMPARE(file->canRestore(),true);
   QCOMPARE(file->canRevert(),false);
   QCOMPARE(file->internalFile()->m_stack->revertIndex(),3);
   file->redo();
   QCOMPARE(file->canRestore(),false);
   QCOMPARE(file->canRevert(),true);
   QCOMPARE(file->internalFile()->m_stack->revertIndex(),0);

   //Test one additional operation after revert
   file->revert();
   Quill::releaseAndWait();
   Quill::releaseAndWait();
   QCOMPARE(file->canRestore(),true);
   QCOMPARE(file->canRevert(),false);
   QCOMPARE(file->internalFile()->m_stack->revertIndex(),2);
   QuillImageFilter *filter2 =
       QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_Rotate);

   filter2->setOption(QuillImageFilter::Angle, QVariant(-90));
   file->runFilter(filter2);
   QCOMPARE(file->canRestore(),false);
   QCOMPARE(file->canRevert(),true);
   QCOMPARE(file->internalFile()->m_stack->revertIndex(),0);

   delete file;
}

void ut_file::testDoubleRevertRestore()
{
    // Tests that revert and restore still work after a previous revert.

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
   QVERIFY(filter2);
   filter2->setOption(QuillImageFilter::Brightness, QVariant(-20));

   QuillFile *file = new QuillFile(testFile.fileName(), Strings::png);
   file->runFilter(filter);
   QVERIFY(file->canRevert());
   file->revert();
   QVERIFY(file->canRestore());
   file->runFilter(filter2);
   QVERIFY(file->canRevert());
   file->revert();
   QVERIFY(file->canRestore());

   delete file;
}

void ut_file::testEdittingHistory()
{
    QTemporaryFile testFile1;
    testFile1.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile1.fileName(), "png");

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillFile *file = new QuillFile(testFile1.fileName(), Strings::png);
    File *fileObject = file->internalFile();
    QVERIFY(file->setDisplayLevel(0));
    QVERIFY(!fileObject->hasOriginal());
    QFile editHistory(fileObject->editHistoryFileName(fileObject->fileName(),
                                          Core::instance()->editHistoryPath()));
    QVERIFY(!editHistory.exists());
    file->runFilter(filter);
    file->undo();
    QVERIFY(editHistory.exists());
    QVERIFY(!fileObject->hasOriginal());

}
int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_file test;
    return QTest::qExec( &test, argc, argv );
}
