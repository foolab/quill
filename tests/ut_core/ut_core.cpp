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
#include <QuillImageFilter>
#include <QuillImageFilterFactory>
#include <QuillImageFilterGenerator>

#include "core.h"
#include "unittests.h"
#include "ut_core.h"

ut_core::ut_core()
{
}

void ut_core::initTestCase()
{
}

void ut_core::cleanupTestCase()
{
}

void ut_core::testSetPreviewLevelCount()
{
    Core* core = new Core();
    core->setPreviewLevelCount(3);
    core->setPreviewSize(0, QSize(3, 4));
    core->setPreviewSize(1, QSize(5, 6));
    core->setPreviewSize(2, QSize(7, 8));

    QCOMPARE(core->previewLevelCount(), 3);
    QCOMPARE(core->previewSize(0), QSize(3, 4));
    QCOMPARE(core->previewSize(1), QSize(5, 6));
    QCOMPARE(core->previewSize(2), QSize(7, 8));

    core->setPreviewLevelCount(1);

    QCOMPARE(core->previewLevelCount(), 1);
    QCOMPARE(core->previewSize(0), QSize(3, 4));
    QCOMPARE(core->previewSize(1), QSize());
    QCOMPARE(core->previewSize(2), QSize());

    delete core;
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_core test;
    return QTest::qExec( &test, argc, argv );

}
