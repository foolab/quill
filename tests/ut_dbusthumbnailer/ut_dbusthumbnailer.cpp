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
#include <QSignalSpy>
#include "thumbnailer_generic.h"
#include "ut_dbusthumbnailer.h"
#include "dbusthumbnailer.h"

ut_dbusthumbnailer::ut_dbusthumbnailer()
{
}

void ut_dbusthumbnailer::initTestCase()
{

}

void ut_dbusthumbnailer::cleanupTestCase()
{
}

void ut_dbusthumbnailer::init()
{
}

void ut_dbusthumbnailer::cleanup()
{
}

void ut_dbusthumbnailer::testSupports()
{
    DBusThumbnailer* dbusThumbnailer = new DBusThumbnailer();
    QCOMPARE(dbusThumbnailer->supports("video/mp4"),true);
    QCOMPARE(dbusThumbnailer->supports("image/jpeg"),false);
    QCOMPARE(dbusThumbnailer->supports("image/jpeg3"),false);
    delete dbusThumbnailer;
}
void ut_dbusthumbnailer::testIsRunning()
{
    DBusThumbnailer* dbusThumbnailer = new DBusThumbnailer();
    QCOMPARE(dbusThumbnailer->isRunning(),false);
    delete dbusThumbnailer;
}

void ut_dbusthumbnailer::testSuccess()
{
    DBusThumbnailer dbusThumbnailer;
    QSignalSpy spy(&dbusThumbnailer,SIGNAL(thumbnailGenerated(const QString,
                                                              const QString)));

    dbusThumbnailer.newThumbnailerTask("/usr/share/libquill-tests/video/Alvin_2.mp4","video/mp4","normal");

    dbusThumbnailer.finishedHandler(2);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.first();
    QCOMPARE(arguments.at(0).toString(),QString("/usr/share/libquill-tests/video/Alvin_2.mp4"));
    QCOMPARE(arguments.at(1).toString(),QString("normal"));
}

void ut_dbusthumbnailer::testError()
{
    DBusThumbnailer dbusThumbnailer;
    QSignalSpy spy(&dbusThumbnailer,SIGNAL(thumbnailError(const QString, uint,
                                                          const QString)));
    dbusThumbnailer.newThumbnailerTask("invalid.mp4","invalid","normal");

    QStringList error;
    error << "invalid.mp4";
    dbusThumbnailer.errorHandler(4,error,5,"this is an error");

    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.first();
    QCOMPARE(arguments.at(0).toString(), QString("invalid.mp4"));
    QCOMPARE(arguments.at(1).toInt(), 5);
    QCOMPARE(arguments.at(2).toString(), QString("this is an error"));
}

int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_dbusthumbnailer test;
    return QTest::qExec( &test, argc, argv );
}
