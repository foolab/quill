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

#include <QVariant>
#include <QtTest/QtTest>

#include "metadata.h"
#include "ut_metadata.h"
#include "unittests.h"

ut_metadata::ut_metadata()
{
}

void ut_metadata::initTestCase()
{
}

void ut_metadata::cleanupTestCase()
{
}

void ut_metadata::init()
{
    metadata = new Metadata("/usr/share/libquill-tests/images/exif.jpg");
}

void ut_metadata::cleanup()
{
    delete metadata;
}

void ut_metadata::testCameraMake()
{
    QVERIFY(metadata->isValid());
    QCOMPARE(metadata->entry(Metadata::Tag_Make).toString(), QString("Quill"));
}

void ut_metadata::testCameraModel()
{
    QVERIFY(metadata->isValid());
    QCOMPARE(metadata->entry(Metadata::Tag_Model).toString(), QString("Q100125"));
}

void ut_metadata::testImageWidth()
{
    QVERIFY(metadata->isValid());
    QCOMPARE(metadata->entry(Metadata::Tag_ImageWidth).toInt(), 2);
}

void ut_metadata::testImageHeight()
{
    QVERIFY(metadata->isValid());
    QCOMPARE(metadata->entry(Metadata::Tag_ImageHeight).toInt(), 2);
}

void ut_metadata::testFocalLength()
{
    QVERIFY(metadata->isValid());
    QEXPECT_FAIL("", "A bug in libexif prevents verification of this property", Continue);
    QCOMPARE(metadata->entry(Metadata::Tag_FocalLength).toDouble(), 9.9);
}

void ut_metadata::testExposureTime()
{
    QVERIFY(metadata->isValid());
    QEXPECT_FAIL("", "A bug in libexif prevents verification of this property", Continue);
    QCOMPARE(metadata->entry(Metadata::Tag_ExposureTime).toDouble(), 1/200.0);
}

void ut_metadata::testTimestampOriginal()
{
    QVERIFY(metadata->isValid());
    QEXPECT_FAIL("", "A bug in libexif prevents verification of this property", Continue);
    QCOMPARE(metadata->entry(Metadata::Tag_TimestampOriginal).toString(),
             QString("2010:01:25 15:00:00"));
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_metadata test;
    return QTest::qExec( &test, argc, argv );
}
