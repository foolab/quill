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
#include <QuillImageFilter>
#include <QuillImageFilterFactory>

#include <Quill>
#include <QuillFile>
#include "ut_file.h"
#include "unittests.h"

ut_file::ut_file()
{
}

void ut_file::initTestCase()
{
    QuillImageFilter::registerAll();
}

void ut_file::cleanupTestCase()
{
}

void ut_file::testRemove()
{
    QTemporaryFile testFile;
    testFile.open();
    Unittests::generatePaletteImage().save(testFile.fileName(), "png");

    Quill *quill = new Quill(QSize(4, 1), Quill::ThreadingTest);
    QVERIFY(quill);
    QuillFile *file = quill->file(testFile.fileName(), "png");
    QVERIFY(file);
    QVERIFY(file->exists());

    QString originalFileName = file->originalFileName();

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    file->runFilter(filter);
    file->save();
    quill->releaseAndWait(); // for runFilter()

    file->remove();

    QVERIFY(!file->exists());
    QVERIFY(!QFile::exists(testFile.fileName()));
    QVERIFY(!QFile::exists(originalFileName));

    quill->releaseAndWait(); // for save()

    QVERIFY(!file->exists());
    QVERIFY(!QFile::exists(testFile.fileName()));
    QVERIFY(!QFile::exists(originalFileName));

    delete quill;
}

void ut_file::testOriginal()
{
    QTemporaryFile testFile;
    testFile.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    Quill *quill = new Quill(QSize(8, 2), Quill::ThreadingTest);
    QVERIFY(quill);
    QuillFile *file = quill->file(testFile.fileName(), "png");

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("BrightnessContrast");
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
    quill->releaseAndWait();
    quill->releaseAndWait();

    original->setDisplayLevel(0);
    quill->releaseAndWait();

    QCOMPARE(file->image(), processedImage);
    QCOMPARE(original->image(), image);

    file->remove();
    QVERIFY(!file->exists());
    QVERIFY(!original->exists());

    delete quill;
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_file test;
    return QTest::qExec( &test, argc, argv );
}
