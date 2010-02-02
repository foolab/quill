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
#include <QuillImageFilterFactory>

#include "quill.h"
#include "quillfile.h"

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
    xmp = new Metadata("/usr/share/libquill-tests/images/xmp.jpg");
    iptc = new Metadata("/usr/share/libquill-tests/images/iptc.jpg");
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

void ut_metadata::testSubject()
{
    QVERIFY(xmp->isValid());
    QCOMPARE(xmp->entry(Metadata::Tag_Subject).toString(),
             QString("test,quill"));
}

void ut_metadata::testCity()
{
    QVERIFY(xmp->isValid());
    QCOMPARE(xmp->entry(Metadata::Tag_City).toString(),
             QString("Tapiola"));
}

void ut_metadata::testCountry()
{
    QVERIFY(xmp->isValid());
    QCOMPARE(xmp->entry(Metadata::Tag_Country).toString(),
             QString("Finland"));
}

void ut_metadata::testRating()
{
    QVERIFY(xmp->isValid());
    QCOMPARE(xmp->entry(Metadata::Tag_Rating).toInt(),
             5);
}

void ut_metadata::testCreator()
{
    QVERIFY(xmp->isValid());
    QCOMPARE(xmp->entry(Metadata::Tag_Creator).toString(),
             QString("John Q"));
}

void ut_metadata::testCityIptc()
{
    QVERIFY(iptc->isValid());
    QCOMPARE(iptc->entry(Metadata::Tag_City).toString(),
             QString("Tapiola"));
}

void ut_metadata::testCountryIptc()
{
    QVERIFY(iptc->isValid());
    QCOMPARE(iptc->entry(Metadata::Tag_Country).toString(),
             QString("Finland"));
}


void ut_metadata::testWriteSubject()
{
    QTemporaryFile file;
    file.open();
    Unittests::generatePaletteImage().save(file.fileName(), "jpg");
    xmp->write(file.fileName());

    Metadata writtenMetadata(file.fileName());
    QVERIFY(writtenMetadata.isValid());
    QCOMPARE(writtenMetadata.entry(Metadata::Tag_Subject).toString(),
             QString("test,quill"));
}

void ut_metadata::testWriteCity()
{
    QTemporaryFile file;
    file.open();
    Unittests::generatePaletteImage().save(file.fileName(), "jpg");
    xmp->write(file.fileName());

    Metadata writtenMetadata(file.fileName());
    QVERIFY(writtenMetadata.isValid());
    QCOMPARE(writtenMetadata.entry(Metadata::Tag_City).toString(),
             QString("Tapiola"));
}

void ut_metadata::testPreserveXMP()
{
    QTemporaryFile file;
    file.open();

    // Perform an overwriting copy since Qt does not have such function
    QFile originalFile("/usr/share/libquill-tests/images/xmp.jpg");
    originalFile.open(QIODevice::ReadOnly);
    QByteArray buffer = originalFile.readAll();
    file.write(buffer);
    file.flush();

    QCOMPARE(QImage(file.fileName()).size(), QSize(2, 2));

    Quill::initTestingMode();
    QuillFile *quillFile = new QuillFile(file.fileName(), "jpg");
    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("org.maemo.scale");
    filter->setOption(QuillImageFilter::SizeAfter, QSize(4, 4));
    quillFile->runFilter(filter);
    quillFile->save();
    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // scale
    Quill::releaseAndWait(); // save

    // Verify that file image size has changed
    QCOMPARE(QImage(file.fileName()).size(), QSize(4, 4));
    Metadata writtenMetadata(file.fileName());
    QVERIFY(writtenMetadata.isValid());

    QCOMPARE(writtenMetadata.entry(Metadata::Tag_Subject).toString(),
             QString("test,quill"));
    QCOMPARE(writtenMetadata.entry(Metadata::Tag_City).toString(),
             QString("Tapiola"));
    QCOMPARE(writtenMetadata.entry(Metadata::Tag_Country).toString(),
             QString("Finland"));
    QCOMPARE(writtenMetadata.entry(Metadata::Tag_Rating).toInt(),
             5);
    QCOMPARE(writtenMetadata.entry(Metadata::Tag_Creator).toString(),
             QString("John Q"));
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_metadata test;
    return QTest::qExec( &test, argc, argv );
}
