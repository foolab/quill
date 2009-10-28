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
#include <QSignalSpy>

#include "unittests.h"
#include "ut_quill.h"
#include "quillfile.h"
#include "quillundocommand.h"
#include "quillundostack.h"
ut_quill::ut_quill()
{
}

Q_DECLARE_METATYPE(QuillImage);

void ut_quill::initTestCase()
{
    QuillImageFilter::registerAll();
    QDir().mkpath("/tmp/quill/history");
}

void ut_quill::cleanupTestCase()
{
}

void ut_quill::testQuill()
{
    Quill *quill = new Quill(QSize(4, 1), Quill::ThreadingTest);

    QCOMPARE(quill->previewLevelCount(), 1);
    QCOMPARE(quill->previewSize(0), QSize(4, 1));

    delete quill;
}

void ut_quill::testQuillFile()
{
    QTemporaryFile testFile;
    testFile.open();
    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill *quill = new Quill(QSize(4, 1), Quill::ThreadingTest);
    quill->setEditHistoryCacheSize(0, 5);
    quill->setEditHistoryCacheSize(1, 5);

    QuillFile *file = quill->file(testFile.fileName());

    QVERIFY(file);

    QCOMPARE(file->fileName(),
             testFile.fileName());

    QCOMPARE(file->displayLevel(), -1);
    file->setDisplayLevel(1);

    quill->releaseAndWait(); // preview

    QImage initialPreview = file->image();

    QCOMPARE(initialPreview.size(), QSize(4, 1));

    QCOMPARE(file->fullImageSize(), QSize(8, 2));

    QVERIFY(!file->canUndo());
    QVERIFY(!file->canRedo());

    QCOMPARE(file->allImageLevels().count(), 1);
    QVERIFY(file->image(1).isNull());

    // let the quill construct a full-size image for us

    quill->releaseAndWait();

    QImage initialFull = file->image();

    QCOMPARE(initialFull.size(), QSize(8, 2));
    QCOMPARE(initialFull, Unittests::generatePaletteImage());

    // run a brightness filter

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(16));

    file->runFilter(filter);

    quill->releaseAndWait(); // preview

    QImage laterPreview = file->image();

    QCOMPARE(laterPreview.size(), QSize(4, 1));
    QCOMPARE(laterPreview, (QImage)filter->apply(initialPreview));

    QCOMPARE(laterPreview, (QImage)file->image(0));
    QVERIFY(Unittests::compareImage(file->image(1), QImage()));

    // let the quill construct a full-size image for us

    quill->releaseAndWait();

    QImage laterFull = file->image();
    QCOMPARE(laterFull, (QImage)file->image(1));

    QCOMPARE(laterFull.size(), QSize(8, 2));
    QCOMPARE(laterFull, (QImage)filter->apply(initialFull));

    // undo

    QVERIFY(file->canUndo());
    QVERIFY(!file->canRedo());

    file->undo();

    QCOMPARE(initialPreview, (QImage)file->image(0));
    QCOMPARE(initialFull, (QImage)file->image());

    // redo

    QVERIFY(file->canRedo());
    QVERIFY(!file->canUndo());

    file->redo();

    QCOMPARE(laterPreview, (QImage)file->image(0));
    QCOMPARE(laterFull, (QImage)file->image());

    QList<QuillImage> targetList;
    targetList.append(file->image(0));
    targetList.append(file->image());

    QCOMPARE(targetList, file->allImageLevels());

    delete quill;
}

// Test cache disabling, 2 preview levels + 1 full level

void ut_quill::testDisableCache()
{
    QTemporaryFile testFile;
    testFile.open();
    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill *quill = new Quill(QSize(2, 1), Quill::ThreadingTest);

    quill->setPreviewLevelCount(2);
    QCOMPARE(quill->previewLevelCount(), 2);

    quill->setPreviewSize(1, QSize(4, 1));

    QuillFile *file = quill->file(testFile.fileName());

    file->setDisplayLevel(2);

    // first level preview

    quill->releaseAndWait();

    QuillImage initialPreview = file->image();

    QCOMPARE(initialPreview.size(), QSize(2, 1));

    // mid level preview

    quill->releaseAndWait();

    QuillImage initialMid = file->image();

    QCOMPARE(initialMid.size(), QSize(4, 1));

    // full-size image

    quill->releaseAndWait();

    QuillImage initialFull = file->image();

    QCOMPARE(initialFull.size(), QSize(8, 2));
    QCOMPARE(initialFull, QuillImage(Unittests::generatePaletteImage()));

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(16));

    file->runFilter(filter);

    // preview level

    quill->releaseAndWait();

    QuillImage laterPreview = file->image();
    QVERIFY(Unittests::compareImage(laterPreview,
                                    filter->apply(initialPreview)));
    QVERIFY(Unittests::compareImage(file->image(0), laterPreview));
    QVERIFY(file->image(1).isNull());
    QVERIFY(file->image(2).isNull());

    // mid level

    quill->releaseAndWait();

    QuillImage laterMid = file->image();
    QVERIFY(Unittests::compareImage(laterMid,
                                    filter->apply(initialMid)));
    QVERIFY(Unittests::compareImage(file->image(0), laterPreview));
    QVERIFY(Unittests::compareImage(file->image(1), laterMid));
    QVERIFY(file->image(2).isNull());

    // high level

    quill->releaseAndWait();

    QuillImage laterFull = file->image();
    QVERIFY(Unittests::compareImage(laterFull,
                                    filter->apply(initialFull)));
    QVERIFY(Unittests::compareImage(file->image(0), laterPreview));
    QVERIFY(Unittests::compareImage(file->image(1), laterMid));
    QVERIFY(Unittests::compareImage(file->image(2), laterFull));

    // undo - preview is regenerated

    file->undo();

    quill->releaseAndWait();

    QVERIFY(Unittests::compareImage(file->image(0), initialPreview));
    QVERIFY(file->image(1).isNull());
    QVERIFY(file->image(2).isNull());

    // mid level regenerated

    quill->releaseAndWait();

    QCOMPARE(initialPreview, file->image(0));
    QCOMPARE(initialMid, file->image(1));
    QCOMPARE(file->image(2), QuillImage());

    // high level regenerated

    quill->releaseAndWait();

    QCOMPARE(initialPreview, file->image(0));
    QCOMPARE(initialMid, file->image(1));
    QCOMPARE(initialFull, file->image(2));

    delete quill;
}

void ut_quill::testSignals()
{
    QTemporaryFile testFile;
    testFile.open();
    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill *quill = new Quill(QSize(2, 1), Quill::ThreadingTest);
    quill->setPreviewLevelCount(2);

    QuillFile *file = quill->file(testFile.fileName());

    QSignalSpy spy(file, SIGNAL(imageAvailable(const QuillImageList)));

    QuillImage image =
        QImage(testFile.fileName());
    QuillImage midImage = image.scaled(QSize(4, 1),
                                       Qt::IgnoreAspectRatio,
                                       Qt::SmoothTransformation);
    QuillImage smallImage = image.scaled(QSize(2, 1),
                                         Qt::IgnoreAspectRatio,
                                         Qt::SmoothTransformation);

    file->setDisplayLevel(2);
    quill->releaseAndWait(); // preview 0

    QCOMPARE(spy.count(), 1);
    QuillImage spyImage = spy.last().first().value<QuillImageList>().first();

    QVERIFY(Unittests::compareImage(spyImage, smallImage));
    QCOMPARE(spyImage.z(), 0);

    quill->releaseAndWait(); // preview 1

    QCOMPARE(spy.count(), 2);
    spyImage = spy.last().first().value<QuillImageList>().first();

    QVERIFY(Unittests::compareImage(spyImage, midImage));
    QCOMPARE(spyImage.z(), 1);

    quill->releaseAndWait(); // full image

    QCOMPARE(spy.count(), 3);
    spyImage = spy.last().first().value<QuillImageList>().first();
    QVERIFY(Unittests::compareImage(spyImage, image));
    QCOMPARE(spyImage.z(), 2);

    // run brightness + contrast filters

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    QuillImageFilter *filter2 =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QVERIFY(filter2);
    filter2->setOption(QuillImageFilter::Contrast, QVariant(25));

    file->runFilter(filter);
    file->runFilter(filter2);

    quill->releaseAndWait();
    quill->releaseAndWait();
    quill->releaseAndWait(); // preview 0

    QCOMPARE(spy.count(), 4);
    spyImage = spy.last().first().value<QuillImageList>().first();

    QVERIFY(Unittests::compareImage(spyImage,
                                    filter2->apply(filter->apply(smallImage))));
    QCOMPARE(spyImage.z(), 0);

    quill->releaseAndWait();
    quill->releaseAndWait(); // preview 1

    QCOMPARE(spy.count(), 5);
    spyImage = spy.last().first().value<QuillImageList>().first();

    QVERIFY(Unittests::compareImage(spyImage,
                                    filter2->apply(filter->apply(midImage))));
    QCOMPARE(spyImage.z(), 1);

    quill->releaseAndWait(); // full image

    QCOMPARE(spy.count(), 6);
    spyImage = spy.last().first().value<QuillImageList>().first();
    QVERIFY(Unittests::compareImage(spyImage,
                                    filter2->apply(filter->apply(image))));
    QCOMPARE(spyImage.z(), 2);

    delete quill;
}

void ut_quill::testLoadSave()
{
    QTemporaryFile testFile;
    testFile.open();

    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill *quill = new Quill(QSize(4, 1), Quill::ThreadingTest);
    quill->setEditHistoryDirectory("/tmp/quill/history");

    QuillImage image(QImage(testFile.fileName()));
    QCOMPARE(image, QuillImage(Unittests::generatePaletteImage()));

    QuillFile *file = quill->file(testFile.fileName(), "png");
    QSignalSpy spy(file, SIGNAL(saved()));

    file->setDisplayLevel(1);

    quill->releaseAndWait();
    quill->releaseAndWait();
    QCOMPARE(file->image(), image);

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImage targetImage = filter->apply(image);

    // construct a copy of the filter, since original filter will be lost

    QuillImageFilter *backupFilter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    QVERIFY(backupFilter);
    backupFilter->setOption(QuillImageFilter::Brightness, QVariant(20));

    file->runFilter(filter);
    quill->releaseAndWait();
    quill->releaseAndWait();

    file->save();
    quill->releaseAndWait();

    QCOMPARE(spy.count(), 1);

    QImage loadedImage(testFile.fileName(), "png");
    QImage origImage(file->originalFileName());

    QVERIFY(Unittests::compareImage(origImage, image));
    QVERIFY(Unittests::compareImage(loadedImage, targetImage));

    Quill *quill2 = new Quill(QSize(4, 1), Quill::ThreadingTest);
    quill2->setEditHistoryDirectory("/tmp/quill/history");
    quill2->setEditHistoryCacheSize(0, 5);

    QuillFile *file2 = quill2->file(testFile.fileName(), "png");

    file2->setDisplayLevel(1);

    quill2->releaseAndWait();
    quill2->releaseAndWait();

    QVERIFY(Unittests::compareImage(file2->image(), loadedImage));

    // Check that edit history is preserved

    file2->undo();
    quill2->releaseAndWait();
    quill2->releaseAndWait();

    QVERIFY(Unittests::compareImage(file2->image(), image));

    // Redo and check repeated loading

    file2->redo();
    quill2->releaseAndWait();

    QVERIFY(Unittests::compareImage(file2->image(), loadedImage));

    delete quill;
    delete quill2;
    delete backupFilter;
}

void ut_quill::testMultiSave()
{
    QTemporaryFile testFile;
    testFile.open();

    QTemporaryFile originFile;
    originFile.open();

    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill *quill = new Quill(QSize(4, 1), Quill::ThreadingTest);
    quill->setEditHistoryDirectory("/tmp/quill/history");

    QImage image(testFile.fileName());
    QVERIFY(Unittests::compareImage(image, Unittests::generatePaletteImage()));

    QuillFile *file = quill->file(testFile.fileName(), "png");
    QSignalSpy spy(file, SIGNAL(saved()));

    file->setDisplayLevel(1);

    quill->releaseAndWait();
    quill->releaseAndWait();
    QCOMPARE((QImage)file->image(), image);

    QuillImageFilter *brightnessFilter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    QVERIFY(brightnessFilter);
    brightnessFilter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImageFilter *contrastFilter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    QVERIFY(contrastFilter);
    contrastFilter->setOption(QuillImageFilter::Contrast, QVariant(20));

    file->runFilter(brightnessFilter);
    quill->releaseAndWait();
    quill->releaseAndWait();

    QImage imageBetween = file->image();

    file->runFilter(contrastFilter);
    quill->releaseAndWait();
    quill->releaseAndWait();

    QImage imageAfter = file->image();

    file->save();
    quill->releaseAndWait();

    QCOMPARE(spy.count(), 1);

    Quill *quill2 = new Quill(QSize(4, 1), Quill::ThreadingTest);
    quill2->setEditHistoryDirectory("/tmp/quill/history");

    QuillFile *file2 = quill2->file(testFile.fileName(), "png");
    QSignalSpy spy2(file2, SIGNAL(saved()));
    file2->setDisplayLevel(1);

    quill2->releaseAndWait();
    quill2->releaseAndWait();

    QVERIFY(Unittests::compareImage(file2->image(), imageAfter));

    file2->undo();
    quill2->releaseAndWait();
    quill2->releaseAndWait();
    quill2->releaseAndWait();
    quill2->releaseAndWait();

    QVERIFY(Unittests::compareImage(file2->image(), imageBetween));

    file2->save();
    quill2->releaseAndWait();

    QCOMPARE(spy2.count(), 1);

    Quill *quill3 = new Quill(QSize(4, 1), Quill::ThreadingTest);
    quill3->setEditHistoryDirectory("/tmp/quill/history");

    QuillFile *file3 = quill3->file(testFile.fileName(), "png");
    file3->setDisplayLevel(1);

    quill3->releaseAndWait();
    quill3->releaseAndWait();

    QVERIFY(Unittests::compareImage(file3->image(), imageBetween));

    delete quill;
    delete quill2;
    delete quill3;
}

// Test situations where nothing should be saved!
void ut_quill::testNoSave()
{
    QTemporaryFile testFile;
    testFile.open();

    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill *quill = new Quill(QSize(4, 1), Quill::ThreadingTest);
    quill->setEditHistoryDirectory("/tmp/quill/history");

    QImage image(testFile.fileName());
    QVERIFY(Unittests::compareImage(image, Unittests::generatePaletteImage()));

    QuillFile *file = quill->file(testFile.fileName(), "png");
    QSignalSpy spy(file, SIGNAL(saved()));

    file->setDisplayLevel(1);

    quill->releaseAndWait();
    quill->releaseAndWait();

    file->save();

    QVERIFY(Unittests::compareImage(QImage(testFile.fileName(), "png"), image));

    QCOMPARE(spy.count(), 0);

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillFile *file2 = quill->file(testFile.fileName(), "png");
    QCOMPARE(file2, file);

    file->runFilter(filter);
    quill->releaseAndWait();
    quill->releaseAndWait();

    QImage imageAfter = file->image();
    QVERIFY(Unittests::compareImage(imageAfter, filter->apply(image)));

    file->save();
    quill->releaseAndWait();

    // now, as there are changes, something should happen

    QCOMPARE(spy.count(), 1);

    Quill *quill2 = new Quill(QSize(4, 1), Quill::ThreadingTest);
    quill2->setEditHistoryDirectory("/tmp/quill/history");

    QuillFile *file3 = quill2->file(testFile.fileName(), "png");

    QSignalSpy spy2(file3, SIGNAL(saved()));

    file3->setDisplayLevel(1);

    quill2->releaseAndWait();
    quill2->releaseAndWait();

    file3->save();

    // Also now, no changes should happen

    QVERIFY(Unittests::compareImage(QImage(testFile.fileName(), "png"), imageAfter));

    QCOMPARE(spy2.count(), 0);

    delete quill;
    delete quill2;
}

// Test that the save index is properly nulled whenever changes are made
void ut_quill::testSaveIndex()
{
    QTemporaryFile testFile;
    testFile.open();

    QTemporaryFile originFile;
    originFile.open();

    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill *quill = new Quill(QSize(4, 1), Quill::ThreadingTest);
    quill->setEditHistoryDirectory("/tmp/quill/history");

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImageFilter *filter2 =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    filter->setOption(QuillImageFilter::Contrast, QVariant(20));

    QuillImageFilter *filter3 =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    filter->setOption(QuillImageFilter::Contrast, QVariant(30));

    QImage image(testFile.fileName());
    QVERIFY(Unittests::compareImage(image, Unittests::generatePaletteImage()));

    QuillFile *file = quill->file(testFile.fileName(), "png");
    QSignalSpy spy(file, SIGNAL(saved()));
    file->setDisplayLevel(1);

    quill->releaseAndWait();
    quill->releaseAndWait();

    file->runFilter(filter);
    quill->releaseAndWait();
    quill->releaseAndWait();

    file->save();
    quill->releaseAndWait();

    QCOMPARE(spy.count(), 1);

    Quill *quill2 = new Quill(QSize(4, 1), Quill::ThreadingTest);
    quill2->setEditHistoryDirectory("/tmp/quill/history");

    QuillFile *file2 = quill2->file(testFile.fileName(), "png");
    QSignalSpy spy2(file2, SIGNAL(saved()));
    file2->setDisplayLevel(1);

    file2->undo();
    quill2->releaseAndWait();
    quill2->releaseAndWait();
    quill2->releaseAndWait();
    quill2->releaseAndWait();

    file2->runFilter(filter2);
    file2->runFilter(filter3);

    quill2->releaseAndWait();
    quill2->releaseAndWait();
    quill2->releaseAndWait();
    quill2->releaseAndWait();

    file2->save();
    quill2->releaseAndWait();

    // Now, even if the save index is exactly the same as before,
    // we should see changes happen

    QCOMPARE(spy2.count(), 1);

    delete quill;
    delete quill2;
}

// Test that background priority works for saving
void ut_quill::testBackgroundPriority()
{
    QTemporaryFile testFile;
    testFile.open();
    QTemporaryFile originFile;
    originFile.open();
    QTemporaryFile testFile2;
    testFile2.open();
    QTemporaryFile originFile2;
    originFile2.open();

    QImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");
    image.save(testFile2.fileName(), "png");

    Quill *quill = new Quill(QSize(2, 1), Quill::ThreadingTest);
    quill->setEditHistoryDirectory("/tmp/quill/history");
    quill->setPreviewLevelCount(2);

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillFile *file = quill->file(testFile.fileName(), "png");
    file->setOriginalFileName(originFile.fileName());

    QSignalSpy changedSpy(file, SIGNAL(imageAvailable(const QuillImageList)));
    QSignalSpy spy(file, SIGNAL(saved()));
    file->setDisplayLevel(2);

    quill->releaseAndWait();
    quill->releaseAndWait();
    quill->releaseAndWait();

    file->runFilter(filter);
    file->save();

    file->setDisplayLevel(-1);

    QuillFile *file2 = quill->file(testFile2.fileName(), "png");
    file2->setOriginalFileName(originFile.fileName());

    QSignalSpy changedSpy2(file2, SIGNAL(imageAvailable(const QuillImageList)));
    QSignalSpy spy2(file2, SIGNAL(saved()));
    file2->setDisplayLevel(2);

    QCOMPARE(changedSpy.count(), 3);
    QCOMPARE(changedSpy2.count(), 0);

    // Filter run, preview 0 for closed file (already started)
    quill->releaseAndWait();

    QCOMPARE(changedSpy.count(), 3);
    QCOMPARE(changedSpy2.count(), 0);

    // Open file, preview 0 for new file
    quill->releaseAndWait();

    QCOMPARE(changedSpy.count(), 3);
    QCOMPARE(changedSpy2.count(), 1);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(spy2.count(), 0);

    // Open file, preview 1 for new file
    quill->releaseAndWait();

    QCOMPARE(changedSpy.count(), 3);
    QCOMPARE(changedSpy2.count(), 2);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(spy2.count(), 0);

    // Filter run, full image for closed file
    quill->releaseAndWait();

    QCOMPARE(file->image().size(), QSize(8, 2));

    QCOMPARE(changedSpy.count(), 3);
    QCOMPARE(changedSpy2.count(), 2);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(spy2.count(), 0);

    // Save file, closed file
    quill->releaseAndWait();

    QCOMPARE(changedSpy.count(), 3);
    QCOMPARE(changedSpy2.count(), 2);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy2.count(), 0);

    QFile realFile(originFile.fileName());
    QVERIFY(realFile.size() > 0);

    // Open file, full image for new file

    quill->releaseAndWait();

    QCOMPARE(changedSpy.count(), 3);
    QCOMPARE(changedSpy2.count(), 3);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy2.count(), 0);

    delete quill;
}

// Test what happens when we reopen a file in progress of saving
void ut_quill::testReOpen()
{
    /*    QTemporaryFile testFile;
    testFile.open();
    QTemporaryFile originFile;
    originFile.open();
    QTemporaryFile testFile2;
    testFile2.open();
    QTemporaryFile originFile2;
    originFile2.open();

    QImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");
    image.save(testFile2.fileName(), "png");

    Quill *quill = new Quill(QSize(2, 1), Quill::ThreadingTest);

    QSignalSpy changedSpy(quill, SIGNAL(imageChanged(int, const QImage)));
    QSignalSpy spy(quill, SIGNAL(saved(const QString, const QByteArray)));

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    quill->openFile(testFile.fileName(),
                    originFile.fileName(),
                    "",
                    "png");

    quill->releaseAndWait();
    quill->releaseAndWait();

    file->runFilter(filter);
    quill->releaseAndWait();
    quill->releaseAndWait();

    quill->closeAndSave();

    quill->openFile(testFile2.fileName(),
                    originFile2.fileName(),
                    "",
                    "png");

    // This should just remove the second stack.
    quill->closeAndSave();

    quill->openFile(testFile.fileName(),
                    originFile.fileName(),
                    "",
                    "png");

    // Let the save operation finish on the background.
    quill->releaseAndWait();

    // We lose the first stack by re-opening it (current implementation).
    quill->releaseAndWait();
    quill->releaseAndWait();

    // Even the BrightnessContrast should be repeated now
    // (current implementation).
    quill->releaseAndWait();
    quill->releaseAndWait();

    QCOMPARE(spy.count(), 0);

    QFile file(originFile.fileName());
    QCOMPARE((int)file.size(), 0);

    // Now, saving should finally result.
    quill->closeAndSave();
    quill->releaseAndWait();

    QCOMPARE(spy.count(), 1);

    QFile file2(originFile.fileName());
    QVERIFY(file2.size() > 0);

    delete quill;*/
}

// Ensure that the small picture in saved edit history case is coming
// from the correct version (up-to-date, instead of the original, if
// available).

void ut_quill::testLoadSaveSmallPicture()
{
    QTemporaryFile testFile;
    testFile.open();
    QTemporaryFile originFile;
    originFile.open();

    QImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    Quill *quill = new Quill(QSize(8, 2), Quill::ThreadingTest);
    quill->setEditHistoryDirectory("/tmp/quill/history");

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImageFilter *filter2 =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    filter->setOption(QuillImageFilter::Contrast, QVariant(20));

    QuillFile *file = quill->file(testFile.fileName(), "png");
    file->setOriginalFileName(originFile.fileName());
    QSignalSpy spy(file, SIGNAL(saved()));
    file->setDisplayLevel(1);

    quill->releaseAndWait();
    quill->releaseAndWait();

    file->runFilter(filter);
    quill->releaseAndWait();
    quill->releaseAndWait();

    file->runFilter(filter2);
    quill->releaseAndWait();
    quill->releaseAndWait();

    QImage imageAfter = file->image();

    file->save();
    quill->releaseAndWait();

    QCOMPARE(spy.count(), 1);

    // Now, thrash the original image (full black)

    QImage blackImage(QSize(8, 2), QImage::Format_RGB32);
    blackImage.fill(qRgb(0, 0, 0));
    blackImage.save(originFile.fileName(), "png");

    QImage filteredBlackImage(QSize(8, 2), QImage::Format_RGB32);
    filteredBlackImage.fill(qRgb(36, 36, 36));

    Quill *quill2 = new Quill(QSize(8, 2), Quill::ThreadingTest);
    quill2->setEditHistoryDirectory("/tmp/quill/history");

    QuillFile *file2 =
        quill2->file(testFile.fileName(), "png");
    file2->setDisplayLevel(1);

    // Verify the small picture (and the big picture) that they still
    // date back to the current version and not the original one.

    quill2->releaseAndWait();
    QVERIFY(Unittests::compareImage(file2->image(), imageAfter));

    quill2->releaseAndWait();
    QVERIFY(Unittests::compareImage(file2->image(), imageAfter));

    // Now, go back one step and see that everything dates back to the
    // original.
    file2->undo();

    quill2->releaseAndWait();
    quill2->releaseAndWait();
    QVERIFY(Unittests::compareImage(file2->image(), filteredBlackImage));

    quill2->releaseAndWait();
    quill2->releaseAndWait();
    QVERIFY(Unittests::compareImage(file2->image(), filteredBlackImage));

    delete quill;
    delete quill2;
}

// Test that export works, both from an image with a history and from
// one without it.

void ut_quill::testExport()
{
    /*    QTemporaryFile testFile;
    testFile.open();
    QTemporaryFile originFile;
    originFile.open();
    QTemporaryFile cloneFile1;
    cloneFile1.open();
    QTemporaryFile cloneFile2;
    cloneFile2.open();
    QTemporaryFile cloneFile3;
    cloneFile3.open();
    QTemporaryFile cloneFile4;
    cloneFile4.open();

    QImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    Quill *quill = new Quill(QSize(8, 2), Quill::ThreadingTest);

    QSignalSpy spy(quill, SIGNAL(saved(const QString, const QByteArray)));

    // Case: no original file, no changes in history

    quill->openFile(testFile.fileName(),
                    originFile.fileName(),
                    "",
                    "png");

    quill->releaseAndWait();
    quill->releaseAndWait();

    quill->exportFile(cloneFile1.fileName(), "png");
    // load
    quill->releaseAndWait();
    // save
    quill->releaseAndWait();

    QVERIFY(Unittests::compareImage(QImage(cloneFile1.fileName()), image));

    // regenerative load
    quill->releaseAndWait();

    // Case: no original file, changes in history

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    file->runFilter(filter);
    quill->releaseAndWait();
    quill->releaseAndWait();

    QImage filteredImage = file->image();

    quill->exportFile(cloneFile2.fileName(), "png");
    // load
    quill->releaseAndWait();
    // operation
    quill->releaseAndWait();
    // save
    quill->releaseAndWait();

    QVERIFY(Unittests::compareImage(QImage(cloneFile2.fileName()), filteredImage));

    // regenerative load
    quill->releaseAndWait();

    // regenerative operation
    quill->releaseAndWait();

    // Case: original file exists, no changes in history

    quill->closeAndSave();
    quill->releaseAndWait();

    QCOMPARE(spy.count(), 3);

    quill->openFile(testFile.fileName(),
                    originFile.fileName(),
                    editHistory,
                    "png");

    quill->releaseAndWait();
    quill->releaseAndWait();

    quill->exportFile(cloneFile3.fileName(), "png");
    // load
    quill->releaseAndWait();
    // operation
    quill->releaseAndWait();
    // save
    quill->releaseAndWait();

    QVERIFY(Unittests::compareImage(QImage(cloneFile3.fileName()), filteredImage));

    // regenerative load
    quill->releaseAndWait();

    // Case: original file exists, changes in history

    QuillImageFilter *filter2 =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    filter2->setOption(QuillImageFilter::Contrast, QVariant(20));

    file->runFilter(filter2);
    quill->releaseAndWait();
    quill->releaseAndWait();

    QImage finalImage = file->image();

    quill->exportFile(cloneFile4.fileName(), "png");
    // load
    quill->releaseAndWait();
    // operation
    quill->releaseAndWait();
    // operation
    quill->releaseAndWait();
    // save
    quill->releaseAndWait();

    QVERIFY(Unittests::compareImage(QImage(cloneFile4.fileName()), finalImage));

    // regenerative loads + ops
    quill->releaseAndWait();
    quill->releaseAndWait();
    quill->releaseAndWait();
    quill->releaseAndWait();

    delete quill;*/
}

// Test that export works with redo history

void ut_quill::testExportWithRedoHistory()
{
    /*    QTemporaryFile testFile;
    testFile.open();
    QTemporaryFile originFile;
    originFile.open();
    QTemporaryFile cloneFile;
    cloneFile.open();

    QImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    Quill *quill = new Quill(QSize(8, 2), Quill::ThreadingTest);

    // Case: no original file, no changes in history

    quill->openFile(testFile.fileName(),
                    originFile.fileName(),
                    "",
                    "png");

    quill->releaseAndWait();
    quill->releaseAndWait();

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillImageFilter *rotate =
        QuillImageFilterFactory::createImageFilter("Rotate");
    filter->setOption(QuillImageFilter::Angle, QVariant(90));

    file->runFilter(filter);
    quill->releaseAndWait();
    quill->releaseAndWait();

    file->runFilter(rotate);
    quill->releaseAndWait(); // rotate
    quill->releaseAndWait();
    quill->releaseAndWait(); // preview re-gen

    QVERIFY(quill->canUndo());

    file->undo();
    quill->releaseAndWait(); // load
    quill->releaseAndWait(); // brightness

    quill->exportFile(cloneFile.fileName(), "png");
    quill->releaseAndWait(); // load
    quill->releaseAndWait(); // brightness
    quill->releaseAndWait(); // save

    QVERIFY(Unittests::compareImage(QImage(cloneFile.fileName()), filter->apply(image)));

    quill->releaseAndWait(); // regenerative load
    quill->releaseAndWait(); // regenerative brightness

    delete quill;*/
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_quill test;
    return QTest::qExec( &test, argc, argv );

}
