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

#include <QVariant>
#include <QtTest/QtTest>
#include <QuillImageFilterFactory>
#include <quillmetadata/QuillMetadata>

#include "quill.h"
#include "quillfile.h"
#include "ut_quillmetadata.h"
#include "unittests.h"
#include "../../src/strings.h"

ut_quillmetadata::ut_quillmetadata()
{
}

void ut_quillmetadata::initTestCase()
{
}

void ut_quillmetadata::cleanupTestCase()
{
}

void ut_quillmetadata::init()
{
    metadata = new QuillMetadata("/usr/share/libquill-tests/images/exif.jpg");
    xmp = new QuillMetadata("/usr/share/libquill-tests/images/xmp.jpg");
    iptc = new QuillMetadata("/usr/share/libquill-tests/images/iptc.jpg");
}

void ut_quillmetadata::cleanup()
{
    delete metadata;
    delete xmp;
    delete iptc;
}

void ut_quillmetadata::testPreserveXMP()
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
    QuillFile *quillFile = new QuillFile(file.fileName(), Strings::jpg);
    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_Scale);
    filter->setOption(QuillImageFilter::SizeAfter, QSize(4, 4));
    quillFile->runFilter(filter);
    quillFile->save();
    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // scale
    Quill::releaseAndWait(); // save

    // Verify that file image size has changed
    QCOMPARE(QImage(file.fileName()).size(), QSize(4, 4));
    QuillMetadata writtenMetadata(file.fileName());
    QVERIFY(writtenMetadata.isValid());

    QStringList list;
    list << "test" << "quill";
    QCOMPARE(writtenMetadata.entry(QuillMetadata::Tag_Subject).toStringList(),
             list);
    QCOMPARE(writtenMetadata.entry(QuillMetadata::Tag_City).toString(),
             QString("Tapiola"));
    QCOMPARE(writtenMetadata.entry(QuillMetadata::Tag_Country).toString(),
             QString("Finland"));
    QCOMPARE(writtenMetadata.entry(QuillMetadata::Tag_Rating).toInt(),
             5);
    QCOMPARE(writtenMetadata.entry(QuillMetadata::Tag_Creator).toString(),
             QString("John Q"));
    delete quillFile;
}

void ut_quillmetadata::testPreserveIptc()
{
    QTemporaryFile file;
    file.open();

    // Perform an overwriting copy since Qt does not have such function
    QFile originalFile("/usr/share/libquill-tests/images/iptc.jpg");
    originalFile.open(QIODevice::ReadOnly);
    QByteArray buffer = originalFile.readAll();
    file.write(buffer);
    file.flush();

    QCOMPARE(QImage(file.fileName()).size(), QSize(2, 2));

    Quill::initTestingMode();
    QuillFile *quillFile = new QuillFile(file.fileName(), Strings::jpg);
    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_Scale);
    filter->setOption(QuillImageFilter::SizeAfter, QSize(4, 4));
    quillFile->runFilter(filter);
    quillFile->save();
    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // scale
    Quill::releaseAndWait(); // save

    // Verify that file image size has changed
    QCOMPARE(QImage(file.fileName()).size(), QSize(4, 4));
    QuillMetadata writtenMetadata(file.fileName());
    QVERIFY(writtenMetadata.isValid());

    QCOMPARE(writtenMetadata.entry(QuillMetadata::Tag_City).toString(),
             QString("Tapiola"));
    QCOMPARE(writtenMetadata.entry(QuillMetadata::Tag_Country).toString(),
             QString("Finland"));
    delete quillFile;
}

void ut_quillmetadata::testPreserveExif()
{
    QTemporaryFile file;
    file.open();

    // Perform an overwriting copy since Qt does not have such function
    QFile originalFile("/usr/share/libquill-tests/images/exif.jpg");
    originalFile.open(QIODevice::ReadOnly);
    QByteArray buffer = originalFile.readAll();
    file.write(buffer);
    file.flush();

    QCOMPARE(QImage(file.fileName()).size(), QSize(2, 2));

    Quill::initTestingMode();
    QuillFile *quillFile = new QuillFile(file.fileName(), Strings::jpg);
    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_Scale);
    filter->setOption(QuillImageFilter::SizeAfter, QSize(4, 4));
    quillFile->runFilter(filter);
    quillFile->save();
    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // scale
    Quill::releaseAndWait(); // save

    // Verify that file image size has changed
    QCOMPARE(QImage(file.fileName()).size(), QSize(4, 4));
    QuillMetadata writtenMetadata(file.fileName());
    QVERIFY(writtenMetadata.isValid());

    QCOMPARE(writtenMetadata.entry(QuillMetadata::Tag_Make).toString(),
             QString("Quill"));
    QCOMPARE(writtenMetadata.entry(QuillMetadata::Tag_Model).toString(),
             QString("Q100125"));
    QCOMPARE(writtenMetadata.entry(QuillMetadata::Tag_ImageWidth).toInt(), 2);
    QCOMPARE(writtenMetadata.entry(QuillMetadata::Tag_ImageHeight).toInt(), 2);
    Unittests::compareReal(writtenMetadata.entry(QuillMetadata::Tag_FocalLength).toDouble(), 9.9);
    Unittests::compareReal(writtenMetadata.entry(QuillMetadata::Tag_ExposureTime).toDouble(), 1/200.0);
    QCOMPARE(metadata->entry(QuillMetadata::Tag_TimestampOriginal).toString(),
             QString("2010:01:25 15:00:00"));
    delete quillFile;
}

void ut_quillmetadata::testResetOrientation()
{
    QTemporaryFile file;
    file.open();

    // Perform an overwriting copy since Qt does not have such function
    QFile originalFile("/usr/share/libquill-tests/images/exif_orientation.jpg");
    originalFile.open(QIODevice::ReadOnly);
    QByteArray buffer = originalFile.readAll();
    file.write(buffer);
    file.flush();

    Quill::initTestingMode();
    QuillFile *quillFile = new QuillFile(file.fileName(), Strings::jpg);
    QuillMetadata originalMetadata(file.fileName());
    // Verify original orientation
    QCOMPARE(originalMetadata.entry(QuillMetadata::Tag_Orientation).toInt(),
	     2);

    QuillImageFilter *filter =
	QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_Scale);
    filter->setOption(QuillImageFilter::SizeAfter, QSize(4, 4));
    quillFile->runFilter(filter);
    quillFile->save();
    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // scale
    Quill::releaseAndWait(); // save

    // Verify that orientation has changed to default
    QuillMetadata writtenMetadata(file.fileName());
    QVERIFY(writtenMetadata.isValid());
    QCOMPARE(writtenMetadata.entry(QuillMetadata::Tag_Orientation).toInt(),
	     1);
    delete quillFile;
}

void ut_quillmetadata::testNoOrientation()
{
    QTemporaryFile file;
    file.open();

    // Perform an overwriting copy since Qt does not have such function
    QFile originalFile("/usr/share/libquill-tests/images/exif.jpg");
    originalFile.open(QIODevice::ReadOnly);
    QByteArray buffer = originalFile.readAll();
    file.write(buffer);
    file.flush();

    Quill::initTestingMode();
    QuillFile *quillFile = new QuillFile(file.fileName(), Strings::jpg);
    QuillMetadata originalMetadata(file.fileName());
    // Verify empty orientation info
    QVERIFY(originalMetadata.entry(QuillMetadata::Tag_Orientation).isNull());

    QuillImageFilter *filter =
	QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_Scale);
    filter->setOption(QuillImageFilter::SizeAfter, QSize(4, 4));
    quillFile->runFilter(filter);
    quillFile->save();
    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // scale
    Quill::releaseAndWait(); // save

    // Verify that no orientation info has been added
    QuillMetadata writtenMetadata(file.fileName());
    QVERIFY(writtenMetadata.isValid());
    QVERIFY(writtenMetadata.entry(QuillMetadata::Tag_Orientation).isNull());
    delete quillFile;
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_quillmetadata test;
    return QTest::qExec( &test, argc, argv );
}
