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

#include <QDebug>
#include <QtTest/QtTest>
#include <QImage>
#include <QuillImageFilter>

#include "tilemap.h"
#include "tilecache.h"
#include "savemap.h"
#include "unittests.h"
#include "ut_savemap.h"

ut_savemap::ut_savemap()
{
}

void ut_savemap::initTestCase()
{
}

void ut_savemap::cleanupTestCase()
{
}

// Test buffer area layout.

void ut_savemap::testBufferArea()
{
    TileCache cache;
    TileMap tileMap(QSize(1, 1), QSize(1, 1), &cache);
    SaveMap map(QSize(2, 7), 4, &tileMap);

    QCOMPARE(map.buffer().area(), QRect(0, 0, 2, 2));

    map.nextBuffer();
    QCOMPARE(map.buffer().area(), QRect(0, 2, 2, 2));

    map.nextBuffer();
    QCOMPARE(map.buffer().area(), QRect(0, 4, 2, 2));

    map.nextBuffer();
    QCOMPARE(map.buffer().area(), QRect(0, 6, 2, 1));

    map.nextBuffer();
    QVERIFY(map.isSaveComplete());
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_savemap test;
    return QTest::qExec( &test, argc, argv );
}
