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
#include <QDateTime>
#include <QTextStream>
#include <QDir>
#include <QSize>
#include <QDebug>
#include "logger.h"

bool Logger::existFlag = true;
Logger::Logger()
{
    //empty
}

void Logger::log(const QString logInfo)
{

    QString homePath = QDir::homePath();
    QFile data(homePath+"/.local/share/quill/log.txt");
    if(data.exists()){
        data.open(QFile::ReadWrite | QFile::Append);
        QTextStream out(&data);

        QDateTime timeStamp = QDateTime::currentDateTime();
        QDate date = timeStamp.date();
        QTime time = timeStamp.time();
        out << date.toString("yyyy-MM-dd")<<" "<<time.toString("hh:mm:ss:zzz")<<" "<<logInfo<<endl;
        data.close();
        existFlag = true;
    }
    else
        existFlag = false;
}


bool Logger::existLog()
{
    return existFlag;
}

QString Logger::intToString(const int value)
{
    return QString(":")+QString::number(value);
}

QString Logger::qsizeToString(const QSize size)
{
    return QString(":")+QString("QSize(")+QString::number(size.width())+QString("x")+QString::number(size.height())+QString(")");
}

QString Logger::boolToString(const bool value)
{
    if(value)
        return QString(":")+QString("true");
    else
        return QString(":")+QString("false");
}
