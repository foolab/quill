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
#include <QuillImage>
#include <QDebug>
#include <QuillImageFilter>
#include "unittests.h"
#include "ut_imagecache.h"

ut_imagecache::ut_imagecache()
{
    QuillImageFilter::registerAll();
}

void ut_imagecache::initTestCase()
{
    cache = new ImageCache(500);
}

void ut_imagecache::cleanupTestCase()
{
    delete cache;
}

void ut_imagecache::testInsert()

{
    bool result;

    image=Unittests::generatePaletteImage();
    result=cache->insertImage(1,image);
    QVERIFY2(result==true, "image is not inserted successfully.");
    //one pic in cache
    testCount(1);
    qDebug()<<"the total cost used 1: "<<cache->cacheTotalCost();
    image=Unittests::generatePaletteImage();
    result=cache->insertImage(2,image);
    QVERIFY2(result==true, "image is not inserted successfully.");
    //two pics in cache
    testCount(2);
    qDebug()<<"the total cost used 2: "<<cache->cacheTotalCost();
    image=Unittests::generatePaletteImage();
    result=cache->insertImage(3,image,ImageCache::ProtectFirst);
    QVERIFY2(result==true, "image is not inserted successfully.");
    testCount(2);
    //one pic in not delete
    testCountProtected(1);

    image=Unittests::generatePaletteImage();
    result=cache->insertImage(4,image);
    QVERIFY2(result==true, "image is not inserted successfully.");
    //three pics in cache
    testCount(3);
    qDebug()<<"the total cost used 3: "<<cache->cacheTotalCost();
    image=Unittests::generatePaletteImage();
    result=cache->insertImage(5,image);
    QVERIFY2(result==true, "image is not inserted successfully.");
    //four pics in cache
    testCount(4);
    qDebug()<<"the total cost used 4: "<<cache->cacheTotalCost();
    image=Unittests::generatePaletteImage();
    result=cache->insertImage(6,image, ImageCache::ProtectLast);
    QVERIFY2(result==true, "image is not inserted successfully.");
    //four pics in cache
    //two pics in not delete
    testCount(4);
    testCountProtected(2);

    image=Unittests::generatePaletteImage();
    result=cache->insertImage(7,image);
    QVERIFY2(result==true, "image is not inserted successfully.");
    //five pics in cache
    testCount(5);
    qDebug()<<"the total cost used 5: "<<cache->cacheTotalCost();
    image=Unittests::generatePaletteImage();
    result=cache->insertImage(8,image);
    QVERIFY2(result==true, "image is not inserted successfully.");
    //six pics in cache
    testCount(6);
    qDebug()<<"the total cost used 6: "<<cache->cacheTotalCost();
    image=Unittests::generatePaletteImage();
    result=cache->insertImage(9,image,ImageCache::ProtectLast);
    QVERIFY2(result==true, "image is not inserted successfully.");
    //key 6 is moved from not delete to cache
    testCountProtected(2);
    testCount(7);
    qDebug()<<"the total cost used 7: "<<cache->cacheTotalCost();
    //key 6 is in cache
    testCacheCheck(6, true);
    testCacheProtectedCheck(6, false);

    image=Unittests::generatePaletteImage();
    result=cache->insertImage(10,image);
    QVERIFY2(result==true, "image is not inserted successfully.");
    //7 pics in cache.
    //key one is deleted
    testCount(7);
    testCacheCheck(1, false);

    image=Unittests::generatePaletteImage();
    result=cache->insertImage(11,image);
    QVERIFY2(result==true, "image is not inserted successfully.");
    //7 pics i cache
    //key 2 is deleted
    testCount(7);
    qDebug()<<"the total cost used 8: "<<cache->cacheTotalCost();
    testCacheCheck(2, false);

    //Testing change status
    testChangeProtectionStatus(11,ImageCache::ProtectLast);
    testCacheCheck(11,false);
    testCacheProtectedCheck(11, true);
    testCacheCheck(9,true);
    testCacheProtectedCheck(9,false);
    //6 pics in cache
    //2 pics in not delete
    testCount(6);
    testCountProtected(2);
    qDebug()<<"the total cost used 9: "<<cache->cacheTotalCost();

    //key 2 was deleted already, nothing happens
    testChangeProtectionStatus(2,ImageCache::ProtectLast);
    testCacheCheck(2,false);
    testCacheProtectedCheck(2,false);
    testCacheCheck(11,false);
    testCacheProtectedCheck(11,true);
    testCount(6);
    testCountProtected(2);
    qDebug()<<"the total cost used 10: "<<cache->cacheTotalCost();

    testChangeProtectionStatus(11,ImageCache::NotProtected);
    testCacheCheck(11,true);
    testCacheProtectedCheck(11, false);
    qDebug()<<"the total cost used 11: "<<cache->cacheTotalCost();
    qDebug()<<"the number of images in cache is: "<<cache->countCache();
    testCount(7);
    testCountProtected(1);
    //key 4 is to be deleted
    testCacheCheck(4, false);

    QList<int> list = cache->checkKeys();
    for(int i=0; i<list.count();i++)
      qDebug()<<"the key in cache is: "<<list.at(i);

    QCOMPARE(cache->image(5),image);
}

void ut_imagecache::testCount(int number)
{
    int num = cache->countCache();
    QCOMPARE(num,number);
}

void ut_imagecache::testCountProtected(int number)
{
    int num = cache->countCacheProtected();
    QCOMPARE(num,number);
}

void ut_imagecache::testCacheCheck(int key, bool flag)
{
    bool result=cache->cacheCheck(key);
    if(flag){
      qDebug()<<"key: "<<key;
      QVERIFY2(result==true, "it is not in cache");
    }
    else{
      qDebug()<<"key: "<<key;
      QVERIFY2(result==false, "it is in cache");
    }
}

void ut_imagecache::testCacheProtectedCheck(int key, bool flag)
{
    bool result = cache->cacheProtectedCheck(key);
    if(flag)
      QVERIFY2(result==true, "it is not in Not delete");
    else
      QVERIFY2(result==false, "it is in Not delete");
}

void ut_imagecache::testChangeProtectionStatus(int key, ImageCache::ProtectionStatus status)
{
    cache->changeProtectionStatus(key, status);
}

int ut_imagecache::testCacheTotalCost()
{
    return cache->cacheTotalCost();
}

void ut_imagecache::testInsert2()
{
    QuillImageFilter::registerAll();

    QtImageFilter *filter =
        QtImageFilterFactory::createImageFilter("BrightnessContrast");
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness,
                      QVariant(16));

    ImageCache *cache = new ImageCache(500);
    QuillImage image = Unittests::generatePaletteImage();
    QuillImage image2 = filter->apply(image);

    cache->insertImage(1, image, ImageCache::ProtectLast);
    QCOMPARE(cache->image(1), image);
    cache->insertImage(2, image2, ImageCache::ProtectLast);
    QCOMPARE(cache->image(1), image);
    QCOMPARE(cache->image(2), image2);

    delete filter;
    delete cache;
}

void ut_imagecache::testDoubleInsert()
{
    QuillImageFilter::registerAll();

    QtImageFilter *filter =
        QtImageFilterFactory::createImageFilter("BrightnessContrast");
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness,
                      QVariant(16));

    ImageCache *cache = new ImageCache(500);
    QuillImage image = Unittests::generatePaletteImage();
    QuillImage image2 = filter->apply(image);

    cache->insertImage(1, image, ImageCache::ProtectLast);
    QCOMPARE(cache->image(1), image);
    cache->insertImage(1, image2, ImageCache::ProtectLast);
    QCOMPARE(cache->image(1), image2);

    delete filter;
    delete cache;
}

void ut_imagecache::testChangeProtectionStatus2()

{
    QuillImageFilter::registerAll();

    QtImageFilter *filter =
        QtImageFilterFactory::createImageFilter("BrightnessContrast");
    QVERIFY(filter);

    ImageCache *cache = new ImageCache(0);
    QuillImage image = Unittests::generatePaletteImage();
    QuillImage image2 = filter->apply(image);
    QuillImage image3 = filter->apply(image2);

    cache->insertImage(1, image, ImageCache::ProtectLast);
    QCOMPARE(cache->image(1), image);

    cache->changeProtectionStatus(1, ImageCache::ProtectFirst);
    QCOMPARE(cache->image(1), image);

    // Replace the "protect last" status, the first image should still stay
    // because of the "protect first" status.

    cache->insertImage(2, image2, ImageCache::ProtectLast);
    QCOMPARE(cache->image(2), image2);
    QCOMPARE(cache->image(1), image);

    // Replace the "protect first" status, the first image
    // is no longer protected and should be removed.

    cache->changeProtectionStatus(2, ImageCache::ProtectFirst);
    QCOMPARE(cache->image(2), image2);
    QCOMPARE(cache->image(1), QuillImage());

    // Replace the "protect first" status, the second image
    // should still stay because of the "protect last" status.

    cache->insertImage(3, image3, ImageCache::ProtectFirst);
    QCOMPARE(cache->image(3), image3);
    QCOMPARE(cache->image(2), image2);

    // Replace the "protect last" status, the second image
    // is no longer protected and should be removed.

    cache->changeProtectionStatus(3, ImageCache::ProtectLast);
    QCOMPARE(cache->image(3), image3);
    QCOMPARE(cache->image(2), QuillImage());
    QCOMPARE(cache->image(1), QuillImage());

    delete filter;
    delete cache;
}

int main ( int argc, char *argv[] ){
    QApplication app( argc, argv );
    ut_imagecache test;
    return QTest::qExec( &test, argc, argv );
}
