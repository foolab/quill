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
#include <QDir>
#include <QFile>
#include <QDate>
#include <QTime>
#include <QDebug>
#include "../../src/logger.h"
#include "ut_logger.h"

ut_logger::ut_logger()
{
}

void ut_logger::initTestCase()
{

}

void ut_logger::cleanupTestCase()
{
}

void ut_logger::init()
{
}

void ut_logger::cleanup()
{

}

void ut_logger::testLog()
{
    QString homePath = QDir::homePath();
    QDir logPath(homePath+"/.local/share/quill/");

    if(!logPath.exists()){
        logPath.mkpath(homePath+"/.local/share/quill/");
        QCOMPARE(logPath.exists(),true);
    }
    QFile data(homePath+"/.local/share/quill/log.txt");
    data.open(QFile::ReadWrite| QFile::Append);
    data.close();
    QCOMPARE(data.exists(),true);

    QString logString ="this is logging info!";
    Logger::log(logString);

    data.open(QFile::ReadOnly);
    QTextStream stream(&data);
    QString logInfo = "";
    QString logInfo1 = "";
    do{
        logInfo= stream.readLine();
        if(!logInfo.isNull())
            logInfo1 = logInfo;
    }while(!logInfo.isNull());
    data.close();
    bool ret = logInfo1.contains(logString);
    QCOMPARE(ret,true);

    QString dateString = logInfo1.left(10);
    QDate date = QDate::fromString(dateString,"yyyy-MM-dd");
    QCOMPARE(date.isValid(),true);
    QString timeString = logInfo1.mid(11,12);
    QString timeFormat = "hh:mm:ss:zzz";
    QTime time = QTime::fromString(timeString,timeFormat);
    QCOMPARE(time.isValid(),true);

    QCOMPARE(Logger::existLog(),true);
}

void ut_logger::testIntToString()
{
    int number = 10;
    QString numberString = Logger::intToString(number);
    QCOMPARE(numberString,QString(":10"));
}
void ut_logger::testQsizeToString()
{
    QSize size = QSize(10,20);
    QString sizeString = Logger::qsizeToString(size);
    QCOMPARE(sizeString,QString(":QSize(10x20)"));
}
void ut_logger::testBoolToString()
{
    bool flag = true;
    QString boolString = Logger::boolToString(flag);
    QCOMPARE(boolString,QString(":true"));
    flag = false;
    boolString = Logger::boolToString(flag);
    QCOMPARE(boolString,QString(":false"));
}
int main ( int argc, char *argv[] ){
    QCoreApplication app( argc, argv );
    ut_logger test;
    return QTest::qExec( &test, argc, argv );

}
