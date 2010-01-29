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
}

void ut_metadata::cleanup()
{
}

void ut_metadata::testCameraMake()
{
    Metadata *metadata = new Metadata("exif.jpg");
    QVERIFY(metadata->isValid());
    QCOMPARE(metadata->entry(Metadata::Tag_Make).toString(), QString("Quill"));
    delete metadata;
}

void ut_metadata::testCameraModel()
{
    Metadata *metadata = new Metadata("exif.jpg");
    QVERIFY(metadata->isValid());
    QCOMPARE(metadata->entry(Metadata::Tag_Model).toString(), QString("Q100125"));
    delete metadata;
}

void ut_metadata::testImageWidth()
{
    Metadata *metadata = new Metadata("exif.jpg");
    QVERIFY(metadata->isValid());
    QCOMPARE(metadata->entry(Metadata::Tag_ImageWidth).toInt(), 2);
    delete metadata;
}

void ut_metadata::testImageHeight()
{
    Metadata *metadata = new Metadata("exif.jpg");
    QVERIFY(metadata->isValid());
    QCOMPARE(metadata->entry(Metadata::Tag_ImageHeight).toInt(), 2);
    delete metadata;
}

void ut_metadata::testFocalLength()
{
    Metadata *metadata = new Metadata("exif.jpg");
    QVERIFY(metadata->isValid());
    QCOMPARE(metadata->entry(Metadata::Tag_FocalLength).toDouble(), 9.9);
    delete metadata;
}

void ut_metadata::testExposureTime()
{
    Metadata *metadata = new Metadata("exif.jpg");
    QVERIFY(metadata->isValid());
    QCOMPARE(metadata->entry(Metadata::Tag_ExposureTime).toDouble(), 1/200.0);
    delete metadata;
}

void ut_metadata::testTimestampOriginal()
{
    Metadata *metadata = new Metadata("exif.jpg");
    QVERIFY(metadata->isValid());
    QCOMPARE(metadata->entry(Metadata::Tag_TimestampOriginal).toString(),
             QString("2010:01:25 15:00:00"));
    delete metadata;
}

void ut_metadata::testSubject()
{
    Metadata *metadata = new Metadata("xmp.jpg");
    QVERIFY(metadata->isValid());
    QCOMPARE(metadata->entry(Metadata::Tag_Subject).toString(),
             QString("test,quill"));
}

void ut_metadata::testCity()
{
    Metadata *metadata = new Metadata("xmp.jpg");
    QVERIFY(metadata->isValid());
    QCOMPARE(metadata->entry(Metadata::Tag_City).toString(),
             QString("Tapiola"));
}

void ut_metadata::testCountry()
{
    Metadata *metadata = new Metadata("xmp.jpg");
    QVERIFY(metadata->isValid());
    QCOMPARE(metadata->entry(Metadata::Tag_Country).toString(),
             QString("Finland"));
}

void ut_metadata::testRating()
{
    Metadata *metadata = new Metadata("xmp.jpg");
    QVERIFY(metadata->isValid());
    QCOMPARE(metadata->entry(Metadata::Tag_Rating).toInt(),
             5);
}

void ut_metadata::testCreator()
{
    Metadata *metadata = new Metadata("xmp.jpg");
    QVERIFY(metadata->isValid());
    QCOMPARE(metadata->entry(Metadata::Tag_Creator).toString(),
             QString("John Q"));
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_metadata test;
    return QTest::qExec( &test, argc, argv );
}
