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

#include <Quill>
#include <QuillFile>
#include "ut_scheduler.h"
#include "unittests.h"

ut_scheduler::ut_scheduler()
{
}

void ut_scheduler::initTestCase()
{
    QDir().mkpath("/tmp/quill/thumbnails/normal");
    image = Unittests::generatePaletteImage();
    // Smooth transformation is the default by the PNG loader
    thumbnailImage = image.scaled(QSize(4, 1), Qt::IgnoreAspectRatio,
                                  Qt::SmoothTransformation);
    fileName1 = "/tmp/test.png";
    fileName2 = "/tmp/test2.png";
    fileName3 = "/tmp/test3.png";
    image.save(fileName1);
    image.save(fileName2);
    image.save(fileName3);
}

void ut_scheduler::cleanupTestCase()
{
}

void ut_scheduler::init()
{
    Quill::initTestingMode();
    Quill::setPreviewSize(0, QSize(4, 1));
    Quill::setThumbnailBasePath("/tmp/quill/thumbnails");
    Quill::setThumbnailFlavorName(0, "normal");
    Quill::setThumbnailExtension("png");

    file1 = new QuillFile(fileName1);
    file2 = new QuillFile(fileName2);
    file3 = new QuillFile(fileName3);

    thumbName1 = file1->thumbnailFileName(0);
    thumbName2 = file2->thumbnailFileName(0);
    thumbName3 = file3->thumbnailFileName(0);
    QFile::remove(thumbName1);
    QFile::remove(thumbName2);
    QFile::remove(thumbName3);
}

void ut_scheduler::cleanup()
{
    Quill::cleanup();
    delete file1;
    delete file2;
    delete file3;
}

void ut_scheduler::testPrioritySetting()
{
    QCOMPARE(file1->priority(), (int)QuillFile::Priority_Normal);

    file1->setPriority(QuillFile::Priority_Low);
    QCOMPARE(file1->priority(), (int)QuillFile::Priority_Low);

    file1->setPriority(QuillFile::Priority_High);
    QCOMPARE(file1->priority(), (int)QuillFile::Priority_High);
}

void ut_scheduler::testThumbnailLoadingPriority()
{
    Quill::setThumbnailCreationEnabled(false);

    // Test that a thumbnail is loaded first

    thumbnailImage.save(thumbName3);

    file1->setDisplayLevel(0);
    file2->setDisplayLevel(0);
    file3->setDisplayLevel(0);

    // File 1 is loaded first since it was started first

    Quill::releaseAndWait();
    QVERIFY(Unittests::compareImage(file1->image(), thumbnailImage));

    // File 3 is loaded next since it has a thumbnail

    Quill::releaseAndWait();
    Quill::releaseAndWait();
    QVERIFY(Unittests::compareImage(file3->image(), thumbnailImage));

    // File 2 is loaded last

    Quill::releaseAndWait();
    QVERIFY(Unittests::compareImage(file2->image(), thumbnailImage));
}

void ut_scheduler::testExplicitPriority()
{
    Quill::setThumbnailCreationEnabled(false);

    file1->setPriority(QuillFile::Priority_Low);
    file2->setPriority(QuillFile::Priority_Low);

    file1->setDisplayLevel(0);
    file2->setDisplayLevel(0);
    file3->setDisplayLevel(0);

    // File 1 is loaded first since it was started first (priority does not
    // affect loads already in progress)

    Quill::releaseAndWait();
    QVERIFY(Unittests::compareImage(file1->image(), thumbnailImage));

    // File 3 is loaded next since it has priority

    Quill::releaseAndWait();
    QVERIFY(Unittests::compareImage(file3->image(), thumbnailImage));

    // File 2 is loaded last

    Quill::releaseAndWait();
    QVERIFY(Unittests::compareImage(file2->image(), thumbnailImage));
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_scheduler test;
    return QTest::qExec( &test, argc, argv );
}
