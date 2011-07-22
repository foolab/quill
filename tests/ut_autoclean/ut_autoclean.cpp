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
#include "ut_autoclean.h"

ut_autoclean::ut_autoclean()
{
}

void ut_autoclean::initTestCase()
{
}

void ut_autoclean::cleanupTestCase()
{
}

void ut_autoclean::init()
{
    autoclean = new Autoclean;
}

void ut_autoclean::cleanup()
{
    delete autoclean;
}


void ut_autoclean::testHistoryPathDefault()
{
    QCOMPARE(autoclean->editHistoryPath(),
             QDir::homePath() + QLatin1String("/.config/quill/history"));
}

void ut_autoclean::testHistoryPathSet()
{
    autoclean->setEditHistoryPath(QLatin1String("/tmp/quill/history"));

    QCOMPARE(autoclean->editHistoryPath(), QLatin1String("/tmp/quill/history"));
}

void ut_autoclean::testThumbnailPathsDefault()
{
    QList<QString> pathList = autoclean->thumbnailPaths();

    QCOMPARE(pathList.count(), 1);

    QCOMPARE(pathList.first(),
             QDir::homePath() + QLatin1String("/.thumbnails/normal"));
}

void ut_autoclean::testThumbnailPathsSet()
{
    QList<QString> pathList;
    pathList.append(QLatin1String("/tmp/quill/thumbnails/normal"));
    pathList.append(QLatin1String("/tmp/quill/thumbnails/wide"));
    pathList.append(QLatin1String("/tmp/quill/thumbnails/screen"));

    autoclean->setThumbnailPaths(pathList);

    QList<QString> resultPath = autoclean->thumbnailPaths();
    QCOMPARE(resultPath.count(), 3);

    QCOMPARE(resultPath.at(0), QLatin1String("/tmp/quill/thumbnails/normal"));
    QCOMPARE(resultPath.at(1), QLatin1String("/tmp/quill/thumbnails/wide"));
    QCOMPARE(resultPath.at(2), QLatin1String("/tmp/quill/thumbnails/screen"));
}

void ut_autoclean::testGetMainFileNameNormal()
{
    QByteArray xml = QString(QLatin1String("<?xml version=\"1.0\" encoding=\"UTF-8\"?><!DOCTYPE QuillEditHistory><Core><QuillUndoStack><File name=\"/tmp/quill/test.jpeg\"></QuillUndoStack></Core>")).toAscii();

    QTemporaryFile testFile;
    testFile.open();
    testFile.write(xml);
    testFile.flush();

    QCOMPARE(Autoclean::getMainFileName(testFile.fileName()), QLatin1String("/tmp/quill/test.jpeg"));
}

void ut_autoclean::testGetMainFileNameBroken()
{
    QVERIFY(Autoclean::getMainFileName(QLatin1String("")).isNull());
}

void ut_autoclean::testOriginalFileName()
{
    QString fileName = QLatin1String("/tmp/quill/test.jpeg");

    QCOMPARE(Autoclean::originalFileName(fileName), QLatin1String("/tmp/quill/.original//test.jpeg"));
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_autoclean test;
    return QTest::qExec( &test, argc, argv );
}
