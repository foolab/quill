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
#include "ut_export.h"
#include "quillfile.h"
#include "quillundocommand.h"
#include "quillundostack.h"

ut_export::ut_export()
{
}

Q_DECLARE_METATYPE(QuillImage);

void ut_export::initTestCase()
{
    QuillImageFilter::registerAll();
}

void ut_export::cleanupTestCase()
{
}

void ut_export::init()
{
    Quill::initTestingMode();
}

void ut_export::cleanup()
{
    Quill::cleanup();
}

void ut_export::testBasicExport()
{
    QTemporaryFile testFile;
    testFile.open();

    QTemporaryFile testFile2;
    testFile2.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillFile *file = new QuillFile(testFile.fileName(), "png");

    file->saveAs(testFile2.fileName(), "tif");

    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // save

    QVERIFY(testFile2.size() > 0);

    QVERIFY(Unittests::compareImage(QImage(testFile2.fileName(), "tif"),
                                    QImage(testFile.fileName(), "png")));
    delete file;
}

void ut_export::testExportIgnoringEditHistory()
{
    QTemporaryFile testFile;
    testFile.open();

    QTemporaryFile testFile2;
    testFile2.open();

    QuillImage image = Unittests::generatePaletteImage();
    image.save(testFile.fileName(), "png");

    QuillImageFilter *filter =
        QuillImageFilterFactory::createImageFilter("org.maemo.composite.brightness.contrast");
    filter->setOption(QuillImageFilter::Brightness, QVariant(20));

    QuillFile *file = new QuillFile(testFile.fileName(), "png");

    file->runFilter(filter);
    file->saveAs(testFile2.fileName(), "tif");

    Quill::releaseAndWait(); // load
    Quill::releaseAndWait(); // filter
    Quill::releaseAndWait(); // save

    QVERIFY(Unittests::compareImage(QImage(testFile2.fileName(), "tif"),
                                    filter->apply(image)));
    delete file;

    file = new QuillFile(testFile2.fileName(), "tif");
    QVERIFY(!file->canUndo());

    delete file;
}


int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_export test;
    return QTest::qExec( &test, argc, argv );

}
