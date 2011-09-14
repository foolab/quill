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
#include "ut_regions.h"
#include "unittests.h"
#include "../../src/strings.h"
#include "../../src/regionsofinterest.h"

ut_regions::ut_regions()
{
}

void ut_regions::initTestCase()
{
    Quill::initTestingMode();
}

void ut_regions::cleanupTestCase()
{
    Quill::cleanup();
}

void ut_regions::init()
{
}

void ut_regions::cleanup()
{

}

void ut_regions::testCropRegions()
{
    QuillMetadataRegionList regions;
    QuillMetadataRegion region1, region2, region3;

    region1.setArea(QRect(100, 300, 100, 100));
    region1.setName("Anna");
    region2.setArea(QRect(300, 300, 100, 100));
    region2.setName("Bob");
    region3.setArea(QRect(500, 300, 100, 100));
    region3.setName("Charlie");

    regions << region1 << region2 << region3;
    regions.setFullImageSize(QSize(1000, 1000));

    QuillImageFilter *filter =
        new QuillImageFilter(QuillImageFilter::Name_Crop);

    filter->setOption(QuillImageFilter::CropRectangle,
                      QRect(200, 200, 600, 600));

    QuillMetadataRegionList result =
        RegionsOfInterest::applyFilterToRegions(filter, regions);

    QCOMPARE(result.fullImageSize(), QSize(600, 600));
    QCOMPARE(result.count(), 2);
    QuillMetadataRegion result1 = result.at(0);
    QuillMetadataRegion result2 = result.at(1);

    QCOMPARE(result1.area(), QRect(100, 100, 100, 100));
    QCOMPARE(result1.name(), QString("Bob"));
    QCOMPARE(result2.area(), QRect(300, 100, 100, 100));
    QCOMPARE(result2.name(), QString("Charlie"));

    delete filter;
}

void ut_regions::testCropImage()
{
    QTemporaryFile file;
    file.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(file.fileName(), "jpeg");

    QuillMetadataRegionList regions;
    QuillMetadataRegion region1, region2, region3;

    region1.setArea(QRect(0, 0, 2, 2));
    region1.setName("Anna");
    region2.setArea(QRect(3, 0, 2, 2));
    region2.setName("Bob");
    region3.setArea(QRect(6, 0, 2, 2));
    region3.setName("Charlie");

    regions.reserve(3);
    regions << region1 << region2 << region3;
    regions.setFullImageSize(QSize(8, 2));

    QuillMetadata metadata;
    QVariant variant;
    variant.setValue(regions);
    metadata.setEntry(QuillMetadata::Tag_Regions, variant);
    QVERIFY(metadata.write(file.fileName()));

    QuillMetadata iMetadata(file.fileName());
    QVERIFY(iMetadata.entry(QuillMetadata::Tag_Regions).canConvert<QuillMetadataRegionList>());
    QuillMetadataRegionList iResult =
        iMetadata.entry(QuillMetadata::Tag_Regions).
        value<QuillMetadataRegionList>();

    QCOMPARE(iResult.fullImageSize(), QSize(8, 2));
    QCOMPARE(iResult.count(), 3);

    QuillMetadataRegion iResult1 = iResult.at(0);

    QCOMPARE(iResult1.area(), QRect(0, 0, 2, 2));
    QCOMPARE(iResult1.name(), QString("Anna"));

    QuillImageFilter *filter =
        new QuillImageFilter(QuillImageFilter::Name_Crop);

    filter->setOption(QuillImageFilter::CropRectangle,
                      QRect(2, 0, 6, 2));

    QuillFile quillFile(file.fileName(), "image/jpeg");

    quillFile.runFilter(filter);
    quillFile.save();

    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // crop
    Quill::releaseAndWait(); // save

    QCOMPARE(QImage(file.fileName()).size(), QSize(6, 2));

    QuillMetadata resultMetadata(file.fileName());

    QVERIFY(resultMetadata.entry(QuillMetadata::Tag_Regions).canConvert<QuillMetadataRegionList>());

    QuillMetadataRegionList result =
        resultMetadata.entry(QuillMetadata::Tag_Regions).
        value<QuillMetadataRegionList>();

    QCOMPARE(result.fullImageSize(), QSize(6, 2));
    QCOMPARE(result.count(), 2);
    QuillMetadataRegion result1 = result.at(0);
    QuillMetadataRegion result2 = result.at(1);

    QCOMPARE(result1.area(), QRect(1, 0, 2, 2));
    QCOMPARE(result1.name(), QString("Bob"));
    QCOMPARE(result2.area(), QRect(4, 0, 2, 2));
    QCOMPARE(result2.name(), QString("Charlie"));
}

void ut_regions::testUndo()
{
    QTemporaryFile file;
    file.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(file.fileName(), "jpeg");

    QuillMetadataRegionList regions;
    QuillMetadataRegion region1, region2, region3;

    region1.setArea(QRect(0, 0, 2, 2));
    region1.setName("Anna");
    region2.setArea(QRect(3, 0, 2, 2));
    region2.setName("Bob");
    region3.setArea(QRect(6, 0, 2, 2));
    region3.setName("Charlie");

    regions.reserve(3);
    regions << region1 << region2 << region3;
    regions.setFullImageSize(QSize(8, 2));

    QuillMetadata metadata;
    QVariant variant;
    variant.setValue(regions);
    metadata.setEntry(QuillMetadata::Tag_Regions, variant);
    QVERIFY(metadata.write(file.fileName()));

    QuillImageFilter *filter =
        new QuillImageFilter(QuillImageFilter::Name_Crop);

    filter->setOption(QuillImageFilter::CropRectangle,
                      QRect(2, 0, 6, 2));

    QuillFile quillFile(file.fileName(), "image/jpeg");

    quillFile.runFilter(filter);
    quillFile.save();

    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // crop
    Quill::releaseAndWait(); // save

    quillFile.undo();
    quillFile.save();

    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // save

    QCOMPARE(QImage(file.fileName()).size(), QSize(8, 2));

    QuillMetadata resultMetadata(file.fileName());

    QVERIFY(!resultMetadata.entry(QuillMetadata::Tag_Regions).canConvert<QuillMetadataRegionList>());
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_regions test;
    return QTest::qExec( &test, argc, argv );
}
