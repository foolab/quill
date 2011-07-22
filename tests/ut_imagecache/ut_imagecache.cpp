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
#include <QuillImage>
#include <QDebug>
#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include "unittests.h"
#include "file.h"
#include "ut_imagecache.h"
#include "../../src/strings.h"

void ut_imagecache::initTestCase()
{
    file = new File();
}

void ut_imagecache::cleanupTestCase()
{
    delete file;
    Quill::cleanup();
}

void ut_imagecache::testInsert2()
{
    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(16));

    ImageCache *cache = new ImageCache(500);
    QuillImage image = Unittests::generatePaletteImage();
    QuillImage image2 = filter->apply(image);

    cache->insert(file, 1, image, ImageCache::Protected);
    QCOMPARE(cache->image(file, 1), image);
    cache->insert(file, 2, image2, ImageCache::Protected);
    QCOMPARE(cache->image(file, 1), image);
    QCOMPARE(cache->image(file, 2), image2);

    delete filter;
    delete cache;
}

void ut_imagecache::testInsertReplace()
{
    QuillImageFilter *filter =
    QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness,
                      QVariant(16));

    ImageCache *cache = new ImageCache(500);
    QuillImage image = Unittests::generatePaletteImage();
    QuillImage image2 = filter->apply(image);

    cache->insert(file, 1, image, ImageCache::Protected);
    QCOMPARE(cache->image(file, 1), image);
    cache->insert(file, 1, image2, ImageCache::Protected);
    QCOMPARE(cache->image(file, 1), image2);

    delete filter;
    delete cache;
}

void ut_imagecache::testProtect()
{
    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(16));

    ImageCache *cache = new ImageCache(1);
    QuillImage image = Unittests::generatePaletteImage();
    QuillImage image2 = filter->apply(image);

    cache->insert(file, 1, image, ImageCache::NotProtected);
    QCOMPARE(cache->image(file, 1), image);

    cache->protect(file, 1);
    QCOMPARE(cache->image(file, 1), image);

    // The "not protected" new image should not replace the "protected" one.
    cache->insert(file, 2, image2, ImageCache::NotProtected);
    QCOMPARE(cache->image(file, 2), image2);
    QCOMPARE(cache->image(file, 1), image);

    delete filter;
    delete cache;
}

void ut_imagecache::testMultipleFile()
{
    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter(QuillImageFilter::Name_BrightnessContrast);
    QVERIFY(filter);
    filter->setOption(QuillImageFilter::Brightness, QVariant(16));
    File *file2 = new File();

    ImageCache *cache = new ImageCache(0);
    QuillImage image = Unittests::generatePaletteImage();
    QuillImage image2 = filter->apply(image);
    QuillImage image3 = filter->apply(image2);

    cache->insert(file, 1, image, ImageCache::Protected);
    QCOMPARE(cache->image(file, 1), image);

    cache->insert(file2, 2, image2, ImageCache::Protected);
    QCOMPARE(cache->image(file2, 2), image2);

    cache->insert(file, 3, image3, ImageCache::Protected);
    QCOMPARE(cache->image(file, 3), image3);

    QCOMPARE(cache->image(file2, 2), image2);
    // The old image should drop from the cache.
    QCOMPARE(cache->image(file, 1), QuillImage());

    delete filter;
    delete cache;

    delete file2;
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_imagecache test;
    return QTest::qExec( &test, argc, argv );
}
